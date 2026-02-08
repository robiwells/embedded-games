#include "hardware.h"
#include "config.h"
#include <LiquidCrystal_I2C.h>

static bool last_button_state = false;
static uint32_t last_debounce_time = 0;

static LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

void hardware_init(void) {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        pinMode(LED_PIN_START + i, OUTPUT);
        digitalWrite(LED_PIN_START + i, LOW);
    }

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    last_button_state = digitalRead(BUTTON_PIN);

    pinMode(BUZZER_PIN, OUTPUT);
    noTone(BUZZER_PIN);

    lcd.init();
    lcd.backlight();
    lcd.clear();
}

void led_set(uint8_t position, bool state) {
    if (position >= NUM_LEDS) {
        return;
    }

    uint8_t pin = LED_PIN_START + position;

    digitalWrite(pin, state ? HIGH : LOW);
}

void led_clear_all(void) {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        digitalWrite(LED_PIN_START + i, LOW);
    }
}

bool button_just_pressed(void) {
    uint32_t now = millis();

    bool current_state = !digitalRead(BUTTON_PIN);

    bool pressed = false;
    if (current_state && !last_button_state) {
        if (now - last_debounce_time >= DEBOUNCE_MS) {
            pressed = true;
            last_debounce_time = now;
        }
    }

    last_button_state = current_state;

    return pressed;
}

void button_clear_state(void) {
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

void buzzer_game_over(void) {
    tone(BUZZER_PIN, FREQ_GAME_OVER_1, DURATION_GAME_OVER_NOTE);
    delay(DURATION_GAME_OVER_NOTE);
    tone(BUZZER_PIN, FREQ_GAME_OVER_2, DURATION_GAME_OVER_NOTE);
    delay(DURATION_GAME_OVER_NOTE);
    tone(BUZZER_PIN, FREQ_GAME_OVER_3, DURATION_GAME_OVER_NOTE);
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
    lcd.setCursor(0, 0);
    lcd.print("Score:   ");
    lcd.print(score);
    lcd.print("    ");

    lcd.setCursor(0, 1);
    lcd.print("HiScore: ");
    lcd.print(high_score);
    lcd.print("    ");
}

void display_clear(void) {
    lcd.clear();
}
