#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "secrets.h"

// Nur aktivieren, wenn du echte Brownout-Resets siehst.
// #include "soc/soc.h"
// #include "soc/rtc_cntl_reg.h"

// ==========================
// Sensor-Pins
// ==========================
#define PIN_SOIL   14
#define PIN_LIGHT  15
#define PIN_TEMP   13

// LEDs
#define FLASH_LED_PIN 4
#define STATUS_LED_PIN 33   // kleine rote LED, active LOW

// ==========================
// Kamera-Pins AI Thinker ESP32-CAM
// ==========================
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

// ==========================
// Hilfsfunktionen
// ==========================

int readAverageAnalog(int pin, int samples = 10) {
    long sum = 0;

    for (int i = 0; i < samples; i++) {
        sum += analogRead(pin);
        delay(10);
    }

    return sum / samples;
}

void readSensorsOnce(int &soilRaw, int &lightRaw, int &tempRaw) {
    Serial.println();
    Serial.println("=== SENSORTEST VOR WIFI ===");

    soilRaw  = readAverageAnalog(PIN_SOIL);
    lightRaw = readAverageAnalog(PIN_LIGHT);
    tempRaw  = readAverageAnalog(PIN_TEMP);

    Serial.printf("Bodenfeuchte GPIO14: %d\n", soilRaw);
    Serial.printf("Licht       GPIO15: %d\n", lightRaw);
    Serial.printf("Temperatur  GPIO13: %d\n", tempRaw);

    Serial.println("Sensorwerte gespeichert. Ab jetzt kein analogRead mehr.");
}

bool initCamera() {
    Serial.println();
    Serial.println("=== KAMERATEST ===");

    camera_config_t config;

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;

    config.pin_d0       = Y2_GPIO_NUM;
    config.pin_d1       = Y3_GPIO_NUM;
    config.pin_d2       = Y4_GPIO_NUM;
    config.pin_d3       = Y5_GPIO_NUM;
    config.pin_d4       = Y6_GPIO_NUM;
    config.pin_d5       = Y7_GPIO_NUM;
    config.pin_d6       = Y8_GPIO_NUM;
    config.pin_d7       = Y9_GPIO_NUM;

    config.pin_xclk     = XCLK_GPIO_NUM;
    config.pin_pclk     = PCLK_GPIO_NUM;
    config.pin_vsync    = VSYNC_GPIO_NUM;
    config.pin_href     = HREF_GPIO_NUM;

    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;

    config.pin_pwdn     = PWDN_GPIO_NUM;
    config.pin_reset    = RESET_GPIO_NUM;

    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    // Für Test klein und stabil halten
    config.frame_size   = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count     = 1;

    esp_err_t err = esp_camera_init(&config);

    if (err != ESP_OK) {
        Serial.printf("Kamera-Init FEHLER: 0x%x\n", err);
        return false;
    }

    Serial.println("Kamera-Init OK.");
    return true;
}

bool takePhotoTest() {
    Serial.println("Nehme Testfoto auf...");

    camera_fb_t *fb = esp_camera_fb_get();

    if (!fb) {
        Serial.println("Foto FEHLER.");
        return false;
    }

    Serial.printf("Foto OK. JPEG-Groesse: %u Bytes\n", fb->len);

    // Hier würdest du später fb->buf und fb->len per HTTP POST senden.
    esp_camera_fb_return(fb);

    return true;
}

bool connectWiFi() {
    Serial.println();
    Serial.println("=== WIFI TEST NACH SENSOR/KAMERA ===");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    Serial.print("Verbinde mit WLAN");

    int counter = 0;
    while (WiFi.status() != WL_CONNECTED && counter < 30) {
        delay(500);
        Serial.print(".");
        counter++;
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi OK.");
        Serial.print("IP-Adresse: ");
        Serial.println(WiFi.localIP());
        return true;
    }

    Serial.println("WiFi FEHLER / TIMEOUT.");
    return false;
}

// ==========================
// Setup
// ==========================

void setup() {
    // Nur einschalten, falls du im Serial Monitor Brownout-Meldungen bekommst.
    // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.begin(115200);
    delay(2000);

    Serial.println();
    Serial.println("======================================");
    Serial.println(" FINALER HARDWARETEST VOR DEM LOETEN");
    Serial.println(" Sensoren -> Kamera -> WiFi");
    Serial.println("======================================");

    pinMode(FLASH_LED_PIN, OUTPUT);
    digitalWrite(FLASH_LED_PIN, LOW);

    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, HIGH); // active LOW: aus

    pinMode(PIN_SOIL, INPUT);
    pinMode(PIN_LIGHT, INPUT);
    pinMode(PIN_TEMP, INPUT);

    analogReadResolution(12);

    // Messbereich bis ca. 3.3V
    analogSetPinAttenuation(PIN_SOIL, ADC_11db);
    analogSetPinAttenuation(PIN_LIGHT, ADC_11db);
    analogSetPinAttenuation(PIN_TEMP, ADC_11db);

    // 1. Sensoren lesen, solange WiFi AUS ist
    int soilRaw = 0;
    int lightRaw = 0;
    int tempRaw = 0;

    readSensorsOnce(soilRaw, lightRaw, tempRaw);

    // 2. Kamera initialisieren und Foto testen
    bool cameraOk = initCamera();
    bool photoOk = false;

    if (cameraOk) {
        photoOk = takePhotoTest();
    }

    // 3. Danach WiFi verbinden
    bool wifiOk = connectWiFi();

    // 4. Ergebnis ausgeben
    Serial.println();
    Serial.println("========== TESTERGEBNIS ==========");

    Serial.printf("Bodenfeuchte gespeichert: %d\n", soilRaw);
    Serial.printf("Licht gespeichert:        %d\n", lightRaw);
    Serial.printf("Temperatur gespeichert:   %d\n", tempRaw);

    Serial.print("Kamera Init: ");
    Serial.println(cameraOk ? "OK" : "FEHLER");

    Serial.print("Foto:        ");
    Serial.println(photoOk ? "OK" : "FEHLER");

    Serial.print("WiFi:        ");
    Serial.println(wifiOk ? "OK" : "FEHLER");

    if (cameraOk && photoOk && wifiOk) {
        Serial.println();
        Serial.println("GESAMT: OK - Hardware grundsaetzlich loetbereit.");
    } else {
        Serial.println();
        Serial.println("GESAMT: NOCH NICHT LOETEN.");
        Serial.println("Erst Fehler oben beheben.");
    }

    Serial.println("==================================");
    Serial.println("Test abgeschlossen. Loop blinkt nur noch.");
}

void loop() {
    // Keine Sensorwerte nach WiFi lesen.
    // Nur Lebenszeichen.
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(100);
    digitalWrite(FLASH_LED_PIN, LOW);
    delay(1900);
}