#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Pin definitions
const uint8_t LED_PIN_START = 2;
const uint8_t BUTTON_PIN = 10;
const uint8_t BUZZER_PIN = 11;
// Pins 12-13 now available for future expansion (previously TM1637)
// LCD uses I2C: SDA (A4), SCL (A5)

// LED configuration
const uint8_t NUM_LEDS = 8;
const uint8_t TARGET_ZONE_START = 3;
const uint8_t TARGET_ZONE_END = 4;

// Timing constants (milliseconds)
const uint16_t DEBOUNCE_MS = 50;
const uint16_t INITIAL_CHASE_SPEED = 200;
const uint16_t MIN_CHASE_SPEED = 50;
const uint16_t SPEED_DECREASE = 5;
const uint16_t SPEED_INTERVAL = 10;

// Scoring constants
const uint8_t BULLSEYE_SCORE = 10;
const uint8_t ADJACENT_SCORE = 5;
const uint8_t OUTER_SCORE = 1;

// Sound frequencies (Hz)
const uint16_t FREQ_TICK = 100;
const uint16_t FREQ_HIT = 500;
const uint16_t FREQ_BULLSEYE_1 = 800;
const uint16_t FREQ_BULLSEYE_2 = 1000;
const uint16_t FREQ_BULLSEYE_3 = 1200;
const uint16_t FREQ_GAME_OVER_1 = 400;
const uint16_t FREQ_GAME_OVER_2 = 300;
const uint16_t FREQ_GAME_OVER_3 = 200;

// Sound durations (milliseconds)
const uint16_t DURATION_TICK = 20;
const uint16_t DURATION_HIT = 100;
const uint16_t DURATION_BULLSEYE_NOTE = 100;
const uint16_t DURATION_GAME_OVER_NOTE = 200;

// Game over LED flash configuration
const uint16_t GAME_OVER_LED_FLASH_DURATION = 150;  // milliseconds per flash state
const uint8_t GAME_OVER_LED_FLASH_COUNT = 5;        // number of complete flash cycles

// LCD configuration
const uint8_t LCD_ADDRESS = 0x27;  // Try 0x3F if 0x27 doesn't work
const uint8_t LCD_COLS = 16;
const uint8_t LCD_ROWS = 2;

// EEPROM configuration
const uint16_t EEPROM_HIGH_SCORE_ADDR = 0;
const uint8_t EEPROM_MAGIC_BYTE = 0xA5;

// Celebration effect constants
const uint16_t CELEBRATION_LED_DELAY = 40;  // milliseconds per LED
const uint8_t CELEBRATION_SWEEPS = 3;        // number of wave sweeps

// Game states
enum GameState {
    STATE_ATTRACT,
    STATE_PLAYING,
    STATE_RESULT,
    STATE_CELEBRATION,
    STATE_GAME_OVER
};

#endif // CONFIG_H
