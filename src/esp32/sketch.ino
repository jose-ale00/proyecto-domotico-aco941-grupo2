/*
 * ACO941 - Proyecto Integrador - Fase 4
 * Sistema domótico "Casa inteligente integral" (ESP32 + MQTT)
 *
 * Sensores  : DHT22 (temperatura y humedad), PIR (movimiento), LDR (luminosidad),
 *             MQ-2 (gas, emulado con potenciómetro), HC-SR04 (distancia)
 * Actuadores: Relé (ventilador), LED RGB (luz), Servo (puerta), Buzzer (alarma)
 *
 * Telemetría  (publica): <BASE>/sensores/<nombre>
 * Comandos    (suscribe): <BASE>/actuadores/<nombre>   {"actuador":"ventilador","comando":"ON"}
 * Confirmación (publica): <BASE>/estado/<nombre>       {"actuador":"ventilador","estado":"ON"}
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <time.h>

// ====================== CONFIGURACIÓN ======================
const char* WIFI_SSID = "Wokwi-GUEST";   // red virtual de Wokwi
const char* WIFI_PASS = "";

const char*    MQTT_HOST = "broker.emqx.io";   // broker MQTT público (EMQX)
const uint16_t MQTT_PORT = 1883;
const char*    MQTT_USER = "";                 // el broker público no exige credenciales
const char*    MQTT_PASS = "";

const char* DEVICE_ID = "casa1";
const char* BASE      = "aco941/PREFIJO_EQUIPO"; // <-- CAMBIAR por un prefijo único (NO subir el real a Git)

const unsigned long INTERVALO_MS = 5000;       // periodo de publicación
const int UMBRAL_GAS_ON  = 600;                // alarma automática local (ppm simulados)
const int UMBRAL_GAS_OFF = 500;

// ====================== PINES ======================
#define PIN_DHT     15
#define PIN_PIR     27
#define PIN_LDR     34
#define PIN_GAS     35
#define PIN_TRIG     5
#define PIN_ECHO    18
#define PIN_RELE    26   // ventilador
#define PIN_SERVO   13   // puerta
#define PIN_BUZZER  25   // alarma
#define PIN_LED_R   14   // luz (LED RGB de cátodo común)
#define PIN_LED_G   32
#define PIN_LED_B   33

// ====================== OBJETOS ======================
DHT dht(PIN_DHT, DHT22);
Servo servoPuerta;
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long ultimaLectura = 0;
unsigned long ultimoIntentoMqtt = 0;
int pirAnterior = -1;
bool alarmaOn = false;
bool alarmaAuto = false;
bool estVentilador = false, estLuz = false, estPuerta = false;  // estado actual de los actuadores

// ====================== UTILIDADES ======================
String fechaISO() {
  struct tm t;
  if (!getLocalTime(&t, 100)) return String("sin-hora");
  char b[25];
  strftime(b, sizeof(b), "%Y-%m-%dT%H:%M:%S", &t);
  return String(b);
}

void publicarSensor(const char* nombre, float valor, const char* unidad) {
  char topic[96];
  snprintf(topic, sizeof(topic), "%s/sensores/%s", BASE, nombre);

  JsonDocument doc;
  doc["dispositivo"] = DEVICE_ID;
  doc["sensor"] = nombre;
  doc["valor"] = valor;
  doc["unidad"] = unidad;
  doc["fecha"] = fechaISO();

  char payload[256];
  serializeJson(doc, payload, sizeof(payload));
  mqtt.publish(topic, payload);
  Serial.printf("[PUB] %s -> %s\n", topic, payload);
}

void publicarEstado(const char* nombre, bool on) {
  char topic[96];
  snprintf(topic, sizeof(topic), "%s/estado/%s", BASE, nombre);

  JsonDocument doc;
  doc["dispositivo"] = DEVICE_ID;
  doc["actuador"] = nombre;
  doc["estado"] = on ? "ON" : "OFF";
  doc["fecha"] = fechaISO();

  char payload[200];
  serializeJson(doc, payload, sizeof(payload));
  mqtt.publish(topic, payload, true);  // retenido: el dashboard ve el último estado al conectarse
  Serial.printf("[ESTADO] %s -> %s\n", topic, payload);
}

// ====================== ACTUADORES ======================
void setLuz(bool on) {
  digitalWrite(PIN_LED_R, on);
  digitalWrite(PIN_LED_G, on);
  digitalWrite(PIN_LED_B, on);
}

void aplicarActuador(const String& nombre, bool on) {
  if (nombre == "ventilador") {
    digitalWrite(PIN_RELE, on ? HIGH : LOW);
    estVentilador = on;
  } else if (nombre == "luz") {
    setLuz(on);
    estLuz = on;
  } else if (nombre == "puerta") {
    servoPuerta.write(on ? 90 : 0);
    estPuerta = on;
  } else if (nombre == "alarma") {
    if (on) tone(PIN_BUZZER, 2000); else noTone(PIN_BUZZER);
    alarmaOn = on;
    if (!on) alarmaAuto = false;
  } else {
    Serial.printf("[WARN] actuador desconocido: %s\n", nombre.c_str());
    return;
  }
  publicarEstado(nombre.c_str(), on);
}

// Publica el estado real de los 4 actuadores (se llama al conectar al broker).
// Evita que el dashboard muestre un estado retenido antiguo tras reiniciar el ESP32.
void publicarEstados() {
  publicarEstado("ventilador", estVentilador);
  publicarEstado("luz", estLuz);
  publicarEstado("puerta", estPuerta);
  publicarEstado("alarma", alarmaOn);
}

// ====================== MQTT ======================
void onMensaje(char* topic, byte* payload, unsigned int len) {
  String t(topic);
  String msg;
  for (unsigned int i = 0; i < len; i++) msg += (char)payload[i];
  Serial.printf("[SUB] %s <- %s\n", topic, msg.c_str());

  // Nombre del actuador = último segmento del topic
  String nombre = t.substring(t.lastIndexOf('/') + 1);

  // Acepta JSON {"comando":"ON"} o texto plano ON/OFF
  String cmd = msg;
  JsonDocument doc;
  if (deserializeJson(doc, msg) == DeserializationError::Ok && doc["comando"].is<const char*>()) {
    cmd = doc["comando"].as<String>();
  }
  cmd.trim();
  aplicarActuador(nombre, cmd.equalsIgnoreCase("ON"));
}

void conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("Conectando WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS, 6);
  while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
  Serial.println(" OK");
  configTime(-6 * 3600, 0, "pool.ntp.org");  // El Salvador UTC-6
  struct tm t;
  for (int i = 0; i < 20 && !getLocalTime(&t, 500); i++) Serial.print("t");
  Serial.println(" hora sincronizada");
}

void conectarMqtt() {
  if (mqtt.connected()) return;
  if (millis() - ultimoIntentoMqtt < 5000) return;
  ultimoIntentoMqtt = millis();

  String clientId = String("esp32-") + DEVICE_ID + "-" + String(random(0xffff), HEX);
  Serial.printf("Conectando MQTT %s:%u ... ", MQTT_HOST, MQTT_PORT);
  bool ok = (strlen(MQTT_USER) > 0)
              ? mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)
              : mqtt.connect(clientId.c_str());
  if (ok) {
    Serial.println("OK");
    char topic[96];
    snprintf(topic, sizeof(topic), "%s/actuadores/+", BASE);
    mqtt.subscribe(topic);
    Serial.printf("Suscrito a %s\n", topic);
    publicarEstados();
  } else {
    Serial.printf("fallo, rc=%d\n", mqtt.state());
  }
}

// ====================== SENSORES ======================
float leerDistanciaCm() {
  digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  long dur = pulseIn(PIN_ECHO, HIGH, 30000);
  if (dur == 0) return -1;
  return dur * 0.0343 / 2.0;
}

void leerYPublicar() {
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();
  if (!isnan(temp)) publicarSensor("temperatura", temp, "C");
  if (!isnan(hum))  publicarSensor("humedad", hum, "%");

  int ldrRaw = analogRead(PIN_LDR);
  publicarSensor("luminosidad", map(ldrRaw, 0, 4095, 0, 100), "%");

  int gas = map(analogRead(PIN_GAS), 0, 4095, 0, 1000);  // ppm simulados
  publicarSensor("gas", gas, "ppm");

  float dist = leerDistanciaCm();
  if (dist >= 0) publicarSensor("distancia", dist, "cm");

  int pir = digitalRead(PIN_PIR);
  publicarSensor("movimiento", pir, "bool");
  pirAnterior = pir;

  // Regla local de seguridad: gas alto => alarma automática
  if (gas > UMBRAL_GAS_ON && !alarmaOn) {
    alarmaAuto = true;
    aplicarActuador("alarma", true);
  } else if (gas < UMBRAL_GAS_OFF && alarmaOn && alarmaAuto) {
    aplicarActuador("alarma", false);
  }
}

// ====================== SETUP / LOOP ======================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_RELE, OUTPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  setLuz(false);
  digitalWrite(PIN_RELE, LOW);
  dht.begin();
  servoPuerta.attach(PIN_SERVO, 500, 2400);
  servoPuerta.write(0);

  conectarWiFi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMensaje);
  mqtt.setBufferSize(512);
}

void loop() {
  conectarWiFi();
  conectarMqtt();
  mqtt.loop();

  if (mqtt.connected()) {
    // Cambio de movimiento: publica de inmediato
    int pir = digitalRead(PIN_PIR);
    if (pir != pirAnterior && pirAnterior != -1) {
      publicarSensor("movimiento", pir, "bool");
      pirAnterior = pir;
    }

    if (millis() - ultimaLectura >= INTERVALO_MS) {
      ultimaLectura = millis();
      leerYPublicar();
    }
  }
}
