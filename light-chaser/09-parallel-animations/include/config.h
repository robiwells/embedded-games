#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

const uint8_t LED_PIN_START = 2;
const uint8_t BUTTON_PIN = 10;
const uint8_t BUZZER_PIN = 11;

const uint8_t NUM_LEDS = 8;
const uint8_t TARGET_ZONE_START = 3;
const uint8_t TARGET_ZONE_END = 4;

const uint16_t DEBOUNCE_MS = 50;
const uint16_t INITIAL_CHASE_SPEED = 200;
const uint16_t MIN_CHASE_SPEED = 50;
const uint16_t SPEED_DECREASE = 10;

const uint8_t BULLSEYE_SCORE = 10;

const uint16_t FREQ_TICK = 100;
const uint16_t FREQ_HIT = 500;
const uint16_t FREQ_BULLSEYE_1 = 800;
const uint16_t FREQ_BULLSEYE_2 = 1000;
const uint16_t FREQ_BULLSEYE_3 = 1200;
const uint16_t FREQ_GAME_OVER_1 = 400;
const uint16_t FREQ_GAME_OVER_2 = 300;
const uint16_t FREQ_GAME_OVER_3 = 200;

const uint16_t DURATION_TICK = 20;
const uint16_t DURATION_HIT = 100;
const uint16_t DURATION_BULLSEYE_NOTE = 100;
const uint16_t DURATION_GAME_OVER_NOTE = 200;

const uint16_t GAME_OVER_LED_FLASH_DURATION = 150;
const uint8_t GAME_OVER_LED_FLASH_COUNT = 5;

const uint16_t CELEBRATION_LED_DELAY = 40;
const uint8_t CELEBRATION_SWEEPS = 3;

const uint8_t LCD_ADDRESS = 0x27;
const uint8_t LCD_COLS = 16;
const uint8_t LCD_ROWS = 2;

const uint16_t EEPROM_HIGH_SCORE_ADDR = 0;
const uint8_t EEPROM_MAGIC_BYTE = 0xA5;

enum GameState {
    STATE_ATTRACT,
    STATE_PLAYING,
    STATE_RESULT,
    STATE_CELEBRATION,
    STATE_GAME_OVER
};

#endif
