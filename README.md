# Progetto individuale per il corso di Mobile Systems M (A.A. 2021/2022)
> Luca Bartolomei **0000979388**

- [Rete mesh Cluster Tree](https://docs.espressif.com/projects/esp-idf/en/v4.4.1/esp32/api-reference/network/esp-wifi-mesh.html) con scambio di pacchetti da sensori fake ([random gaussiano](https://docs.espressif.com/projects/esp-idf/en/v4.4.1/esp32/api-reference/system/random.html))
- Cifratura pacchetti tramite WPA2 
- Protocollo UDP per lo scambio cluster head - gateway
- [HMAC](https://tls.mbed.org/api/md_8h.html) (*accelerazione HW solo di SHA-256*) per autenticazione pacchetto (segreto condiviso) 
- Messaggi di controllo autenticati server -> sensori (per ora invio di timestamp per sincronizzazone)
- Gateway di benchmark per misurare le performance
- Gateway Azure IoT (compilazione e installazione dell'[SDK Azure](https://github.com/Azure/azure-iot-sdk-c))
- IoT Hub Azure for Students
