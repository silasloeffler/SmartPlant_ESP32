#include <Arduino.h>

// GPIO 14 für den kapazitiven Sensor
#define SOIL_PIN 14

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("--- Kapazitiver Sensorentest aktiv ---");
    pinMode(SOIL_PIN, INPUT);
}

void loop() {
    // Rohwert einlesen (0 bis 4095)
    int rawValue = analogRead(SOIL_PIN);

    Serial.print("Bodenfeuchte Rohwert: ");
    Serial.println(rawValue);

    delay(1000);
}

// fix: GND-Verkabelung korrigiert und Sensor-Baselines ermittelt

// * Fehlerhaften GND-Pin (rechts unten) identifiziert und auf funktionierenden Masse-Pin (links oben unter 5V) gewechselt
// * Serielle Kommunikation und Status-LED laufen stabil
// * Kalibrierungswerte dokumentiert: Luft (Trockenwert) = ~2500, Wasser (Nasswert) = ~471
// * Umgebungsluftfeuchtigkeit bei Testlauf: 49%