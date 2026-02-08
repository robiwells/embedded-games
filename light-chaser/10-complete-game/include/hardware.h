#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>

void hardware_init(void);

void led_set(uint8_t position, bool state);
void led_clear_all(void);

bool button_just_pressed(void);
void button_clear_state(void);

void buzzer_tick(void);
void buzzer_hit(void);

bool animation_update(void);
void animation_start_bullseye(void);
void animation_start_celebration(void);
void animation_start_game_over(void);
bool animation_is_playing(void);

void display_show_attract(uint16_t high_score);
void display_show_game(uint16_t score, uint16_t high_score);
void display_show_celebration(uint16_t score);
void display_clear(void);

uint16_t eeprom_read_high_score(void);
void eeprom_write_high_score(uint16_t score);

#endif
