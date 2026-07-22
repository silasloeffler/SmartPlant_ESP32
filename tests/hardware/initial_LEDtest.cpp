#include <Arduino.h>

const int testLedPin = 4; // Typischer onboard-LED-Pin beim ESP32-CAM (aktiv LOW)

void setup() {
    pinMode(testLedPin, OUTPUT);
    digitalWrite(testLedPin, HIGH); // LED aus

    Serial.begin(115200);
    delay(2000);

    Serial.println("=================================================");
    Serial.println("ESP32-CAM TEST: Wenn du das hier siehst, ist UART OK");
    Serial.println("=================================================");
}

void loop() {
    digitalWrite(testLedPin, LOW);
    Serial.println("LED AN");
    delay(1000);

    digitalWrite(testLedPin, HIGH);
    Serial.println("LED AUS");
    delay(1000);
}

