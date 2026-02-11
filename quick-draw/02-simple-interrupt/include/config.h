#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// Pin Configuration
// ============================================================================

// Button pin (using pin 2 for INT0 hardware interrupt)
// Arduino Uno only supports external interrupts on pins 2 (INT0) and 3 (INT1)
#define BUTTON_PIN 2

// LED pins (8 LEDs on pins 3-10)
#define LED_PIN_START 3
#define LED_COUNT 8  // All 8 LEDs available (button moved to pin 2)

// Buzzer pin
#define BUZZER_PIN 11

// I2C LCD (default SDA=A4, SCL=A5)
// No pin definitions needed - handled by Wire library

// ============================================================================
// Game Configuration
// ============================================================================

// Timing thresholds (now using microseconds for interrupt precision)
#define REACTION_MIN_US 100000   // Minimum valid reaction time (100ms in microseconds)
#define REACTION_MAX_US 1000000  // Maximum valid reaction time (1000ms in microseconds)

// Display thresholds (still in milliseconds)
#define REACTION_MIN_MS 100   // For display and feedback logic
#define REACTION_MAX_MS 1000

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
