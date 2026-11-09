# Plan de pruebas — Fase 4

Para cada prueba se indica la evidencia esperada (captura de pantalla). Guardar las capturas en `docs/capturas/`.

| ID | Prueba | Pasos | Resultado esperado | Evidencia |
|---|---|---|---|---|
| P1 | Circuito completo | Abrir el proyecto en Wokwi e iniciar la simulación | 5 sensores y 4 actuadores conectados | Captura del circuito |
| P2 | Conexión al broker | Observar el monitor serie al iniciar | `Conectando MQTT ... OK` y `Suscrito a ...` | Captura del monitor serie |
| P3 | Publicación de telemetría | Esperar 10 s | Mensajes `[PUB]` de los 6 sensores | Captura del monitor serie |
| P4 | Recepción en el servidor | Consultar `lecturas` en PostgreSQL | Filas nuevas cada 5 s | Captura de la terminal |
| P5 | Dashboard desplegado | Abrir `http://<IP_DEL_SERVIDOR>:1880/dashboard/casa` desde otro equipo | Medidores con valores en vivo | Captura del dashboard |
| P6 | Comando → actuador | Activar el ventilador en el dashboard | El relé cambia en Wokwi; el monitor muestra `[SUB]` y `[ESTADO]` | Capturas de Wokwi y del dashboard |
| P7 | Resto de actuadores | Activar luz, puerta y alarma | Cada uno responde en Wokwi y se refleja en el dashboard | Capturas |
| P8 | Alarma de temperatura | Subir la temperatura del DHT22 por encima de 30 °C | Aparece "Temperatura alta" en el panel y en la tabla | Captura del dashboard |
| P9 | Alarma de gas | Subir el potenciómetro por encima de 600 | Suena el buzzer y aparece "Gas elevado" | Captura del dashboard y de Wokwi |
| P10 | Registro en base de datos | Consultar `estado_actuadores`, `comandos` y `alarmas` | Registros de las pruebas anteriores | Captura de la terminal |
| P11 | Histórico en Grafana | Abrir el panel "Casa inteligente integral" | Series de tiempo con las lecturas | Captura de Grafana |
| P12 | Prueba de extremo a extremo | Cambiar un sensor y enviar un comando desde la interfaz | El cambio se refleja en el actuador, el histórico y las alarmas | Video o secuencia de capturas |

Consultas útiles:

```bash
docker exec postgres psql -U domotica -d domotica -c "SELECT sensor, valor, ts FROM lecturas ORDER BY ts DESC LIMIT 10;"
docker exec postgres psql -U domotica -d domotica -c "SELECT * FROM estado_actuadores;" -c "SELECT * FROM comandos;" -c "SELECT * FROM alarmas ORDER BY ts DESC LIMIT 5;"
```
