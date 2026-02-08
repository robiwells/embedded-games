/******************************************************************************
 * HARDWARE.H - Hardware Abstraction Layer (HAL) Interface
 *
 * This file defines the PUBLIC INTERFACE to all hardware. Notice what's
 * missing: no mention of specific pins, I2C addresses, or implementation
 * details. Game logic code calls these functions without knowing how they
 * work internally.
 *
 * WHAT IS A HARDWARE ABSTRACTION LAYER (HAL)?
 *
 * A HAL is an interface that hides hardware implementation details from
 * higher-level code. It's a clean separation:
 *
 * ┌──────────────┐
 * │  game.cpp    │  High-level game logic (scoring, state machine)
 * │              │  Calls: led_set(), button_just_pressed(), etc.
 * └──────┬───────┘
 *        │
 *        │  HAL Interface (this file - hardware.h)
 *        │  Pure function declarations, no implementation
 *        │
 * ┌──────▼───────┐
 * │ hardware.cpp │  Implementation (digitalWrite, I2C, EEPROM, etc.)
 * └──────┬───────┘
 *        │
 * ┌──────▼───────┐
 * │  Physical    │  Actual hardware: GPIO pins, I2C bus, EEPROM memory
 * │  Hardware    │
 * └──────────────┘
 *
 * BENEFITS OF HARDWARE ABSTRACTION:
 *
 * 1. **Portability**: Switching from GPIO LEDs to shift register LEDs?
 *    Only hardware.cpp changes. game.cpp unchanged.
 *
 * 2. **Testability**: Can create mock implementations for desktop testing:
 *    void led_set(uint8_t pos, bool state) { printf("LED %d = %d\n", pos, state); }
 *
 * 3. **Readability**: game.cpp reads like natural language:
 *    if (button_just_pressed()) { led_set(0, true); }
 *    vs.
 *    if (!digitalRead(10) && debounce_check()) { digitalWrite(2, HIGH); }
 *
 * 4. **Maintainability**: Hardware changes are localised. Pin number change?
 *    Edit config.h and hardware.cpp. game.cpp never needs to know.
 *
 * 5. **Clear API**: This file serves as documentation of all hardware
 *    capabilities. New developers read this to see what's available.
 *
 ******************************************************************************/

#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>

/******************************************************************************
 * INITIALISATION
 *
 * hardware_init - One-time hardware setup
 *
 * Called from main.cpp:setup() before any other hardware functions.
 * Configures:
 * - LED pins as OUTPUT (8 pins)
 * - Button pin as INPUT_PULLUP (active-low with internal pull-up resistor)
 * - Buzzer pin as OUTPUT
 * - I2C LCD display (initialisation, backlight on)
 *
 * EMBEDDED CONCEPT: Pin Configuration
 * Unlike desktop I/O (always ready), embedded pins must be configured:
 * - pinMode(pin, OUTPUT): Pin can source/sink current (for LEDs, buzzer)
 * - pinMode(pin, INPUT): High-impedance, reads voltage (for sensors)
 * - pinMode(pin, INPUT_PULLUP): Reads voltage + internal 20kΩ pull-up resistor
 *
 * After calling hardware_init():
 * - All LEDs are OFF (LOW)
 * - Button is ready to read
 * - LCD displays blank screen with backlight
 * - No sounds playing
 ******************************************************************************/

void hardware_init(void);

/******************************************************************************
 * LED CONTROL - Simple GPIO Output
 *
 * HARDWARE SETUP:
 * 8 LEDs connected to Arduino pins 2-9 through current-limiting resistors.
 * Physical layout: [0][1][2][3][4][5][6][7]
 *                  Red Red Red Grn Grn Red Red Red
 *
 * led_set - Turn a single LED on or off
 * @param position: LED index (0-7)
 * @param state: true = ON (HIGH/5V), false = OFF (LOW/0V)
 *
 * led_clear_all - Turn off all LEDs
 * Bulk operation, equivalent to calling led_set(i, false) for all LEDs.
 * Used during state transitions and animations.
 *
 * EXAMPLE USAGE:
 *   led_set(3, true);   // Turn on LED 3 (first green LED)
 *   led_set(3, false);  // Turn off LED 3
 *   led_clear_all();    // Turn off all LEDs at once
 ******************************************************************************/

void led_set(uint8_t position, bool state);
void led_clear_all(void);

/******************************************************************************
 * BUTTON INPUT - Edge Detection with Debouncing
 *
 * HARDWARE SETUP:
 * Tactile button wired between Arduino pin 10 and GND.
 * Arduino pin configured as INPUT_PULLUP (pulled to 5V by internal resistor).
 * When button pressed: pin connects to GND → reads LOW.
 * When button released: pin pulled to 5V → reads HIGH.
 *
 * button_just_pressed - Detect button PRESS event (not button STATE)
 * @return: true if button was just pressed (rising edge), false otherwise
 *
 * EMBEDDED CONCEPT: Edge Detection vs Level Detection
 *
 * LEVEL detection (what NOT to do):
 *   if (digitalRead(BUTTON_PIN) == LOW) {
 *       score++;  // BUG! Increments every loop iteration (1000s/second!)
 *   }
 *
 * EDGE detection (what we implement):
 *   if (button_just_pressed()) {
 *       score++;  // CORRECT! Increments once per button press
 *   }
 *
 * Edge detection tracks TRANSITIONS:
 * - Rising edge: unpressed (HIGH) → pressed (LOW)
 * - Falling edge: pressed (LOW) → unpressed (HIGH)
 *
 * We detect rising edges (button presses) and ignore falling edges.
 *
 * DEBOUNCING:
 * Physical buttons "bounce," contacts make/break rapidly (~5-20ms) before
 * settling. Without debouncing, one press = multiple detected transitions.
 *
 * Our implementation enforces 50ms lockout between detected presses.
 * See hardware.cpp:button_just_pressed() for detailed implementation.
 *
 * button_clear_state - Reset edge detector to current physical state
 *
 * Called during state transitions to prevent "stale" button presses.
 *
 * Example problem without clearing:
 * 1. User presses button in STATE_ATTRACT (starts game)
 * 2. Transition to STATE_PLAYING
 * 3. User still holding button from step 1
 * 4. STATE_PLAYING sees old press, immediately registers hit (unintended!)
 *
 * Solution: Call button_clear_state() in state exit functions.
 ******************************************************************************/

bool button_just_pressed(void);
void button_clear_state(void);

/******************************************************************************
 * SOUND EFFECTS - PWM Tone Generation
 *
 * HARDWARE SETUP:
 * Piezo buzzer connected to pin 11 (PWM-capable pin).
 * Arduino's tone() function generates square waves at specified frequencies.
 *
 * buzzer_tick - Play brief tick sound (100 Hz, 20ms)
 * Used for each LED movement in chase animation.
 *
 * buzzer_hit - Play hit confirmation sound (500 Hz, 100ms)
 * Used for non-bullseye successful hits.
 *
 * EMBEDDED CONCEPT: PWM (Pulse Width Modulation)
 * Arduino generates audio by rapidly toggling pin HIGH/LOW at audio frequencies.
 * Example: 1000 Hz tone = pin toggles HIGH/LOW 1000 times per second.
 * Buzzer membrane vibrates at this frequency → audible tone.
 *
 * NON-BLOCKING BEHAVIOUR:
 * Arduino's tone() returns IMMEDIATELY. The tone plays in the background
 * using hardware timers. No delay() needed!
 *
 *   tone(pin, 500, 100);  // Start 500 Hz tone for 100ms
 *   // Code continues immediately, tone plays independently
 *   led_set(0, true);     // Can do other work while tone plays
 *
 * For multi-note sequences (melodies), see animation system below.
 ******************************************************************************/

void buzzer_tick(void);
void buzzer_hit(void);

/******************************************************************************
 * LCD DISPLAY - I2C Character Display
 *
 * HARDWARE SETUP:
 * 16×2 character LCD connected via I2C (pins A4/A5).
 * I2C = Inter-Integrated Circuit (2-wire serial protocol).
 *
 * I2C CONNECTION:
 *   Arduino A4 (SDA) ──── LCD SDA (bidirectional data)
 *   Arduino A5 (SCL) ──── LCD SCL (clock from master)
 *   Both pins also connected to +5V via pull-up resistors
 *
 * Why I2C? Alternative is parallel mode (requires 6+ pins). I2C saves pins
 * at the cost of slightly slower updates (~1ms vs ~100μs).
 *
 * display_show_attract - Show attract mode screen
 * @param high_score: High score to display
 * Display:
 *   ┌────────────────┐
 *   │Press to Play!  │
 *   │HiScore: 120    │
 *   └────────────────┘
 *
 * display_show_game - Show active game screen
 * @param score: Current game score
 * @param high_score: High score
 * Display:
 *   ┌────────────────┐
 *   │Score:   45     │
 *   │HiScore: 120    │
 *   └────────────────┘
 *
 * display_show_celebration - Show new high score screen
 * @param score: New high score value
 * Display:
 *   ┌────────────────┐
 *   │NEW HIGH SCORE! │
 *   │Score: 150      │
 *   └────────────────┘
 *
 * display_clear - Clear display (blank screen, backlight remains on)
 *
 * PERFORMANCE NOTE:
 * I2C communication is relatively slow (~100 kHz clock = 10μs per bit).
 * Sending 32 characters (full 16×2 screen) takes ~3-4ms.
 * This is why display_show_game() only updates numbers, not labels.
 ******************************************************************************/

void display_show_attract(uint16_t high_score);
void display_show_game(uint16_t score, uint16_t high_score);
void display_show_celebration(uint16_t score);
void display_clear(void);

/******************************************************************************
 * NON-BLOCKING ANIMATION SYSTEM
 *
 * PROBLEM SOLVED:
 * How do you play multi-step animations (melodies, LED sequences) without
 * using delay() which freezes the entire program?
 *
 * SOLUTION: State Machine + Cooperative Multitasking
 *
 * WHAT IS COOPERATIVE MULTITASKING?
 *
 * Arduino Uno has NO operating system, NO threads. Everything runs in a single
 * execution thread. To do multiple things "at once", we use cooperative
 * multitasking: each "task" runs a little bit, then yields control.
 *
 * BLOCKING (bad):
 *   void play_melody() {
 *       tone(pin, 500, 100);
 *       delay(100);  // ❌ ENTIRE PROGRAM FROZEN for 100ms
 *       tone(pin, 600, 100);
 *       delay(100);  // ❌ Can't check button, update LEDs, reset watchdog!
 *       tone(pin, 700, 100);
 *   }
 *
 * NON-BLOCKING (good):
 *   void animation_update() {
 *       if (time_for_next_note()) {
 *           play_next_note();  // ✅ Returns immediately
 *       }
 *       // Returns control to caller (main loop)
 *   }
 *
 * ANIMATION INTERFACE:
 *
 * animation_update - Advance animation state (call every loop iteration)
 * @return: true if animation finished this frame, false if still playing
 *
 * This function MUST be called every loop() iteration. It checks if enough
 * time has elapsed to advance to the next animation step, then returns
 * immediately. Main loop remains responsive.
 *
 * animation_start_bullseye - Start 3-note ascending melody
 * Plays when player hits bullseye zone (green LEDs).
 * Notes: 800 Hz → 1000 Hz → 1200 Hz (100ms each)
 *
 * animation_start_celebration - Start new high score celebration
 * Parallel animation:
 * - Buzzer: 5-note melody (C-E-G-C-E, 150-300ms each)
 * - LEDs: Wave effect sweeps left-to-right 3 times (40ms per LED)
 * Demonstrates PARALLEL TIMING - two animations with independent timers.
 *
 * animation_start_game_over - Start game over animation
 * Parallel animation:
 * - Buzzer: 3-note descending "sad trombone" (400→300→200 Hz, 200ms each)
 * - LEDs: All flash on/off 5 times (150ms per state)
 *
 * animation_is_playing - Check if any animation is active
 * @return: true if animation playing, false if idle
 *
 * Used by game state machine to detect animation completion:
 *   if (!animation_is_playing()) {
 *       game_transition_to(STATE_ATTRACT);  // Animation done, change state
 *   }
 *
 * IMPLEMENTATION PREVIEW:
 * See hardware.cpp lines 84-246 for full implementation using:
 * - AnimationState enum (IDLE, BULLSEYE, CELEBRATION, GAME_OVER)
 * - millis() timestamps for timing
 * - Step counters for sequence position
 * - Parallel timing variables for simultaneous buzzer + LED effects
 ******************************************************************************/

bool animation_update(void);
void animation_start_bullseye(void);
void animation_start_celebration(void);
void animation_start_game_over(void);
bool animation_is_playing(void);

/******************************************************************************
 * EEPROM PERSISTENT STORAGE
 *
 * EEPROM (Electrically Erasable Programmable Read-Only Memory):
 * Non-volatile memory that survives power loss. Arduino Uno has 1KB (1024 bytes).
 *
 * CRITICAL CHARACTERISTICS:
 * - NON-VOLATILE: Data persists when power is removed
 * - SLOW WRITES: ~3.3ms per byte (1000× slower than RAM)
 * - LIMITED ENDURANCE: ~100,000 write cycles per byte
 * - UNINITIALISED: Contains random garbage on first power-up
 *
 * EEPROM vs RAM vs FLASH:
 * ┌──────────┬──────────┬───────────────┬─────────────┬─────────────┐
 * │ Type     │ Size     │ Speed         │ Persistence │ Endurance   │
 * ├──────────┼──────────┼───────────────┼─────────────┼─────────────┤
 * │ RAM      │ 2 KB     │ Fast (62.5ns) │ Lost on RST │ Unlimited   │
 * │ FLASH    │ 32 KB    │ Fast (read)   │ Permanent   │ 10K writes  │
 * │ EEPROM   │ 1 KB     │ Slow (3.3ms)  │ Permanent   │ 100K writes │
 * └──────────┴──────────┴───────────────┴─────────────┴─────────────┘
 *
 * USAGE PATTERN:
 * - Store: Rarely-changing data (high scores, settings, calibration)
 * - Avoid: Frequently-changing data (game state, sensor readings)
 *
 * eeprom_read_high_score - Load high score from EEPROM
 * @return: High score value, or 0 if EEPROM uninitialised/corrupted
 *
 * DATA VALIDATION:
 * EEPROM may contain garbage (first boot) or corrupted data (power loss during
 * write). We use a validation scheme:
 *
 * Storage format (4 bytes at address 0):
 *   Byte 0: Score low byte (bits 0-7)
 *   Byte 1: Score high byte (bits 8-15)
 *   Byte 2: Magic byte (0xA5) - "data valid" marker
 *   Byte 3: Checksum (low ^ high ^ magic) - corruption detection
 *
 * Read process:
 * 1. Read all 4 bytes
 * 2. Check byte 2 == 0xA5 (if not, EEPROM never initialised → return 0)
 * 3. Calculate checksum: byte0 ^ byte1 ^ byte2
 * 4. Compare to byte 3 (if mismatch, data corrupted → return 0)
 * 5. Reconstruct score: byte0 | (byte1 << 8)
 *
 * eeprom_write_high_score - Save high score to EEPROM
 * @param score: Score value to save (0-65535)
 *
 * Write process:
 * 1. Split score into low/high bytes
 * 2. Calculate checksum: low ^ high ^ 0xA5
 * 3. Write all 4 bytes using EEPROM.update() (only writes if changed)
 *
 * WHY EEPROM.update() INSTEAD OF EEPROM.write()?
 * update() only writes if the byte value differs from current EEPROM value.
 * This preserves write endurance. If high score is already 100, writing 100
 * again does nothing (0 wear vs 1 write cycle).
 *
 * See hardware.cpp:eeprom_read_high_score() and eeprom_write_high_score()
 * for complete implementation with detailed validation logic.
 ******************************************************************************/

uint16_t eeprom_read_high_score(void);
void eeprom_write_high_score(uint16_t score);

#endif // HARDWARE_H
