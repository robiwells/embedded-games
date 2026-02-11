#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// Pin Configuration
// ============================================================================

// Button pin (using pin 10 for polling - NO interrupt capability)
#define BUTTON_PIN 10

// LED pins (8 LEDs on pins 3-10, but pin 10 used for button)
#define LED_PIN_START 3
#define LED_COUNT 7  // Only 7 LEDs available (pin 10 used for button)

// Buzzer pin
#define BUZZER_PIN 11

// I2C LCD (default SDA=A4, SCL=A5)
// No pin definitions needed - handled by Wire library

// ============================================================================
// Game Configuration
// ============================================================================

// Timing thresholds
#define REACTION_MIN_MS 100   // Minimum valid reaction time (anti-cheat)
#define REACTION_MAX_MS 1000  // Maximum valid reaction time (timeout)

// Feedback thresholds (milliseconds)
#define REACTION_LIGHTNING_MS 300   // "Lightning!" feedback
#define REACTION_QUICK_MS 500       // "Quick!" feedback
#define REACTION_OK_MS 800          // "OK" feedback
// Above 800ms = "Slow"

// Ready state timing
#define READY_MIN_MS 2000  // Minimum countdown time
#define READY_MAX_MS 4000  // Maximum countdown time

// Display durations
#define RESULT_DISPLAY_MS 2000  // Show result for 2 seconds

// ============================================================================
// Hardware Configuration
// ============================================================================

// LCD I2C address (common addresses: 0x27, 0x3F)
#define LCD_I2C_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// Button configuration
#define BUTTON_ACTIVE LOW  // Button pulls to ground when pressed

#endif // CONFIG_H
