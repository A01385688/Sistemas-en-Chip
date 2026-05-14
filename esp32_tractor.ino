/* ============================================================
 * esp32_tractor.ino
 * ESP32 — Recibe datos del STM32 por UART y los reenvía
 * por MQTT (WiFi) a la Raspberry Pi.
 *
 * Trama esperada del STM32: "RPM,VSPEED,GEAR\r\n"
 * Ejemplo:                  "2400,60,2\r\n"
 *
 * Conexión física ESP32 ↔ STM32
 *   ESP32 GPIO16 (RX2) → STM32 PA9  (TX)
 *   ESP32 GPIO17 (TX2) → STM32 PA10 (RX)  (opcional)
 *   GND                → GND
 *
 * Librerías necesarias (instalar en Arduino IDE):
 *   - PubSubClient  by Nick O'Leary
 * ============================================================ */

#include <WiFi.h>
#include <PubSubClient.h>

/* ── Configuración WiFi ─────────────────────────────────────── */
#define WIFI_SSID     "WASAAA 6447"        // <-- cambia esto
#define WIFI_PASSWORD "51G7@w33"   // <-- cambia esto

/* ── Configuración MQTT ─────────────────────────────────────── */
#define MQTT_BROKER   "192.168.137.177"        // <-- IP de tu Raspberry Pi
#define MQTT_PORT     1883
#define MQTT_TOPIC    "tractor/datos"      // <-- topic que leerá la RPi
#define MQTT_CLIENT   "ESP32_Tractor"

/* ── UART2 para recibir del STM32 ───────────────────────────── */
#define STM32_RX_PIN  16    // GPIO16 = RX2
#define STM32_TX_PIN  17    // GPIO17 = TX2 (no se usa pero se declara)
#define STM32_BAUD    1

/* ── Objetos globales ───────────────────────────────────────── */
WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

/* Buffer de recepción UART */
#define BUF_SIZE 32
char    rxBuf[BUF_SIZE];
uint8_t rxIdx = 0;

/* ============================================================
 * Conectar WiFi
 * ============================================================ */
void wifi_connect()
{
    Serial.print("Conectando WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi conectado. IP: " + WiFi.localIP().toString());
}

/* ============================================================
 * Conectar/reconectar MQTT
 * ============================================================ */
void mqtt_connect()
{
    while (!mqtt.connected())
    {
        Serial.print("Conectando MQTT...");
        if (mqtt.connect(MQTT_CLIENT))
        {
            Serial.println(" OK");
        }
        else
        {
            Serial.print(" Error: ");
            Serial.println(mqtt.state());
            delay(3000);
        }
    }
}

/* ============================================================
 * Procesar trama completa recibida del STM32
 * Formato: "RPM,VSPEED,GEAR"  (sin \r\n, ya se quitaron)
 * Publica en MQTT como JSON: {"rpm":2400,"speed":60,"gear":2}
 * ============================================================ */
void process_frame(char *frame)
{
    /* Parsear los tres valores separados por coma */
    char *token = strtok(frame, ",");
    if (token == NULL) return;
    int rpm = atoi(token);

    token = strtok(NULL, ",");
    if (token == NULL) return;
    int speed = atoi(token);

    token = strtok(NULL, ",");
    if (token == NULL) return;
    int gear = atoi(token);

    /* Construir JSON */
    char payload[64];
    snprintf(payload, sizeof(payload),
             "{\"rpm\":%d,\"speed\":%d,\"gear\":%d}",
             rpm, speed, gear);

    /* Publicar en MQTT */
    if (mqtt.connected())
    {
        mqtt.publish(MQTT_TOPIC, payload);
        Serial.println("MQTT → " + String(payload));
    }
}

/* ============================================================
 * SETUP
 * ============================================================ */
void setup()
{
    /* Monitor serie para debug */
    Serial.begin(115200);

    /* UART2 para recibir del STM32 */
    Serial2.begin(STM32_BAUD, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);

    wifi_connect();

    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    mqtt_connect();

    Serial.println("Sistema listo.");
}

/* ============================================================
 * LOOP
 * ============================================================ */
void loop()
{
    /* Mantener conexión MQTT activa */
    if (!mqtt.connected()) mqtt_connect();
    mqtt.loop();

    /* Leer bytes del STM32 por UART2 */
    while (Serial2.available())
    {
        char c = (char)Serial2.read();

        if (c == '\n')
        {
            /* Fin de trama: quitar \r si existe y procesar */
            if (rxIdx > 0 && rxBuf[rxIdx - 1] == '\r')
                rxIdx--;

            rxBuf[rxIdx] = '\0';   /* terminar string */

            if (rxIdx > 0)
                process_frame(rxBuf);

            rxIdx = 0;             /* reiniciar buffer */
        }
        else if (rxIdx < BUF_SIZE - 1)
        {
            rxBuf[rxIdx++] = c;
        }
        else
        {
            /* Buffer overflow: descartar y reiniciar */
            rxIdx = 0;
        }
    }
}
