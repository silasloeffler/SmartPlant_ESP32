#include <Arduino.h>
#include <WiFi.h>
#include "soc/soc.h"             // Wird für den Brownout-Fix benötigt
#include "soc/rtc_cntl_reg.h"    // Wird für den Brownout-Fix benötigt
#include "secrets.h"

void setup() {
    WRITE_PERI_REG(0x3ff4800c, 0); // Deaktiviert den Brownout-Schutz direkt über die Register-Adresse

    Serial.begin(115200);
    delay(2000);
    Serial.println("\n--- WiFi Test Start ---");

    // ... Rest deines WLAN-Codes
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n--- WiFi Test Start ---");

    // WLAN-Verbindung aufbauen
    WiFi.begin(ssid, password);
    Serial.print("Verbinde mit WLAN");

    // Warten, bis Verbindung steht (max. 15 Sekunden)
    int counter = 0;
    while (WiFi.status() != WL_CONNECTED && counter < 30) {
        delay(500);
        Serial.print(".");
        counter++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nErfolgreich verbunden!");
        Serial.print("IP-Adresse: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nVerbindung fehlgeschlagen (Timeout).");
    }

    // Deep Sleep Simulation für das spätere Projekt
    Serial.println("Gehe jetzt in den Deep Sleep für 10 Sekunden...");
    Serial.flush();

    // 10 Sekunden schlafen (Angabe in Mikrosekunden)
    esp_deep_sleep(10ULL * 1000000ULL);
}

void loop() {
    // Bleibt im Deep Sleep leer, da der Chip nach dem Aufwachen setup() neu startet
}