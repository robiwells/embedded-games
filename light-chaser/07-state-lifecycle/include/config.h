#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

const uint8_t LED_PIN_START = 2;
const uint8_t NUM_LEDS = 8;
const uint8_t BUTTON_PIN = 10;
const uint8_t TARGET_ZONE_START = 3;
const uint8_t TARGET_ZONE_END = 4;

const uint16_t CHASE_SPEED = 200;
const uint16_t DEBOUNCE_MS = 50;
const uint8_t BULLSEYE_SCORE = 10;

enum GameState {
    STATE_ATTRACT,
    STATE_PLAYING,
    STATE_RESULT,
    STATE_GAME_OVER
};

#endif
