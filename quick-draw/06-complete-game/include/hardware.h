#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>
#include "game.h"

// ============================================================================
// Hardware Abstraction Layer
// ============================================================================
// This layer provides a clean interface between game logic and physical
// hardware. Game code should NEVER directly access pins or hardware.

// ============================================================================
// Initialisation
// ============================================================================

// Initialise all hardware (call once in setup())
void hardware_init(void);

// ============================================================================
// Button Input (Interrupt-based)
// ============================================================================

// Initialise button interrupt (call once in setup())
void button_init_interrupt(void);

// Check if button was pressed (checks ISR flag)
// Returns true if button was pressed since last check
bool button_was_pressed(void);

// Get timestamp of button press (in microseconds)
uint32_t button_get_press_time(void);

// Get game state at time of button press
GameState button_get_state_at_press(void);

// Clear button state (used when transitioning between game states)
void button_clear_state(void);

// ============================================================================
// LED Output
// ============================================================================

// Set specific LED on (index 0-7)
void led_set(uint8_t index);

// Clear all LEDs
void led_clear_all(void);

// Set distractor LED (one of the LEDs before the draw LED)
void led_set_distractor(uint8_t index);

// ============================================================================
// Display Output
// ============================================================================

// Clear display
void display_clear(void);

// Show two-line message
void display_show(const char* line1, const char* line2);

// Show attract screen with best time
void display_show_attract(uint16_t best_time_ms);

// Show "Get Ready..." message
void display_show_ready(void);

// Show reaction time result with round info
void display_show_result(uint32_t reaction_us, const char* feedback,
                         uint8_t round, uint16_t avg_ms);

// Show timeout message
void display_show_timeout(void);

// Show false start message
void display_show_false_start(void);

// Show round complete screen
void display_show_round_complete(uint8_t round, uint16_t avg_ms);

// Show game complete screen
void display_show_game_complete(uint16_t best_ms, uint16_t avg_ms);

// Show new record celebration
void display_show_new_record(uint16_t new_record_ms);

// ============================================================================
// Buzzer Output
// ============================================================================

// Play short beep
void buzzer_beep(void);

// Play error tone (false start)
void buzzer_error(void);

// Play success tone (good reaction)
void buzzer_success(void);

// Play celebration tune (new record)
void buzzer_celebration(void);

// ============================================================================
// EEPROM Persistence
// ============================================================================

// Load best time from EEPROM (returns 0 if invalid/not set)
uint16_t eeprom_load_best_time(void);

// Save best time to EEPROM
void eeprom_save_best_time(uint16_t time_ms);

#endif // HARDWARE_H
