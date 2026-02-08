#include <Arduino.h>
#include "game.h"

void setup() {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        pinMode(LED_PIN_START + i, OUTPUT);
        digitalWrite(LED_PIN_START + i, LOW);
    }

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    Serial.begin(9600);

    game_init();
}

void loop() {
    game_update();
}
