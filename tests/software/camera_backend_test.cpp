#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "app_config.h"
#include "esp_camera.h"
#include "secrets.h"

static_assert(!AUTO_ANALYZE_ON_BOOT,
              "Automatische Backend-Analyse muss fuer den Diagnosetest aus bleiben.");

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

// GPIO16 bleibt absichtlich vollstaendig unberuehrt. Bei aktiviertem PSRAM
// ist dieser Pin fuer die Speicheranbindung reserviert. Die fest angeschlossene
// externe Warn-LED wird in diesem Diagnosetest deshalb nicht verwendet.

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
    Serial.println("=== SENSORMESSUNG VOR WIFI ===");

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
    Serial.println("=== KAMERA-INITIALISIERUNG ===");

    camera_config_t config = {};

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

    // Fuer Test klein und stabil halten
    config.frame_size   = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count     = 1;
    config.fb_location  = CAMERA_USE_INTERNAL_DRAM
        ? CAMERA_FB_IN_DRAM
        : CAMERA_FB_IN_PSRAM;
    config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;

    esp_err_t err = esp_camera_init(&config);

    if (err != ESP_OK) {
        Serial.printf("Kamera-Init FEHLER: 0x%x\n", err);
        return false;
    }

    Serial.println("Kamera-Init OK.");
    return true;
}

bool connectWiFi() {
    Serial.println();
    Serial.println("=== WLAN-VERBINDUNG ===");

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

bool validateAndPrintJpeg(const camera_fb_t *fb) {
    Serial.printf("Framebuffer-Format: %d (PIXFORMAT_JPEG=%d)\n",
                  static_cast<int>(fb->format),
                  static_cast<int>(PIXFORMAT_JPEG));
    Serial.printf("Framebuffer-Laenge: %u Bytes\n", fb->len);

    if (fb->len < 4) {
        Serial.println("JPEG-Bytes: weniger als vier Bytes vorhanden.");
        Serial.println("JPEG lokal ungueltig");
        return false;
    }

    Serial.printf("Erste vier Bytes: %02X %02X %02X %02X\n",
                  fb->buf[0], fb->buf[1], fb->buf[2], fb->buf[3]);
    Serial.printf("Letzte vier Bytes: %02X %02X %02X %02X\n",
                  fb->buf[fb->len - 4], fb->buf[fb->len - 3],
                  fb->buf[fb->len - 2], fb->buf[fb->len - 1]);

    bool valid = fb->format == PIXFORMAT_JPEG
        && fb->buf[0] == 0xFF
        && fb->buf[1] == 0xD8
        && fb->buf[fb->len - 2] == 0xFF
        && fb->buf[fb->len - 1] == 0xD9;

    Serial.println(valid ? "JPEG lokal gueltig" : "JPEG lokal ungueltig");
    return valid;
}

bool runLocalCameraTest() {
    Serial.println();
    Serial.println("=== LOKALER KAMERATEST ===");
    Serial.println("WLAN und Backend bleiben ausgeschaltet.");

    if (!initCamera()) {
        return false;
    }

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Kamera-FEHLER: Framebuffer ist null.");
        return false;
    }

    bool valid = validateAndPrintJpeg(fb);

    // Genau eine kontrollierte Rueckgabe. Die Kamera bleibt bis zum Reset
    // initialisiert, um den problematischen Deinit-Pfad nicht zu betreten.
    esp_camera_fb_return(fb);
    return valid;
}

bool sendPhotoToBackend(int soilRaw, int lightRaw, int tempRaw) {
    Serial.println();
    Serial.println("=== BACKEND-TRANSPORT ===");
    Serial.println("Nehme frisches JPEG auf...");

    camera_fb_t *fb = esp_camera_fb_get();

    if (!fb) {
        Serial.println("Kamera-FEHLER: JPEG-Aufnahme fehlgeschlagen.");
        return false;
    }

    bool requestOk = false;
    bool jpegValid = validateAndPrintJpeg(fb);

    if (jpegValid) {
        String requestUrl = String(backendUrl)
            + "?soil_raw=" + String(soilRaw)
            + "&light_raw=" + String(lightRaw)
            + "&temp_raw=" + String(tempRaw);

        Serial.print("POST ");
        Serial.println(requestUrl);

        HTTPClient http;
        if (http.begin(requestUrl)) {
            http.setTimeout(60000);
            http.addHeader("Content-Type", "image/jpeg");

            // Keine Konvertierung und keine Kopie: Der originale Kamera-Puffer
            // wird direkt und synchron als Request-Body gesendet.
            int httpCode = http.POST(fb->buf, fb->len);

            if (httpCode > 0) {
                Serial.printf("HTTP-Status: %d\n", httpCode);
                String response = http.getString();
                Serial.println("Vollstaendige Antwort:");
                Serial.println(response);

                requestOk = httpCode >= 200 && httpCode < 300;
                if (!requestOk) {
                    Serial.println("HTTP-FEHLER: Backend antwortete mit Fehlerstatus.");
                }
            } else {
                Serial.printf("HTTP-FEHLERCODE: %d\n", httpCode);
                Serial.print("HTTP-FEHLERMELDUNG: ");
                Serial.println(HTTPClient::errorToString(httpCode));
            }
        } else {
            Serial.println("HTTP-FEHLER: Request konnte nicht initialisiert werden.");
        }

        // Auch nach Initialisierungs- und Transportfehlern aufrufen.
        http.end();
    } else {
        Serial.println("POST aus Sicherheitsgruenden uebersprungen.");
    }

    // Ein einziger gemeinsamer Rueckgabepfad nach der optionalen Uebertragung.
    esp_camera_fb_return(fb);
    return requestOk;
}

void disableWiFi() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

bool runTransportTest() {
    Serial.println();
    Serial.println("=== EINMALIGER TRANSPORTTEST ===");

    // ADC2-Sensoren ausschliesslich bei deaktiviertem WLAN lesen.
    disableWiFi();
    pinMode(PIN_SOIL, INPUT);
    pinMode(PIN_LIGHT, INPUT);
    pinMode(PIN_TEMP, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_SOIL, ADC_11db);
    analogSetPinAttenuation(PIN_LIGHT, ADC_11db);
    analogSetPinAttenuation(PIN_TEMP, ADC_11db);

    int soilRaw = 0;
    int lightRaw = 0;
    int tempRaw = 0;
    readSensorsOnce(soilRaw, lightRaw, tempRaw);

    if (!initCamera()) {
        Serial.println("Transporttest abgebrochen: Kamera nicht initialisiert.");
        return false;
    }

    if (!connectWiFi()) {
        Serial.println("Transporttest abgebrochen: Keine WLAN-Verbindung.");
        disableWiFi();
        return false;
    }

    bool requestOk = sendPhotoToBackend(soilRaw, lightRaw, tempRaw);
    disableWiFi();

    Serial.println("WLAN ausgeschaltet. Es wird keine weitere Anfrage gesendet.");
    return requestOk;
}

void printMenu() {
    Serial.println();
    Serial.println("======================================");
    Serial.println(" SMARTPLANT SICHERER DIAGNOSETEST");
    Serial.println("======================================");
    Serial.println("Automatische Analyse beim Boot: DEAKTIVIERT");
    Serial.println("Kamera-Framebuffer: internes DRAM");
    Serial.println();
    Serial.println("p = nur lokaler Kameratest, ohne WLAN und Backend");
    Serial.println("t = Transporttest zum Backend");
    Serial.println("h = Hilfe anzeigen");
    Serial.println();
    Serial.println("Auswahl eingeben und senden:");
}

// ==========================
// Setup
// ==========================

void setup() {
    // Nur einschalten, falls du im Serial Monitor Brownout-Meldungen bekommst.
    // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.begin(115200);
    delay(2000);

    pinMode(FLASH_LED_PIN, OUTPUT);
    digitalWrite(FLASH_LED_PIN, LOW);

    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, HIGH); // active LOW: aus

    WiFi.mode(WIFI_OFF);
    printMenu();
}

void loop() {
    static bool testExecuted = false;

    if (testExecuted || Serial.available() == 0) {
        return;
    }

    char command = static_cast<char>(tolower(Serial.read()));
    if (command == '\r' || command == '\n' || command == ' ') {
        return;
    }

    if (command == 'h') {
        printMenu();
        return;
    }

    bool testOk = false;
    if (command == 'p') {
        testExecuted = true;
        testOk = runLocalCameraTest();
    } else if (command == 't') {
        testExecuted = true;
        testOk = runTransportTest();
    } else {
        Serial.println("Unbekannte Auswahl. Mit h wird die Hilfe angezeigt.");
        return;
    }

    Serial.println();
    Serial.println(testOk ? "TESTERGEBNIS: OK" : "TESTERGEBNIS: FEHLER");
    Serial.println("Test abgeschlossen. Fuer einen weiteren Test bitte resetten.");

    // Interne rote LED an GPIO33: bei Fehler an, bei Erfolg aus (active LOW).
    digitalWrite(STATUS_LED_PIN, testOk ? HIGH : LOW);
}
