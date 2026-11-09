# Arquitectura

Ver el diagrama en el [README](../README.md#arquitectura).

## Decisiones técnicas

- **ESP32 + Wokwi:** permite simular el sistema completo sin hardware físico, manteniendo el mismo código que se usaría en un dispositivo real.
- **MQTT (publicación/suscripción):** desacopla los dispositivos del servidor; el broker solo enruta mensajes.
- **Broker público (EMQX):** se probó primero el broker institucional (`sima.udb.edu.sv:1884`); desde la simulación de Wokwi la conexión TCP fallaba, por lo que se usó un broker público listado como opción válida en las indicaciones del curso. La contrapartida (sin autenticación ni cifrado) se documenta en *Limitaciones*.
- **Node-RED:** integra MQTT, base de datos y dashboard en un mismo entorno y es de bajo consumo.
- **PostgreSQL:** almacena el histórico de lecturas, estados, comandos y alarmas.
- **Grafana:** visualización del histórico sobre PostgreSQL.
- **Docker Compose:** despliegue reproducible de todos los servicios en el servidor.
- **Confirmación de estado:** el interruptor del dashboard solo cambia cuando el ESP32 confirma la acción, de modo que la interfaz refleja el estado real del actuador.
