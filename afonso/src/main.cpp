#include <Arduino.h>

#define LED_PIN 2

void setup() {
    // write your initialization code here
    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    // write your code here
    digitalWrite(LED_PIN, LOW);
    delay(1000);
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
}
