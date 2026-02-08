/******************************************************************************
 * CONFIG.H - Hardware Configuration and Game Constants
 *
 * This file demonstrates a fundamental embedded systems pattern: centralised
 * configuration. Unlike desktop applications that might read config files at
 * runtime, embedded systems typically use compile-time constants because:
 *
 * 1. Memory efficiency: Constants live in Flash (32KB), not RAM (2KB)
 * 2. No file system: Microcontrollers don't have traditional storage
 * 3. Performance: Compiler can optimise constant values
 * 4. Safety: Values can't be accidentally changed at runtime
 *
 ******************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

/******************************************************************************
 * PIN DEFINITIONS - Mapping Software to Physical Hardware
 *
 * Arduino pins are numbered 0-13 (digital) and A0-A5 (analogue). Each pin
 * can be configured as INPUT or OUTPUT at runtime using pinMode().
 *
 * LED LAYOUT (Pins 2-9):
 * We use 8 consecutive pins for the LEDs. Sequential numbering simplifies
 * code: we can iterate: for (i = 0; i < 8; i++) led_set(LED_PIN_START + i)
 *
 * Physical layout:
 *   [LED0] [LED1] [LED2] [LED3] [LED4] [LED5] [LED6] [LED7]
 *   Red    Red    Red    Green  Green  Red    Red    Red
 *   Pin2   Pin3   Pin4   Pin5   Pin6   Pin7   Pin8   Pin9
 *                         └-------┘
 *                          Bullseye zone (TARGET_ZONE_START/END)
 *
 * BUTTON (Pin 10):
 * Uses INPUT_PULLUP mode: internal 20kΩ resistor pulls pin HIGH when button
 * is not pressed. Pressing button connects pin to GND -> reads LOW.
 *
 * Wiring: Arduino Pin 10 ─── [Button] ─── GND
 *         (No external resistor needed!)
 *
 * BUZZER (Pin 11):
 * PWM-capable pin for tone() function. Generates square waves at specified
 * frequencies to produce beeps and melodies.
 *
 * I2C LCD DISPLAY (Pins A4/A5):
 * I2C (Inter-Integrated Circuit) is a 2-wire protocol for communicating with
 * peripheral devices. Only 2 pins control the entire 16×2 LCD:
 * - A4 = SDA (Serial Data) - bidirectional data line
 * - A5 = SCL (Serial Clock) - clock signal from master (Arduino)
 *
 ******************************************************************************/

const uint8_t LED_PIN_START = 2;   // First LED on pin 2, subsequent LEDs on pins 3-9
const uint8_t BUTTON_PIN = 10;
const uint8_t BUZZER_PIN = 11;

/******************************************************************************
 * LED CONFIGURATION
 *
 * NUM_LEDS: Total number of LEDs in the chase sequence
 *
 * TARGET_ZONE_START/END: Define the "bullseye" (green LED zone)
 * Only positions 3-4 score points. Positions 0-2 and 5-7 are misses.
 *
 * MEMORY NOTE: We use uint8_t (8-bit unsigned int, range 0-255) instead of
 * int (16-bit on Arduino, wastes memory). Since we only need 0-7, uint8_t is
 * perfect. Every byte of RAM saved matters on a 2KB system!
 ******************************************************************************/

const uint8_t NUM_LEDS = 8;
const uint8_t TARGET_ZONE_START = 3;  // First green LED (bullseye zone)
const uint8_t TARGET_ZONE_END = 4;    // Last green LED (bullseye zone)

/******************************************************************************
 * TIMING CONSTANTS (milliseconds)
 *
 * EMBEDDED TIMING CONCEPT:
 * We use millis() to track time. millis() returns milliseconds since power-on
 * as a uint32_t (32-bit unsigned, range 0 to 4,294,967,295). This wraps after
 * ~49.7 days of continuous operation.
 *
 * All timing uses uint16_t (16-bit) since our intervals are small (<65,535 ms).
 *
 * DEBOUNCE_MS (50ms):
 * Physical buttons "bounce" when pressed, the contacts make/break rapidly for
 * 5-20ms before settling. Without debouncing, one press registers as multiple.
 * Solution: Ignore transitions within 50ms of the last detected press.
 *
 * CHASE SPEED (200ms initial, 50ms minimum):
 * Time between LED movements. Starts at 200ms (5 LEDs/sec), decreases by 10ms
 * after each successful hit, bottoming out at 50ms (20 LEDs/sec).
 *
 * GAME DIFFICULTY TUNING:
 * - Increase INITIAL_CHASE_SPEED → easier (slower start)
 * - Decrease MIN_CHASE_SPEED → harder (faster endgame)
 * - Increase SPEED_DECREASE → difficulty ramps faster
 ******************************************************************************/

const uint16_t DEBOUNCE_MS = 50;
const uint16_t INITIAL_CHASE_SPEED = 200;  // Starting LED movement interval (ms)
const uint16_t MIN_CHASE_SPEED = 50;       // Fastest possible LED movement (ms)
const uint16_t SPEED_DECREASE = 10;         // Amount to speed up per successful hit (ms)

/******************************************************************************
 * SCORING CONSTANTS
 *
 * BULLSEYE_SCORE (10 points): Hitting green LEDs (positions 3-4)
 *
 * FUTURE EXPANSION: Could implement a "zone" scoring system where positions
 * 2 and 5 (adjacent to bullseye) score 5 points, and outer positions score 1.
 ******************************************************************************/

const uint8_t BULLSEYE_SCORE = 10;

/******************************************************************************
 * SOUND FREQUENCIES (Hz) and DURATIONS (ms)
 *
 * PWM TONE GENERATION:
 * Arduino's tone() function generates square waves at specified frequencies
 * using PWM (Pulse Width Modulation). The buzzer vibrates at this frequency
 * to produce audible tones.
 *
 * FREQUENCY SELECTION:
 * These frequencies are chosen from the musical scale (in Hz):
 * - 523 Hz = C5 (middle C, one octave up)
 * - 659 Hz = E5
 * - 784 Hz = G5
 * - 1047 Hz = C6 (high C)
 * - 1319 Hz = E6
 *
 * SOUND EFFECTS:
 * - TICK (100 Hz): Low-frequency pulse on each LED movement
 * - HIT (500 Hz): Mid-frequency beep for non-bullseye hits
 * - BULLSEYE: 3-note ascending scale (800→1000→1200 Hz)
 * - GAME_OVER: 3-note descending scale (400→300→200 Hz) 
 *
 * WHY THESE DURATIONS?
 * - TICK (20ms): Very brief click, doesn't interfere with gameplay
 * - HIT (100ms): Long enough to register, short enough to not delay game
 * - NOTE (100-300ms): Musical timing for melodies
 ******************************************************************************/

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

/******************************************************************************
 * ANIMATION CONFIGURATION
 *
 * CELEBRATION_LED_DELAY (40ms per LED):
 * Speed of the "wave" effect during celebration. LEDs light sequentially
 * left-to-right. 40ms × 8 LEDs = 320ms per complete sweep.
 *
 * CELEBRATION_SWEEPS (3 complete sweeps):
 * Total animation time: 3 sweeps × 320ms = 960ms (~1 second)
 *
 * GAME_OVER_LED_FLASH_DURATION (150ms):
 * All LEDs flash on/off together. 150ms on + 150ms off = 300ms per cycle.
 *
 * GAME_OVER_LED_FLASH_COUNT (5 cycles):
 * Total flash animation: 5 cycles × 300ms = 1500ms (1.5 seconds)
 ******************************************************************************/

const uint16_t GAME_OVER_LED_FLASH_DURATION = 150;  // milliseconds per flash state
const uint8_t GAME_OVER_LED_FLASH_COUNT = 5;        // number of complete flash cycles

const uint16_t CELEBRATION_LED_DELAY = 40;  // milliseconds per LED in wave
const uint8_t CELEBRATION_SWEEPS = 3;        // number of left-to-right sweeps

/******************************************************************************
 * LCD DISPLAY CONFIGURATION
 *
 * I2C ADDRESS (0x27):
 * Each I2C device on the bus has a unique 7-bit address (0x00-0x7F).
 * Most I2C LCD modules use address 0x27 or 0x3F.
 *
 * LCD DIMENSIONS:
 * Standard 16×2 character LCD (16 columns, 2 rows)
 ******************************************************************************/

const uint8_t LCD_ADDRESS = 0x27;  // Try 0x3F if 0x27 doesn't work
const uint8_t LCD_COLS = 16;
const uint8_t LCD_ROWS = 2;

/******************************************************************************
 * EEPROM CONFIGURATION
 *
 * EEPROM (Electrically Erasable Programmable Read-Only Memory):
 * Non-volatile storage that persists after power-off. Arduino Uno has 1KB.
 *
 * IMPORTANT CHARACTERISTICS:
 * - Slow writes (~3.3ms per byte) 
 * - Limited write cycles (~100,000 per byte) 
 * - Contents are RANDOM/GARBAGE on first power-up
 *
 * EEPROM_HIGH_SCORE_ADDR (0):
 * Starting address for high score storage. We use 4 bytes:
 *   Byte 0: Score low byte (bits 0-7)
 *   Byte 1: Score high byte (bits 8-15)
 *   Byte 2: Magic byte (0xA5) - indicates data was written
 *   Byte 3: Checksum (XOR of bytes 0-2) - detects corruption
 *
 * EEPROM_MAGIC_BYTE (0xA5):
 * Special marker value. If byte 2 ≠ 0xA5, EEPROM was never initialised.
 * We chose 0xA5 (10100101 binary) because it has alternating bits - unlikely
 * to occur randomly in uninitialised memory.
 *
 * See hardware.cpp:eeprom_read_high_score() for validation logic.
 ******************************************************************************/

const uint16_t EEPROM_HIGH_SCORE_ADDR = 0;
const uint8_t EEPROM_MAGIC_BYTE = 0xA5;

/******************************************************************************
 * GAME STATE ENUM
 *
 * The game uses a state machine, at any moment, it's in exactly one state.
 * Each state handles input differently:
 *
 * STATE_ATTRACT: Demo mode, waiting for player to press button
 * STATE_PLAYING: Active gameplay, chase LED bouncing, button hits counted
 * STATE_RESULT: Brief pause after successful hit, then resume playing
 * STATE_CELEBRATION: New high score achieved! Play animation, then return to attract
 * STATE_GAME_OVER: Missed the target. Play sad animation, then return to attract
 *
 ******************************************************************************/

enum GameState {
    STATE_ATTRACT,      // Demo mode, show high score, wait for button
    STATE_PLAYING,      // Active gameplay
    STATE_RESULT,       // Brief pause after successful hit
    STATE_CELEBRATION,  // New high score animation
    STATE_GAME_OVER     // Miss animation, then return to attract
};

#endif // CONFIG_H
