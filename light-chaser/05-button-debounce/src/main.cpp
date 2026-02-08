#include <Arduino.h>
#include "config.h"

uint8_t current_position = 0;
int8_t direction = 1;
uint32_t last_chase_update = 0;
uint16_t score = 0;

bool last_button_state = false;
uint32_t last_debounce_time = 0;

void setup() {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        pinMode(LED_PIN_START + i, OUTPUT);
        digitalWrite(LED_PIN_START + i, LOW);
    }

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    last_button_state = !digitalRead(BUTTON_PIN);

    Serial.begin(9600);
    last_chase_update = millis();
}

bool button_just_pressed() {
    uint32_t now = millis();
    bool current_state = !digitalRead(BUTTON_PIN);
    bool pressed = false;

    if (current_state && !last_button_state) {
        if (now - last_debounce_time >= DEBOUNCE_MS) {
            pressed = true;
            last_debounce_time = now;
        }
    }

    last_button_state = current_state;
    return pressed;
}

void loop() {
    uint32_t now = millis();

    if (now - last_chase_update >= CHASE_SPEED) {
        last_chase_update = now;

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

    if (button_just_pressed()) {
        if (current_position >= TARGET_ZONE_START && current_position <= TARGET_ZONE_END) {
            score += 10;
            Serial.print("Hit! Score: ");
            Serial.println(score);
        } else {
            Serial.print("Miss! Final score: ");
            Serial.println(score);
            score = 0;
        }
    }
}
