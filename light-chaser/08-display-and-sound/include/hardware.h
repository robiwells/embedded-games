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
void buzzer_bullseye(void);
void buzzer_game_over(void);

void display_show_attract(uint16_t high_score);
void display_show_game(uint16_t score, uint16_t high_score);
void display_clear(void);

#endif
