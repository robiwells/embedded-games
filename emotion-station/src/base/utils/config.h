#ifndef CONFIG_H
#define CONFIG_H

#ifndef UNIT_TEST
#include <Arduino.h>
#else
#include <stdint.h>
#include <stdbool.h>
#endif

// =============================================================================
// GPIO Pin Assignment Table
// =============================================================================
// Pin  | Function             | Notes
// -----+----------------------+------------------------------------------
//   2  | STATUS_LED_PIN       | Boot-strapping pin — must be H/float at boot
//   4  | SD_CS_PIN            | SPI chip-select for SD card
//   5  | LED_DATA_PIN         | NeoPixel data (WS2812B)
//  18  | SD_SCK_PIN           | SPI clock for SD card
//  19  | SD_MISO_PIN          | SPI MISO for SD card
//  21  | I2C_SDA_PIN          | I2C data (NFC PN532)
//  22  | I2C_SCL_PIN          | I2C clock (NFC PN532)
//  23  | SD_MOSI_PIN          | SPI MOSI for SD card
//  25  | I2S_BCLK_PIN         | I2S bit clock (audio DAC)
//  26  | I2S_LRC_PIN          | I2S left/right clock (audio DAC)
//  27  | I2S_DOUT_PIN         | I2S data out (audio DAC)
//  34  | BATTERY_ADC_PIN      | ADC input (input-only pin — no pull-up)
// =============================================================================

// GPIO Pin Assignments
#define LED_DATA_PIN 5
#define STATUS_LED_PIN 2  // Boot strapping pin - must be high/floating during boot

// LED Configuration
#define NUM_LEDS 16
#define LED_BRIGHTNESS_ACTIVE 255      // 100% brightness during activities
#define LED_BRIGHTNESS_IDLE 128        // 50% brightness during idle
#define LED_BRIGHTNESS_LOW_BATTERY 64  // 25% brightness when battery low

// Battery Monitoring
#define BATTERY_ADC_PIN                34
#define BATTERY_VOLTAGE_DIVIDER_RATIO  2.0f   // 2× 10kΩ voltage divider
#define BATTERY_LOW_THRESHOLD          3.4f   // V — trigger LOW_BATTERY state
#define BATTERY_CRITICAL_THRESHOLD     3.3f   // V — deep sleep warning
#define BATTERY_RECOVERY_THRESHOLD     3.5f   // V — return to IDLE from LOW_BATTERY
#define BATTERY_CHECK_INTERVAL_MS      10000  // ms — check every 10 s in IDLE

// Timing Constants
#define WATCHDOG_TIMEOUT_MS 4000

// State Machine Timeouts
#define STATE_NFC_DETECTED_TIMEOUT_MS 1000
#define STATE_COMPLETE_TIMEOUT_MS     2000
#define STATE_ERROR_TIMEOUT_MS        5000

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

// I2S Pin Assignments (Audio)
#define I2S_BCLK_PIN  25
#define I2S_LRC_PIN   26
#define I2S_DOUT_PIN  27
#define AUDIO_VOLUME  18   // 0-21 scale (18 = comfortable listening level)

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

// Game states (system-wide type — used by event_bus, led_controller, and game)
typedef enum {
    STATE_IDLE = 0,              // Waiting for NFC tag
    STATE_NFC_DETECTED,          // NFC tag detected, reading UID
    STATE_VALIDATING,            // Validating NFC UID
    STATE_SELECTING,             // Choosing appropriate activity
    STATE_PLAYING_ACTIVITY,      // Activity in progress
    STATE_ACTIVITY_COMPLETE,     // Success celebration
    STATE_ERROR,                 // Error handling state
    STATE_LOW_BATTERY,           // Critical battery mode
    NUM_STATES                   // Array sizing constant
} GameState;

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

typedef struct {
    uint32_t timestamp;      // Seconds since boot
    MoodCategory mood;
    uint8_t activity_id;
    char activity_name[ACTIVITY_NAME_LENGTH];
    uint16_t duration_seconds;
    bool completed;
    TimeOfDay time_of_day;
} SessionLog;

// Error codes
typedef enum {
    ERROR_NONE = 0,
    ERROR_SD_INIT_FAILED,
    ERROR_SD_READ_FAILED,
    ERROR_NFC_INIT_FAILED,
    ERROR_NFC_READ_TIMEOUT,
    ERROR_AUDIO_INIT_FAILED,
    ERROR_AUDIO_FILE_NOT_FOUND,
    ERROR_JSON_PARSE_FAILED,
    ERROR_INVALID_UID,
    ERROR_NO_ACTIVITIES,
    ERROR_BATTERY_CRITICAL,
    ERROR_WATCHDOG_RESET
} ErrorCode;

// Debug configuration (set to 0 for production build)
#define DEBUG_SERIAL 1

#if DEBUG_SERIAL
    #define DEBUG_PRINT(x)   Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
#endif

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
