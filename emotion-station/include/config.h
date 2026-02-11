#ifndef CONFIG_H
#define CONFIG_H

#ifndef UNIT_TEST
#include <Arduino.h>
#endif

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

// ========= ENUMS (Forward declarations for Phase 2.5 Event Bus) =========

// Mood categories (fully implemented in Phase 3)
typedef enum {
    MOOD_HAPPY = 0,
    MOOD_SAD,
    MOOD_CALM,
    MOOD_ENERGETIC,
    MOOD_ANXIOUS,
    MOOD_ANGRY,
    NUM_MOODS
} MoodCategory;

// Error codes (fully implemented in Phase 10)
typedef enum {
    ERROR_NONE = 0,
    ERROR_NFC_TIMEOUT,
    ERROR_NFC_READ_FAILED,
    ERROR_INVALID_UID,
    ERROR_AUDIO_INIT_FAILED,
    ERROR_AUDIO_PLAYBACK_FAILED,
    ERROR_SD_CARD_MISSING,
    ERROR_ACTIVITY_NOT_FOUND,
    ERROR_BATTERY_CRITICAL,
    ERROR_HARDWARE_FAULT,
    ERROR_UNKNOWN,
    NUM_ERROR_CODES
} ErrorCode;

#endif
