#include <Arduino.h>

// GPIO 14 für den kapazitiven Sensor
#define SOIL_PIN 16

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