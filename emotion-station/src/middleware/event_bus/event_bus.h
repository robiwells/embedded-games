/**
 * @file event_bus.h
 * @brief Lightweight event bus for decoupled module communication
 *
 * Phase 2.5: Event Bus Implementation
 * Architecture: Circular queue with publish/subscribe pattern
 * Memory: 406 bytes SRAM (176 bytes queue + 176 bytes subscribers + 54 bytes overhead)
 * Processing: <200µs per iteration (architecture requirement)
 */

#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#ifndef UNIT_TEST
#include <Arduino.h>
#endif

#include "config.h"

// ========= EVENT TYPE ENUMERATION (12 types as per architecture.md Section 3.5) =========

typedef enum {
    // Hardware events (5)
    NFC_DETECTED,
    NFC_REMOVED,
    NFC_TOKEN_PRESENT,   // Token stable after debounce (no UID yet)
    AUDIO_COMPLETE,
    BATTERY_LOW,

    // State events (2)
    STATE_ENTERED,
    STATE_EXITED,

    // Application events (3)
    ACTIVITY_SELECTED,
    SESSION_STARTED,
    SESSION_COMPLETED,

    // System events (2)
    ERROR_OCCURRED,
    BOOT_COMPLETE,

    NUM_EVENT_TYPES
} EventType;

// ========= PRIORITY LEVELS (4 levels) =========

typedef enum {
    PRIORITY_CRITICAL = 0,  // System errors, battery critical
    PRIORITY_HIGH,          // State transitions, NFC events
    PRIORITY_NORMAL,        // Activity events, audio complete
    PRIORITY_LOW            // Logging, diagnostics
} EventPriority;

// ========= EVENT STRUCTURE (22 bytes total) =========

typedef struct {
    EventType type;           // 1 byte
    EventPriority priority;   // 1 byte
    uint32_t timestamp;       // 4 bytes (millis() when published)

    // Payload union (16 bytes) - largest member determines size
    union {
        // NFC detected event (8 bytes)
        struct {
            uint8_t uid[7];       // 7-byte NFC UID
            MoodCategory mood;    // Detected mood
        } nfc_detected;

        // State transition event (2 bytes)
        struct {
            GameState old_state;
            GameState new_state;
        } state_transition;

        // Activity selection event (2 bytes)
        struct {
            uint8_t activity_id;
            MoodCategory mood;
        } activity;

        // Error event (1 byte)
        struct {
            ErrorCode error_code;
        } error;

        // Generic payload (16 bytes max)
        uint8_t raw[16];
    } payload;
} Event;  // Total: 22 bytes (1 + 1 + 4 + 16)

// ========= SUBSCRIBER CALLBACK FUNCTION TYPE =========

typedef void (*EventCallback)(const Event* event);

// ========= PUBLIC API FUNCTIONS =========

/**
 * @brief Initialise event bus
 *
 * Clears circular queue and subscriber table.
 * Call once during setup().
 */
void event_bus_init();

/**
 * @brief Publish event to bus
 * @param type Event type (routing key)
 * @param priority Event priority level
 * @param payload_data Pointer to event payload (or NULL)
 * @param payload_size Size of payload in bytes (max 16)
 *
 * Enqueues event to circular buffer. If queue is full, drops oldest event.
 * Timestamp automatically set to millis() at publish time.
 */
void event_bus_publish(EventType type, EventPriority priority, const void* payload_data, uint8_t payload_size);

/**
 * @brief Subscribe to event type
 * @param type Event type to subscribe to
 * @param callback Function to call when event received
 *
 * Registers callback for specified event type.
 * Max 4 subscribers per event type.
 */
void event_bus_subscribe(EventType type, EventCallback callback);

/**
 * @brief Process all queued events
 *
 * Dispatches all events in queue to subscribers (synchronous).
 * Call every loop iteration (after wdt_reset(), before game_update()).
 * Processing time logged and compared to 200µs threshold.
 */
void event_bus_process();

// ========= HELPER MACROS FOR COMMON EVENTS =========

/**
 * @brief Publish NFC detected event
 * @param uid 7-byte NFC UID
 * @param mood Detected mood category
 */
#define PUBLISH_NFC_DETECTED(uid, mood) \
    do { \
        struct { uint8_t uid[7]; MoodCategory mood; } data = {0}; \
        memcpy(data.uid, uid, 7); \
        data.mood = mood; \
        event_bus_publish(NFC_DETECTED, PRIORITY_HIGH, &data, sizeof(data)); \
    } while(0)

/**
 * @brief Publish state transition event
 * @param old Old game state
 * @param new New game state
 */
#define PUBLISH_STATE_TRANSITION(old, new) \
    do { \
        struct { GameState old_state; GameState new_state; } data = {old, new}; \
        event_bus_publish(STATE_ENTERED, PRIORITY_HIGH, &data, sizeof(data)); \
    } while(0)

/**
 * @brief Publish audio complete event
 */
#define PUBLISH_AUDIO_COMPLETE() \
    event_bus_publish(AUDIO_COMPLETE, PRIORITY_NORMAL, NULL, 0)

/**
 * @brief Publish battery low event
 */
#define PUBLISH_BATTERY_LOW() \
    event_bus_publish(BATTERY_LOW, PRIORITY_CRITICAL, NULL, 0)

/**
 * @brief Publish error event
 * @param code Error code
 */
#define PUBLISH_ERROR(code) \
    do { \
        struct { ErrorCode error_code; } data = {code}; \
        event_bus_publish(ERROR_OCCURRED, PRIORITY_CRITICAL, &data, sizeof(data)); \
    } while(0)

#endif // EVENT_BUS_H
