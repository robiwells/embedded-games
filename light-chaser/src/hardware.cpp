#include "hardware.h"
#include "config.h"
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// Button state tracking for edge detection
static bool last_button_state = false;
static uint32_t last_debounce_time = 0;

// LCD object
static LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

void hardware_init(void) {
    // Initialise LED pins as outputs
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        pinMode(LED_PIN_START + i, OUTPUT);
        digitalWrite(LED_PIN_START + i, LOW);
    }

    // Initialise button with internal pull-up (active-low)
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    last_button_state = digitalRead(BUTTON_PIN);

    // Initialise buzzer
    pinMode(BUZZER_PIN, OUTPUT);
    noTone(BUZZER_PIN);

    // Initialise LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
}

void led_set(uint8_t position, bool state) {
    // Bounds checking
    if (position >= NUM_LEDS) {
        return;
    }

    digitalWrite(LED_PIN_START + position, state ? HIGH : LOW);
}

void led_clear_all(void) {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        digitalWrite(LED_PIN_START + i, LOW);
    }
}

bool button_just_pressed(void) {
    uint32_t now = millis();

    // Read button state (active-low, so invert)
    bool current_state = !digitalRead(BUTTON_PIN);

    // Detect rising edge (button press)
    bool pressed = false;
    if (current_state && !last_button_state) {
        // Check debounce timeout
        if (now - last_debounce_time >= DEBOUNCE_MS) {
            pressed = true;
            last_debounce_time = now;
        }
    }

    last_button_state = current_state;
    return pressed;
}

void button_clear_state(void) {
    // Reset button state to current physical state
    // This prevents stale edge detections from being processed
    last_button_state = !digitalRead(BUTTON_PIN);
    last_debounce_time = millis();
}

void buzzer_tick(void) {
    tone(BUZZER_PIN, FREQ_TICK, DURATION_TICK);
}

void buzzer_hit(void) {
    tone(BUZZER_PIN, FREQ_HIT, DURATION_HIT);
}

void buzzer_bullseye(void) {
    tone(BUZZER_PIN, FREQ_BULLSEYE_1, DURATION_BULLSEYE_NOTE);
    delay(DURATION_BULLSEYE_NOTE);
    tone(BUZZER_PIN, FREQ_BULLSEYE_2, DURATION_BULLSEYE_NOTE);
    delay(DURATION_BULLSEYE_NOTE);
    tone(BUZZER_PIN, FREQ_BULLSEYE_3, DURATION_BULLSEYE_NOTE);
}

void buzzer_celebration(void) {
    // Ascending 5-note fanfare: C5, E5, G5, C6, E6
    const uint16_t freqs[] = {523, 659, 784, 1047, 1319};
    const uint16_t durations[] = {150, 150, 150, 150, 300};

    for (uint8_t i = 0; i < 5; i++) {
        tone(BUZZER_PIN, freqs[i], durations[i]);
        delay(durations[i] + 50);  // Small gap between notes
    }
}

void buzzer_game_over(void) {
    // Descending 3-note "sad trombone": 400Hz, 300Hz, 200Hz
    tone(BUZZER_PIN, FREQ_GAME_OVER_1, DURATION_GAME_OVER_NOTE);
    delay(DURATION_GAME_OVER_NOTE);
    tone(BUZZER_PIN, FREQ_GAME_OVER_2, DURATION_GAME_OVER_NOTE);
    delay(DURATION_GAME_OVER_NOTE);
    tone(BUZZER_PIN, FREQ_GAME_OVER_3, DURATION_GAME_OVER_NOTE);
}

void led_celebration(void) {
    // LED wave pattern: sweep left-to-right 3 times
    for (uint8_t sweep = 0; sweep < CELEBRATION_SWEEPS; sweep++) {
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            led_set(i, true);
            delay(CELEBRATION_LED_DELAY);
            led_set(i, false);
        }
    }
    led_clear_all();
}

void led_game_over(void) {
    // Flash all LEDs twice
    for (uint8_t flash = 0; flash < 2; flash++) {
        // Turn all on
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            led_set(i, true);
        }
        delay(100);

        // Turn all off
        led_clear_all();
        delay(100);
    }
}

void display_show_attract(uint16_t high_score) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Press to Play!");
    lcd.setCursor(0, 1);
    lcd.print("HiScore: ");
    lcd.print(high_score);
}

void display_show_game(uint16_t score, uint16_t high_score) {
    // Only update the numbers, not the labels (reduces flicker)
    lcd.setCursor(0, 0);
    lcd.print("Score:   ");
    lcd.print(score);
    lcd.print("    ");  // Clear trailing digits
    lcd.setCursor(0, 1);
    lcd.print("HiScore: ");
    lcd.print(high_score);
    lcd.print("    ");  // Clear trailing digits
}

void display_show_celebration(uint16_t score) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NEW HIGH SCORE!");
    lcd.setCursor(0, 1);
    lcd.print("Score: ");
    lcd.print(score);
}

void display_clear(void) {
    lcd.clear();
}

uint16_t eeprom_read_high_score(void) {
    uint8_t low_byte = EEPROM.read(EEPROM_HIGH_SCORE_ADDR);
    uint8_t high_byte = EEPROM.read(EEPROM_HIGH_SCORE_ADDR + 1);
    uint8_t magic = EEPROM.read(EEPROM_HIGH_SCORE_ADDR + 2);
    uint8_t checksum = EEPROM.read(EEPROM_HIGH_SCORE_ADDR + 3);

    // Validate magic byte
    if (magic != EEPROM_MAGIC_BYTE) {
        return 0;  // Uninitialised EEPROM
    }

    // Validate checksum
    uint8_t expected_checksum = low_byte ^ high_byte ^ magic;
    if (checksum != expected_checksum) {
        return 0;  // Corrupted data
    }

    // Reconstruct 16-bit score
    return (uint16_t)low_byte | ((uint16_t)high_byte << 8);
}

void eeprom_write_high_score(uint16_t score) {
    uint8_t low_byte = score & 0xFF;
    uint8_t high_byte = (score >> 8) & 0xFF;
    uint8_t checksum = low_byte ^ high_byte ^ EEPROM_MAGIC_BYTE;

    // Use update() instead of write() to minimise EEPROM wear
    EEPROM.update(EEPROM_HIGH_SCORE_ADDR, low_byte);
    EEPROM.update(EEPROM_HIGH_SCORE_ADDR + 1, high_byte);
    EEPROM.update(EEPROM_HIGH_SCORE_ADDR + 2, EEPROM_MAGIC_BYTE);
    EEPROM.update(EEPROM_HIGH_SCORE_ADDR + 3, checksum);
}
