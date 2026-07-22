#include <Arduino.h>

// Historischer Test dauerhaft deaktiviert: GPIO16 ist bei aktiviertem PSRAM
// fuer die Speicheranbindung reserviert und darf nicht als GPIO benutzt werden.

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("Warn-LED-Test deaktiviert: reservierter PSRAM-Pin.");
}

void loop() {
    delay(1000);
}
