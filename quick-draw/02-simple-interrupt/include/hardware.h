#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>

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
// Button Input (Polling-based)
// ============================================================================

// Check if button was just pressed (edge detection)
// Returns true once per press, false otherwise
// NOTE: This is polling-based with ~20ms precision
bool button_just_pressed(void);

// Clear button state (used when transitioning between game states)
void button_clear_state(void);

// ============================================================================
// LED Output
// ============================================================================

// Set specific LED on (index 0-6)
void led_set(uint8_t index);

// Clear all LEDs
void led_clear_all(void);

// ============================================================================
// Display Output
// ============================================================================

// Clear display
void display_clear(void);

// Show two-line message
void display_show(const char* line1, const char* line2);

// Show attract screen
void display_show_attract(void);

// Show "Get Ready..." message
void display_show_ready(void);

// Show reaction time result
void display_show_result(uint32_t reaction_ms, const char* feedback);

// Show timeout message
void display_show_timeout(void);

// ============================================================================
// Buzzer Output
// ============================================================================

// Play short beep
void buzzer_beep(void);

// Play error tone
void buzzer_error(void);

// Play success tone
void buzzer_success(void);

#endif // HARDWARE_H
