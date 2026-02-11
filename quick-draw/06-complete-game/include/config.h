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
#define LED_COUNT 8

// Buzzer pin
#define BUZZER_PIN 11

// I2C LCD (default SDA=A4, SCL=A5)
// No pin definitions needed - handled by Wire library

// ============================================================================
// Game Configuration
// ============================================================================

// Game settings
#define TOTAL_ROUNDS 5  // Number of rounds per game

// Timing thresholds (microseconds for interrupt precision)
#define REACTION_MIN_US 100000    // Minimum valid reaction time (100ms)
#define REACTION_MAX_US 1000000   // Maximum valid reaction time (1000ms)

// Debounce timing
#define DEBOUNCE_US 50000  // 50ms debounce in microseconds

// Feedback thresholds (milliseconds for display)
#define REACTION_LIGHTNING_MS 300   // "Lightning!" feedback
#define REACTION_QUICK_MS 500       // "Quick!" feedback
#define REACTION_OK_MS 800          // "OK" feedback
// Above 800ms = "Slow"

// Timeout constants (milliseconds for timeout comparisons)
#define REACTION_MIN_MS 100   // Minimum valid reaction time
#define REACTION_MAX_MS 1000  // Maximum valid reaction time (timeout)

// Ready state timing
#define READY_MIN_MS 2000  // Minimum countdown time
#define READY_MAX_MS 4000  // Maximum countdown time

// Distractor LED timing
#define DISTRACTOR_MIN_MS 500   // Minimum time before first distractor
#define DISTRACTOR_MAX_MS 1500  // Maximum time before distractor
#define DISTRACTOR_DURATION 100 // How long distractor LED stays on

// Display durations
#define RESULT_DISPLAY_MS 2000      // Show result for 2 seconds
#define ROUND_PAUSE_MS 1000         // Pause between rounds
#define FALSE_START_DISPLAY_MS 2000 // Show false start message
#define NEW_RECORD_DISPLAY_MS 3000  // Show new record celebration

// ============================================================================
// EEPROM Configuration
// ============================================================================

#define EEPROM_MAGIC_BYTE 0xA5  // Magic byte to verify valid data
#define EEPROM_ADDR_MAGIC 0     // Address of magic byte
#define EEPROM_ADDR_BEST_HI 1   // Best time high byte
#define EEPROM_ADDR_BEST_LO 2   // Best time low byte
#define EEPROM_ADDR_CHECKSUM 3  // Checksum byte

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
