#include <Arduino.h>
#include "config.h"

uint8_t current_position = 0;
int8_t direction = 1;
uint32_t last_update = 0;

void setup() {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        pinMode(LED_PIN_START + i, OUTPUT);
        digitalWrite(LED_PIN_START + i, LOW);
    }
    last_update = millis();
}

void loop() {
    uint32_t now = millis();

    if (now - last_update >= CHASE_SPEED) {
        last_update = now;

        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            digitalWrite(LED_PIN_START + i, LOW);
        }

        digitalWrite(LED_PIN_START + current_position, HIGH);

        current_position += direction;

        if (current_position == 0) {
            direction = 1;
        } else if (current_position == NUM_LEDS - 1) {
            direction = -1;
        }
    }
}
