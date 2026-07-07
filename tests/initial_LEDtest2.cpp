#include <Arduino.h>

#define WARN_LED_PIN 16

// additional status LED for plant needs water warning (active LOW)

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(WARN_LED_PIN, OUTPUT);

    // LED aus beim Start
    digitalWrite(WARN_LED_PIN, HIGH);

    Serial.println("GPIO16 Warn-LED Test gestartet");
}

void loop() {
    Serial.println("LED AN");
    digitalWrite(WARN_LED_PIN, LOW);   // active LOW: an
    delay(1000);

    Serial.println("LED AUS");
    digitalWrite(WARN_LED_PIN, HIGH);  // active LOW: aus
    delay(1000);
}