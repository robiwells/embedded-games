#include <Arduino.h>
#include "config.h"

GameState current_state = STATE_ATTRACT;
uint8_t current_position = 0;
int8_t direction = 1;
uint32_t last_chase_update = 0;
uint16_t score = 0;
uint16_t high_score = 0;

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
    Serial.println("Light Chaser - Press button to start!");
    last_chase_update = millis();
}

bool button_just_pressed() {
    uint32_t now = millis();
    bool current = !digitalRead(BUTTON_PIN);
    bool pressed = false;

    if (current && !last_button_state) {
        if (now - last_debounce_time >= DEBOUNCE_MS) {
            pressed = true;
            last_debounce_time = now;
        }
    }

    last_button_state = current;
    return pressed;
}

void update_chase() {
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

void loop() {
    uint32_t now = millis();

    if (now - last_chase_update >= CHASE_SPEED) {
        last_chase_update = now;
        update_chase();
    }

    if (current_state == STATE_ATTRACT) {
        if (button_just_pressed()) {
            score = 0;
            current_state = STATE_PLAYING;
            Serial.println("Game started!");
        }

    } else if (current_state == STATE_PLAYING) {
        if (button_just_pressed()) {
            if (current_position >= TARGET_ZONE_START && current_position <= TARGET_ZONE_END) {
                score += 10;
                Serial.print("Hit! Score: ");
                Serial.println(score);
            } else {
                Serial.print("Miss! Final score: ");
                Serial.println(score);
                if (score > high_score) {
                    high_score = score;
                    Serial.print("New high score: ");
                    Serial.println(high_score);
                }
                current_state = STATE_GAME_OVER;
            }
        }

    } else if (current_state == STATE_GAME_OVER) {
        static uint32_t game_over_time = 0;
        static bool game_over_started = false;

        if (!game_over_started) {
            game_over_time = millis();
            game_over_started = true;
        }

        if (millis() - game_over_time >= 2000) {
            game_over_started = false;
            current_state = STATE_ATTRACT;
            Serial.println("Press button to start!");
        }
    }
}
