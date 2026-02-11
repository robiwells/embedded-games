#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// GPIO Pin Assignments
#define LED_DATA_PIN 5
#define STATUS_LED_PIN 2  // Boot strapping pin - must be high/floating during boot

// LED Configuration
#define NUM_LEDS 16
#define LED_BRIGHTNESS_ACTIVE 255      // 100% brightness during activities
#define LED_BRIGHTNESS_IDLE 128        // 50% brightness during idle
#define LED_BRIGHTNESS_LOW_BATTERY 64  // 25% brightness when battery low

// Timing Constants
#define WATCHDOG_TIMEOUT_MS 4000

#endif
