#include "hardware.h"
#include "config.h"
#include <LiquidCrystal_I2C.h>

// ============================================================================
// Hardware Objects
// ============================================================================

static LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);

// ============================================================================
// Button State (Polling-based)
// ============================================================================

static bool last_button_state = !BUTTON_ACTIVE;  // Released initially

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

// ============================================================================
// Button Input (Polling-based)
// ============================================================================

bool button_just_pressed(void) {
    // Read current button state
    bool current_state = (digitalRead(BUTTON_PIN) == BUTTON_ACTIVE);

    // Detect falling edge (transition from not pressed to pressed)
    bool just_pressed = (current_state && !last_button_state);

    // Update state for next call
    last_button_state = current_state;

    return just_pressed;
}

void button_clear_state(void) {
    // Reset edge detection state
    last_button_state = (digitalRead(BUTTON_PIN) == BUTTON_ACTIVE);
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

void display_show_attract(void) {
    display_show("Quick Draw!", "Press to start");
}

void display_show_ready(void) {
    display_show("Get Ready...", "");
}

void display_show_result(uint32_t reaction_ms, const char* feedback) {
    char line1[17];
    char line2[17];

    // Format reaction time with jitter visible (polling precision ~20ms)
    snprintf(line1, sizeof(line1), "%lums", reaction_ms);
    snprintf(line2, sizeof(line2), "%s", feedback);

    display_show(line1, line2);
}

void display_show_timeout(void) {
    display_show("Too slow!", "Try again");
}

// ============================================================================
// Buzzer Output
// ============================================================================

void buzzer_beep(void) {
    tone(BUZZER_PIN, 1000, 100);  // 1kHz for 100ms
}

void buzzer_error(void) {
    tone(BUZZER_PIN, 200, 200);   // Low tone for errors
}

void buzzer_success(void) {
    tone(BUZZER_PIN, 2000, 150);  // High tone for success
}
