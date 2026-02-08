#include <Arduino.h>
#include "config.h"

void setup() {
    pinMode(LED_PIN_START, OUTPUT);
}

void loop() {
    digitalWrite(LED_PIN_START, HIGH);
    delay(500);
    digitalWrite(LED_PIN_START, LOW);
    delay(500);
}
