#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "secrets.h" // Lädt die Zugangsdaten lokal

// Pins für die Sensoren (Alle auf ADC1)
#define PIN_SOIL 14
#define PIN_LIGHT 15
#define PIN_TEMP 13

// Kamera-Konfiguration für AI-Thinker Board
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void initCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    // Niedrige Auflösung für den schnellen Test
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Kamera-Initiierung fehlgeschlagen: 0x%x\n", err);
    } else {
        Serial.println("Kamera erfolgreich gestartet!");
    }
}

// void setup() {
//     Serial.begin(115200);
//     delay(2000);
//     Serial.println("\n=== INTEGRATIONSTEST START ===");

//     // 1. Kamera starten
//     initCamera();

//     // 2. Test-Foto schießen (In den Speicher laden)
//     camera_fb_t * fb = esp_camera_fb_get();
//     if(!fb) {
//         Serial.println("Foto-Aufnahme fehlgeschlagen!");
//     } else {
//         Serial.printf("Foto erfolgreich aufgenommen! Groesse: %u Bytes\n", fb->len);
//         esp_camera_fb_return(fb); // Speicher wieder freigeben
//     }

//     // 3. Sensoren auslesen
//     int soilRaw = analogRead(PIN_SOIL);
//     int lightRaw = analogRead(PIN_LIGHT);
//     int tempRaw = analogRead(PIN_TEMP);

//     Serial.println("\n--- MESSWERTE ---");
//     Serial.printf("Bodenfeuchte (Rohwert): %d\n", soilRaw);
//     Serial.printf("Licht (Rohwert): %d\n", lightRaw);
//     Serial.printf("Temperatur (Rohwert): %d\n", tempRaw);

//     // 4. WLAN testen
//     WiFi.begin(ssid, password);
//     Serial.print("\nVerbinde mit WLAN...");
//     int counter = 0;
//     while (WiFi.status() != WL_CONNECTED && counter < 20) {
//         delay(500);
//         Serial.print(".");
//         counter++;
//     }

//     if (WiFi.status() == WL_CONNECTED) {
//         Serial.println("\nWLAN erfolgreich verbunden!");
//         Serial.print("IP: "); Serial.println(WiFi.localIP());
//     } else {
//         Serial.println("\nWLAN Timeout!");
//     }

//     // 5. Ab in den Deep Sleep (Für den Upload: IO0 auf GND ziehen und RST drücken!)
//     Serial.println("\nTest beendet. Deep Sleep fuer 30 Sekunden...");
//     Serial.flush();
//     esp_deep_sleep(30ULL * 1000000ULL);
// }

// void loop() {}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n=== SENSOR LIVE TEST ===");
    initCamera();
}

void loop() {
    // Sensoren kontinuierlich auslesen
    int soilRaw = analogRead(PIN_SOIL);
    int lightRaw = analogRead(PIN_LIGHT);
    int tempRaw = analogRead(PIN_TEMP);

    Serial.printf("Feuchte: %d | Licht: %d | Temp: %d\n", soilRaw, lightRaw, tempRaw);

    // Alle 500ms messen
    delay(500);
}