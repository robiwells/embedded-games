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

// SPI Pin Assignments (SD Card)
#define SD_SCK_PIN   18
#define SD_MISO_PIN  19
#define SD_MOSI_PIN  23
#define SD_CS_PIN     4

// Activity Configuration
#define MAX_ACTIVITIES        50
#define ACTIVITY_NAME_LENGTH  32
#define ACTIVITY_PATH_LENGTH  64
#define ACTIVITY_TYPE_LENGTH  16

// I2C Pin Assignments (NFC)
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define PN532_I2C_ADDRESS 0x24

// NFC Retry Configuration
#define NFC_READ_ATTEMPTS 3
#define NFC_RETRY_DELAY_MS 200
#define NFC_DEBOUNCE_TIME_MS 100
#define NFC_READ_TIMEOUT_MS 1000

// ========= ENUMS (Forward declarations for Phase 2.5 Event Bus) =========

// Mood categories (fully implemented in Phase 3)
typedef enum {
    MOOD_HAPPY = 0,
    MOOD_SAD,
    MOOD_CALM,
    MOOD_ENERGETIC,
    MOOD_ANXIOUS,
    MOOD_ANGRY,
    NUM_MOODS,
    MOOD_UNKNOWN = 255
} MoodCategory;

typedef enum {
    TIME_MORNING = 0,    // 06:00–11:59
    TIME_AFTERNOON,      // 12:00–16:59
    TIME_EVENING,        // 17:00–20:59
    TIME_BEDTIME         // 21:00–05:59
} TimeOfDay;

typedef struct {
    uint8_t  id;
    MoodCategory mood;
    char     name[ACTIVITY_NAME_LENGTH];
    char     file_path[ACTIVITY_PATH_LENGTH];
    uint16_t duration_seconds;
    bool     time_flags[4];
    char     type[ACTIVITY_TYPE_LENGTH];
} Activity;

typedef struct {
    uint8_t uid[7];
    MoodCategory mood;
    const char* display_name;
} NfcMoodMapping;

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

// Fallback audio paths (used if activities.json fails to load)
// Indexed by MoodCategory enum — must match order exactly
#define FALLBACK_AUDIO_COUNT NUM_MOODS
static const char* const FALLBACK_AUDIO_PATHS[NUM_MOODS] = {
    "/audio/happy/25_sunshine_dance.mp3",    // MOOD_HAPPY
    "/audio/sad/17_rainbow_breath.mp3",       // MOOD_SAD
    "/audio/calm/41_slow_breathing.mp3",      // MOOD_CALM
    "/audio/energetic/movement_1.mp3",        // MOOD_ENERGETIC (placeholder — no content defined yet)
    "/audio/anxious/9_bubble_breath.mp3",     // MOOD_ANXIOUS
    "/audio/angry/1_dragon_breath.mp3",       // MOOD_ANGRY
};

#endif
