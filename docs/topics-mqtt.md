# Tópicos y mensajes MQTT

Broker: `broker.emqx.io:1883` (público, sin autenticación). Prefijo: `aco941/<PREFIJO_EQUIPO>`.

## Telemetría (el ESP32 publica)

| Topic | Unidad | Rango simulado |
|---|---|---|
| `<prefijo>/sensores/temperatura` | °C | 0 – 50 |
| `<prefijo>/sensores/humedad` | % | 0 – 100 |
| `<prefijo>/sensores/luminosidad` | % | 0 – 100 |
| `<prefijo>/sensores/gas` | ppm (simulados) | 0 – 1000 |
| `<prefijo>/sensores/distancia` | cm | 2 – 400 |
| `<prefijo>/sensores/movimiento` | bool (0/1) | 0 – 1 |

Mensaje:

```json
{"dispositivo":"casa1","sensor":"temperatura","valor":28.5,"unidad":"C","fecha":"2026-10-30T12:34:37"}
```

Periodo de publicación: 5 s. El movimiento se publica además de inmediato al detectar un cambio.

## Comandos (el ESP32 se suscribe)

Topic: `<prefijo>/actuadores/<actuador>` con `<actuador>` ∈ `ventilador`, `luz`, `puerta`, `alarma`.

```json
{"dispositivo":"casa1","actuador":"ventilador","comando":"ON"}
```

`comando` acepta `ON` u `OFF`. También se acepta el texto plano `ON`/`OFF`.

## Estado (el ESP32 publica la confirmación)

Topic: `<prefijo>/estado/<actuador>` (mensaje retenido).

```json
{"dispositivo":"casa1","actuador":"ventilador","estado":"ON","fecha":"2026-10-30T12:37:07"}
```

Con el mensaje retenido, el dashboard recupera el último estado conocido al conectarse. Además, el ESP32 publica el estado real de sus 4 actuadores cada vez que se conecta al broker, para que el dashboard nunca muestre un estado antiguo tras un reinicio.

## Reglas locales en el ESP32

Si el gas supera 600 ppm, el ESP32 activa la alarma (buzzer) sin esperar al servidor, y la desactiva cuando baja de 500 ppm (histéresis).
