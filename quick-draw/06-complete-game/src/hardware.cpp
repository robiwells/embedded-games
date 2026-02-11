#include "hardware.h"
#include "config.h"
#include "game.h"
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// ============================================================================
// Hardware Objects
// ============================================================================

static LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);

// ============================================================================
// Button Interrupt State
// ============================================================================
// CRITICAL: All variables accessed by ISR MUST be declared volatile
// The 'volatile' keyword tells the compiler that these variables can change
// at any time (from the ISR), preventing incorrect optimisations.

// Flag set by ISR, checked by main loop
volatile bool button_pressed_flag = false;

// Timestamp captured by ISR (microseconds for precision)
volatile uint32_t button_press_time_us = 0;

// Game state at moment of button press (for validation)
volatile GameState state_at_press = STATE_ATTRACT;

// Last interrupt time for debouncing
volatile uint32_t last_interrupt_time_us = 0;

// ============================================================================
// Button Interrupt Service Routine (ISR)
// ============================================================================
// CRITICAL ISR CONSTRAINTS:
// 1. Keep execution time < 10µs (ideally < 5µs)
// 2. NO delay(), Serial.print(), lcd.print(), or any blocking calls
// 3. NO complex logic - just capture data and set flags
// 4. Only access volatile variables
// 5. Return immediately
//
// ISR PATTERN:
// - ISR captures moment-in-time data (timestamp, state)
// - Sets flag to signal main loop
// - Main loop processes the event with full context

void button_isr(void) {
    uint32_t current_time = micros();

    // Software debouncing: Ignore bounces within 50ms of last interrupt
    if ((current_time - last_interrupt_time_us) > DEBOUNCE_US) {
        last_interrupt_time_us = current_time;
        button_press_time_us = current_time;
        state_at_press = game_get_state();  // Capture state at press moment
        button_pressed_flag = true;
    }
    // Bounces are silently ignored
}

// ============================================================================
// Initialisation
// ============================================================================

void hardware_init(void) {
    // Initialise button pin
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Initialise LED pins
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        pinMode(LED_PIN_START + i, OUTPUT);
        digitalWrite(LED_PIN_START + i, LOW);
    }

    // Initialise buzzer pin
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    // Initialise LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
}

void button_init_interrupt(void) {
    // Attach ISR to INT0 (pin 2) on FALLING edge (button press)
    // FALLING = transition from HIGH to LOW (button pulls pin to ground)
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_isr, FALLING);
}

// ============================================================================
// Button Input (Interrupt-based)
// ============================================================================

bool button_was_pressed(void) {
    // Check and clear flag atomically
    bool was_pressed = button_pressed_flag;
    if (was_pressed) {
        button_pressed_flag = false;
    }
    return was_pressed;
}

uint32_t button_get_press_time(void) {
    return button_press_time_us;
}

GameState button_get_state_at_press(void) {
    return state_at_press;
}

void button_clear_state(void) {
    // Reset all interrupt state
    button_pressed_flag = false;
    button_press_time_us = 0;
    state_at_press = game_get_state();
}

// ============================================================================
// LED Output
// ============================================================================

void led_set(uint8_t index) {
    if (index < LED_COUNT) {
        digitalWrite(LED_PIN_START + index, HIGH);
    }
}

void led_clear_all(void) {
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        digitalWrite(LED_PIN_START + i, LOW);
    }
}

void led_set_distractor(uint8_t index) {
    // Use one of the LEDs before the draw LED (middle one)
    // Draw LED is at index 4 (middle of 0-7), so use 0-3 for distractors
    if (index < 4) {
        led_set(index);
    }
}

// ============================================================================
// Display Output
// ============================================================================

void display_clear(void) {
    lcd.clear();
}

void display_show(const char* line1, const char* line2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
}

void display_show_attract(uint16_t best_time_ms) {
    char line1[17] = "Quick Draw!";
    char line2[17];

    if (best_time_ms > 0) {
        snprintf(line2, sizeof(line2), "Best: %ums", best_time_ms);
    } else {
        snprintf(line2, sizeof(line2), "Press to start");
    }

    display_show(line1, line2);
}

void display_show_ready(void) {
    display_show("Get Ready...", "");
}

void display_show_result(uint32_t reaction_us, const char* feedback,
                         uint8_t round, uint16_t avg_ms) {
    char line1[17];
    char line2[17];

    // Convert microseconds to milliseconds for display
    uint16_t reaction_ms = reaction_us / 1000;

    snprintf(line1, sizeof(line1), "%ums - %s", reaction_ms, feedback);
    snprintf(line2, sizeof(line2), "Rd %u/5 Avg:%ums", round, avg_ms);

    display_show(line1, line2);
}

void display_show_timeout(void) {
    display_show("Too slow!", "Try faster!");
}

void display_show_false_start(void) {
    display_show("False Start!", "Wait for LED!");
}

void display_show_round_complete(uint8_t round, uint16_t avg_ms) {
    char line1[17];
    char line2[17];

    snprintf(line1, sizeof(line1), "Round %u/5", round);
    snprintf(line2, sizeof(line2), "Average: %ums", avg_ms);

    display_show(line1, line2);
}

void display_show_game_complete(uint16_t best_ms, uint16_t avg_ms) {
    char line1[17];
    char line2[17];

    snprintf(line1, sizeof(line1), "Game Over!");
    snprintf(line2, sizeof(line2), "B:%u A:%u", best_ms, avg_ms);

    display_show(line1, line2);
}

void display_show_new_record(uint16_t new_record_ms) {
    char line1[17] = "NEW RECORD!";
    char line2[17];

    snprintf(line2, sizeof(line2), "%ums!", new_record_ms);

    display_show(line1, line2);
}

// ============================================================================
// Buzzer Output
// ============================================================================

void buzzer_beep(void) {
    tone(BUZZER_PIN, 1000, 100);  // 1kHz for 100ms
}

void buzzer_error(void) {
    // Descending tones for false start
    tone(BUZZER_PIN, 400, 100);
    delay(120);
    tone(BUZZER_PIN, 200, 200);
}

void buzzer_success(void) {
    tone(BUZZER_PIN, 2000, 150);  // High tone for good reaction
}

void buzzer_celebration(void) {
    // Ascending melody for new record
    tone(BUZZER_PIN, 1000, 100);
    delay(120);
    tone(BUZZER_PIN, 1500, 100);
    delay(120);
    tone(BUZZER_PIN, 2000, 200);
}

// ============================================================================
// EEPROM Persistence
// ============================================================================

uint16_t eeprom_load_best_time(void) {
    // Check magic byte
    if (EEPROM.read(EEPROM_ADDR_MAGIC) != EEPROM_MAGIC_BYTE) {
        return 0;  // No valid data
    }

    // Read best time (16-bit value stored as high/low bytes)
    uint8_t high_byte = EEPROM.read(EEPROM_ADDR_BEST_HI);
    uint8_t low_byte = EEPROM.read(EEPROM_ADDR_BEST_LO);
    uint16_t best_time = (high_byte << 8) | low_byte;

    // Verify checksum
    uint8_t stored_checksum = EEPROM.read(EEPROM_ADDR_CHECKSUM);
    uint8_t calculated_checksum = (uint8_t)(EEPROM_MAGIC_BYTE + high_byte + low_byte);

    if (stored_checksum != calculated_checksum) {
        return 0;  // Corrupted data
    }

    return best_time;
}

void eeprom_save_best_time(uint16_t time_ms) {
    // Split 16-bit value into high/low bytes
    uint8_t high_byte = (time_ms >> 8) & 0xFF;
    uint8_t low_byte = time_ms & 0xFF;

    // Calculate checksum
    uint8_t checksum = (uint8_t)(EEPROM_MAGIC_BYTE + high_byte + low_byte);

    // Write to EEPROM
    EEPROM.write(EEPROM_ADDR_MAGIC, EEPROM_MAGIC_BYTE);
    EEPROM.write(EEPROM_ADDR_BEST_HI, high_byte);
    EEPROM.write(EEPROM_ADDR_BEST_LO, low_byte);
    EEPROM.write(EEPROM_ADDR_CHECKSUM, checksum);
}
