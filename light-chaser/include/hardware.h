#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>

// Initialise all hardware pins
void hardware_init(void);

// LED control
void led_set(uint8_t position, bool state);
void led_clear_all(void);

// Button control with edge detection and debouncing
bool button_just_pressed(void);
void button_clear_state(void);  // Clear button state for clean state transitions

// Sound effects
void buzzer_tick(void);
void buzzer_hit(void);
void buzzer_bullseye(void);
void buzzer_celebration(void);
void buzzer_game_over(void);

// LCD display control
void display_show_attract(uint16_t high_score);
void display_show_game(uint16_t score, uint16_t high_score);
void display_show_celebration(uint16_t score);
void display_clear(void);

// Celebration effects
void led_celebration(void);
void led_game_over(void);

// EEPROM high score storage
uint16_t eeprom_read_high_score(void);
void eeprom_write_high_score(uint16_t score);

#endif // HARDWARE_H
