# Emotion Check-In Station - Implementation Plan

## Overview

This implementation plan breaks down the Emotion Check-In Station into incremental, testable phases. Each phase produces working, demonstrable functionality that can be validated before proceeding to the next phase.

**Architecture Reference:** See `architecture.md` v2.4 for complete system specifications.

**Testing Strategy:** Each phase is designed to be testable in the Wokwi simulator where possible, with clear success criteria and verification steps.

**Implementation Structure:**
- **11 Core Phases:** Complete all essential functionality
  - Phase 2.5: Event Bus Implementation (core architectural component)
- **4 Sub-Phases:** Production enhancements and validation
  - Phase 6.5: Real-Time Clock Integration
  - Phase 8.5: Activity History Persistence
  - Phase 9.5: Battery Calibration & Validation
  - Phase 10 Addendum: Watchdog Timing & Edge Cases
- **Total:** 15 phases for production-ready system

**Coverage vs Architecture v2.4:**
- ✅ 100% direct implementation (all critical functionality)
- ✅ All 8 game states, 6 moods, 11 error types, 11 event types
- ✅ Event bus fully implemented in v1.0
- ⚠️ 4 documented deviations (see Architecture Deviations section)

---

## Phase 1: Hardware Abstraction Layer & LED Control

### Objective
Establish the foundational HAL and implement the WS2812B LED ring with non-blocking animations and power-saving brightness control.

### Components
- ESP32 DevKit
- WS2812B LED Ring (16 LEDs)

### Files to Create
```
include/
  config.h           // Pin definitions, constants, enums
  hardware.h         // HAL interface
  led_controller.h   // LED animation API

src/
  main.cpp          // Setup, main loop, watchdog
  hardware.cpp      // Hardware initialisation
  led_controller.cpp // LED state machine
```

### Implementation Tasks

#### 1.1 Project Setup
**Task:** Create PlatformIO project structure and configure platformio.ini

**Code:**
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

lib_deps =
    adafruit/Adafruit NeoPixel@^1.11.0

build_flags =
    -DCORE_DEBUG_LEVEL=3
    -DCONFIG_BT_ENABLED=0
    -DCONFIG_WIFI_ENABLED=0

monitor_speed = 115200
```

**Success Criteria:**
- ✅ Project compiles without errors
- ✅ ESP32 can be programmed successfully
- ✅ Serial output visible at 115200 baud

#### 1.2 Pin Definitions & Configuration
**Task:** Define all GPIO pins and system constants

**Code (config.h):**
```cpp
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// GPIO Pin Assignments
#define LED_DATA_PIN 5
#define STATUS_LED_PIN 2  // Boot strapping pin - must be high/floating during boot

// LED Configuration
#define NUM_LEDS 16
#define LED_BRIGHTNESS_ACTIVE 255
#define LED_BRIGHTNESS_IDLE 128
#define LED_BRIGHTNESS_LOW_BATTERY 64

// Timing Constants
#define WATCHDOG_TIMEOUT_MS 4000

#endif
```

**Success Criteria:**
- ✅ All pins defined according to architecture.md Section 2.2
- ✅ Constants match specifications
- ✅ Header compiles without warnings

#### 1.3 Basic Hardware Initialisation
**Task:** Implement hardware_init() and watchdog timer

**Code (hardware.cpp):**
```cpp
#include "hardware.h"
#include "config.h"
#include <esp_task_wdt.h>

void hardware_init() {
    Serial.begin(115200);
    Serial.println("\n\n=== Emotion Station Booting ===");

    // Initialise status LED (GPIO2 is a boot strapping pin - only configure AFTER boot completes)
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);

    // Initialise watchdog timer
    esp_task_wdt_init(WATCHDOG_TIMEOUT_MS / 1000, true);
    esp_task_wdt_add(NULL);

    Serial.println("Hardware initialisation complete");
}

void hardware_heartbeat() {
    static uint32_t last_beat = 0;
    if (millis() - last_beat >= 1000) {
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        last_beat = millis();
    }
}
```

**Success Criteria:**
- ✅ Serial output shows boot message
- ✅ Onboard LED blinks every 1 second
- ✅ Watchdog timer armed (4-second timeout)
- ✅ No crashes or resets

#### 1.4 LED Controller State Machine
**Task:** Implement non-blocking LED animations with brightness control

**Code (led_controller.h):**
```cpp
#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <stdint.h>

typedef enum {
    LED_IDLE,           // Slow white pulse
    LED_DETECTED,       // Quick green flash
    LED_BREATHING,      // Mood-specific colour breathing
    LED_SPARKLE,        // Completion sparkle
    LED_ERROR           // Slow red pulse
} LedAnimationState;

typedef enum {
    POWER_MODE_NORMAL,     // Full brightness
    POWER_MODE_ECO,        // 50% brightness
    POWER_MODE_CRITICAL    // 25% brightness
} PowerMode;

void led_init();
void led_update();
void led_set_animation(LedAnimationState animation);
void led_set_brightness(PowerMode mode);
void led_test_sequence();

#endif
```

**Code (led_controller.cpp):**
```cpp
#include "led_controller.h"
#include "config.h"
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel pixels(NUM_LEDS, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);

static LedAnimationState current_animation = LED_IDLE;
static PowerMode current_power_mode = POWER_MODE_ECO;
static uint32_t animation_start = 0;

void led_init() {
    pixels.begin();
    led_set_brightness(POWER_MODE_ECO);
    pixels.clear();
    pixels.show();
    animation_start = millis();
}

void led_set_brightness(PowerMode mode) {
    current_power_mode = mode;
    switch (mode) {
        case POWER_MODE_NORMAL:
            pixels.setBrightness(LED_BRIGHTNESS_ACTIVE);
            break;
        case POWER_MODE_ECO:
            pixels.setBrightness(LED_BRIGHTNESS_IDLE);
            break;
        case POWER_MODE_CRITICAL:
            pixels.setBrightness(LED_BRIGHTNESS_LOW_BATTERY);
            break;
    }
}

void led_set_animation(LedAnimationState animation) {
    current_animation = animation;
    animation_start = millis();
}

void led_update() {
    uint32_t elapsed = millis() - animation_start;

    switch (current_animation) {
        case LED_IDLE: {
            // Slow white pulse (4-second cycle)
            float brightness = (sin((elapsed / 2000.0) * PI) + 1.0) / 2.0;
            uint8_t val = (uint8_t)(brightness * 255);
            for (int i = 0; i < NUM_LEDS; i++) {
                pixels.setPixelColor(i, pixels.Color(val, val, val));
            }
            break;
        }

        case LED_DETECTED: {
            // Quick green flash (500ms total)
            if (elapsed < 250) {
                for (int i = 0; i < NUM_LEDS; i++) {
                    pixels.setPixelColor(i, pixels.Color(0, 255, 0));
                }
            } else {
                for (int i = 0; i < NUM_LEDS; i++) {
                    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
                }
            }
            break;
        }

        case LED_BREATHING: {
            // Breathing blue (2-second cycle)
            float brightness = (sin((elapsed / 1000.0) * PI) + 1.0) / 2.0;
            uint8_t val = (uint8_t)(brightness * 255);
            for (int i = 0; i < NUM_LEDS; i++) {
                pixels.setPixelColor(i, pixels.Color(0, 0, val));
            }
            break;
        }

        case LED_SPARKLE: {
            // Random sparkle (2 seconds)
            if (elapsed < 2000 && elapsed % 100 < 50) {
                int led = random(NUM_LEDS);
                pixels.setPixelColor(led, pixels.Color(255, 255, 255));
            } else {
                pixels.clear();
            }
            break;
        }

        case LED_ERROR: {
            // Slow red pulse (3-second cycle)
            float brightness = (sin((elapsed / 1500.0) * PI) + 1.0) / 2.0;
            uint8_t val = (uint8_t)(brightness * 128); // Dimmer for errors
            for (int i = 0; i < NUM_LEDS; i++) {
                pixels.setPixelColor(i, pixels.Color(val, 0, 0));
            }
            break;
        }
    }

    pixels.show();
}

void led_test_sequence() {
    Serial.println("LED Test: All animations");

    // Test each animation for 3 seconds
    LedAnimationState tests[] = {LED_IDLE, LED_DETECTED, LED_BREATHING, LED_SPARKLE, LED_ERROR};
    const char* names[] = {"IDLE", "DETECTED", "BREATHING", "SPARKLE", "ERROR"};

    for (int i = 0; i < 5; i++) {
        Serial.print("Testing: ");
        Serial.println(names[i]);
        led_set_animation(tests[i]);
        uint32_t start = millis();
        while (millis() - start < 3000) {
            led_update();
            delay(10);
        }
    }

    Serial.println("LED Test complete");
}
```

**Code (main.cpp):**
```cpp
#include <Arduino.h>
#include "config.h"
#include "hardware.h"
#include "led_controller.h"
#include <esp_task_wdt.h>

void setup() {
    hardware_init();
    led_init();

    // Run LED test sequence on boot
    led_test_sequence();

    // Start idle animation
    led_set_animation(LED_IDLE);
    led_set_brightness(POWER_MODE_ECO);
}

void loop() {
    esp_task_wdt_reset();

    hardware_heartbeat();
    led_update();

    // No delay() - fully non-blocking
}
```

**Success Criteria:**
- ✅ All 5 LED animations display correctly
- ✅ Test sequence cycles through all animations
- ✅ Idle animation uses 50% brightness (eco mode)
- ✅ Brightness changes visible when switching power modes
- ✅ No blocking delays, watchdog does not trigger
- ✅ LED animations are smooth (no flickering)

### Wokwi Testing
**Diagram.json:**
```json
{
  "version": 1,
  "author": "Emotion Station",
  "editor": "wokwi",
  "parts": [
    {
      "type": "wokwi-esp32-devkit-v1",
      "id": "esp32",
      "top": 0,
      "left": 0,
      "attrs": {}
    },
    {
      "type": "wokwi-neopixel-ring",
      "id": "ring1",
      "top": -100,
      "left": 200,
      "attrs": { "pixels": "16" }
    }
  ],
  "connections": [
    ["ring1:DIN", "esp32:GPIO5", "green", ["v0"]]
  ]
}
```

**Verification Steps:**
1. Upload code to Wokwi
2. Observe serial output shows boot sequence
3. Watch LED test sequence (5 animations × 3s = 15s)
4. Verify idle animation runs continuously
5. Check onboard LED blinks every second
6. Confirm no watchdog resets in serial monitor

**Phase 1 Complete When:**
- All LED animations working
- Power modes functional (brightness changes)
- Non-blocking architecture verified
- Watchdog timer stable

---

## Phase 2: State Machine Framework

### Objective
Implement the enter/exit/update state machine pattern with centralised transitions, building on Phase 1's LED control.

### Additional Components
None (builds on Phase 1)

### Files to Create/Modify
```
include/
  game.h            // State machine interface

src/
  game.cpp          // State machine implementation
  main.cpp          // [MODIFY] Add game_update()
```

### Implementation Tasks

#### 2.1 State Machine Types & Interface
**Task:** Define GameState enum and StateHandler structure

**Code (game.h):**
```cpp
#ifndef GAME_H
#define GAME_H

#include <stdint.h>

typedef enum {
    STATE_IDLE,
    STATE_NFC_DETECTED,
    STATE_VALIDATING,
    STATE_SELECTING,
    STATE_PLAYING_ACTIVITY,
    STATE_ACTIVITY_COMPLETE,
    STATE_ERROR,
    STATE_LOW_BATTERY,
    NUM_STATES
} GameState;

typedef struct {
    void (*enter)(void);
    void (*update)(void);
    void (*exit)(void);
} StateHandler;

// Public API
void game_init();
void game_update();
void game_transition_to(GameState new_state);
GameState game_get_current_state();

// Test API
void game_test_transitions();

#endif
```

**Success Criteria:**
- ✅ All states from architecture.md defined
- ✅ StateHandler structure matches enter/exit/update pattern
- ✅ Clean public API

#### 2.2 State Handler Implementation
**Task:** Implement all 8 state handlers with enter/update/exit functions

**Code (game.cpp):**
```cpp
#include "game.h"
#include "led_controller.h"
#include "config.h"

static GameState current_state = STATE_IDLE;
static uint32_t state_entry_time = 0;

// Forward declarations
static void idle_enter();
static void idle_update();
static void idle_exit();

static void nfc_detected_enter();
static void nfc_detected_update();
static void nfc_detected_exit();

static void validating_enter();
static void validating_update();
static void validating_exit();

static void selecting_enter();
static void selecting_update();
static void selecting_exit();

static void playing_activity_enter();
static void playing_activity_update();
static void playing_activity_exit();

static void activity_complete_enter();
static void activity_complete_update();
static void activity_complete_exit();

static void error_enter();
static void error_update();
static void error_exit();

static void low_battery_enter();
static void low_battery_update();
static void low_battery_exit();

// State handler table
static const StateHandler state_handlers[NUM_STATES] = {
    {idle_enter,              idle_update,              idle_exit},
    {nfc_detected_enter,      nfc_detected_update,      nfc_detected_exit},
    {validating_enter,        validating_update,        validating_exit},
    {selecting_enter,         selecting_update,         selecting_exit},
    {playing_activity_enter,  playing_activity_update,  playing_activity_exit},
    {activity_complete_enter, activity_complete_update, activity_complete_exit},
    {error_enter,             error_update,             error_exit},
    {low_battery_enter,       low_battery_update,       low_battery_exit}
};

static const char* state_names[NUM_STATES] = {
    "IDLE", "NFC_DETECTED", "VALIDATING", "SELECTING",
    "PLAYING_ACTIVITY", "ACTIVITY_COMPLETE", "ERROR", "LOW_BATTERY"
};

// Centralised state transition
void game_transition_to(GameState new_state) {
    if (new_state >= NUM_STATES) {
        Serial.println("ERROR: Invalid state transition");
        return;
    }

    Serial.print("State: ");
    Serial.print(state_names[current_state]);
    Serial.print(" -> ");
    Serial.println(state_names[new_state]);

    // Exit current state
    state_handlers[current_state].exit();

    // Change state
    current_state = new_state;
    state_entry_time = millis();

    // Enter new state
    state_handlers[current_state].enter();
}

void game_init() {
    Serial.println("Game state machine initialising");
    current_state = STATE_IDLE;
    state_entry_time = millis();
    state_handlers[STATE_IDLE].enter();
}

void game_update() {
    state_handlers[current_state].update();
}

GameState game_get_current_state() {
    return current_state;
}

// ============ STATE_IDLE ============
static void idle_enter() {
    Serial.println("[IDLE] Enter: LED pulsing white (eco mode)");
    led_set_animation(LED_IDLE);
    led_set_brightness(POWER_MODE_ECO);
}

static void idle_update() {
    // Placeholder: Will add NFC detection in Phase 3
    // For now, auto-transition after 5 seconds for testing
    if (millis() - state_entry_time > 5000) {
        game_transition_to(STATE_NFC_DETECTED);
    }
}

static void idle_exit() {
    Serial.println("[IDLE] Exit");
}

// ============ STATE_NFC_DETECTED ============
static void nfc_detected_enter() {
    Serial.println("[NFC_DETECTED] Enter: Green flash");
    led_set_animation(LED_DETECTED);
    led_set_brightness(POWER_MODE_NORMAL);
}

static void nfc_detected_update() {
    // Simulate NFC read delay
    if (millis() - state_entry_time > 1000) {
        game_transition_to(STATE_VALIDATING);
    }
}

static void nfc_detected_exit() {
    Serial.println("[NFC_DETECTED] Exit");
}

// ============ STATE_VALIDATING ============
static void validating_enter() {
    Serial.println("[VALIDATING] Enter: Validating UID");
}

static void validating_update() {
    // Simulate validation
    if (millis() - state_entry_time > 200) {
        game_transition_to(STATE_SELECTING);
    }
}

static void validating_exit() {
    Serial.println("[VALIDATING] Exit");
}

// ============ STATE_SELECTING ============
static void selecting_enter() {
    Serial.println("[SELECTING] Enter: Choosing activity");
}

static void selecting_update() {
    // Simulate selection
    if (millis() - state_entry_time > 500) {
        game_transition_to(STATE_PLAYING_ACTIVITY);
    }
}

static void selecting_exit() {
    Serial.println("[SELECTING] Exit");
}

// ============ STATE_PLAYING_ACTIVITY ============
static void playing_activity_enter() {
    Serial.println("[PLAYING] Enter: Breathing animation, audio playing");
    led_set_animation(LED_BREATHING);
    led_set_brightness(POWER_MODE_NORMAL);
}

static void playing_activity_update() {
    // Simulate 5-second activity
    if (millis() - state_entry_time > 5000) {
        game_transition_to(STATE_ACTIVITY_COMPLETE);
    }
}

static void playing_activity_exit() {
    Serial.println("[PLAYING] Exit");
}

// ============ STATE_ACTIVITY_COMPLETE ============
static void activity_complete_enter() {
    Serial.println("[COMPLETE] Enter: Sparkle animation");
    led_set_animation(LED_SPARKLE);
}

static void activity_complete_update() {
    // 2-second celebration
    if (millis() - state_entry_time > 2000) {
        game_transition_to(STATE_IDLE);
    }
}

static void activity_complete_exit() {
    Serial.println("[COMPLETE] Exit");
}

// ============ STATE_ERROR ============
static void error_enter() {
    Serial.println("[ERROR] Enter: Red pulse");
    led_set_animation(LED_ERROR);
    led_set_brightness(POWER_MODE_ECO);
}

static void error_update() {
    // 5-second error display
    if (millis() - state_entry_time > 5000) {
        game_transition_to(STATE_IDLE);
    }
}

static void error_exit() {
    Serial.println("[ERROR] Exit");
}

// ============ STATE_LOW_BATTERY ============
static void low_battery_enter() {
    Serial.println("[LOW_BATTERY] Enter: Amber pulse");
    led_set_animation(LED_ERROR); // Reuse red, will make amber later
    led_set_brightness(POWER_MODE_CRITICAL);
}

static void low_battery_update() {
    // Stay here until "charged"
}

static void low_battery_exit() {
    Serial.println("[LOW_BATTERY] Exit");
}

// ============ TEST FUNCTION ============
void game_test_transitions() {
    Serial.println("\n=== STATE MACHINE TEST ===");
    Serial.println("Testing all state transitions...\n");

    // Test happy path: IDLE -> NFC -> VALIDATE -> SELECT -> PLAY -> COMPLETE -> IDLE
    game_transition_to(STATE_IDLE);
    delay(1000);

    game_transition_to(STATE_NFC_DETECTED);
    delay(1000);

    game_transition_to(STATE_VALIDATING);
    delay(500);

    game_transition_to(STATE_SELECTING);
    delay(500);

    game_transition_to(STATE_PLAYING_ACTIVITY);
    delay(2000);

    game_transition_to(STATE_ACTIVITY_COMPLETE);
    delay(2000);

    game_transition_to(STATE_IDLE);
    delay(1000);

    // Test error path
    game_transition_to(STATE_ERROR);
    delay(3000);

    game_transition_to(STATE_IDLE);

    Serial.println("\n=== TEST COMPLETE ===\n");
}
```

**Code (main.cpp - updated):**
```cpp
#include <Arduino.h>
#include "config.h"
#include "hardware.h"
#include "led_controller.h"
#include "game.h"
#include <esp_task_wdt.h>

void setup() {
    hardware_init();
    led_init();
    game_init();

    Serial.println("\nPress 't' to run state machine test");
}

void loop() {
    esp_task_wdt_reset();

    hardware_heartbeat();
    led_update();
    game_update();

    // Test trigger from serial input
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 't') {
            game_test_transitions();
        }
    }
}
```

**Success Criteria:**
- ✅ All 8 states implement enter/update/exit lifecycle
- ✅ Centralised transition logs state changes
- ✅ State handler table correctly indexed
- ✅ Test function exercises all states
- ✅ LED animations change with state transitions
- ✅ Brightness control works per state (eco vs normal)
- ✅ No crashes during rapid state transitions

### Wokwi Testing
Use same diagram from Phase 1.

**Verification Steps:**
1. Upload code to Wokwi
2. Observe automatic state progression (5s idle → transition sequence)
3. Type 't' in serial monitor to trigger manual test
4. Verify each state:
   - Logs entry/exit messages
   - Changes LED animation appropriately
   - Transitions to next state correctly
5. Confirm LED brightness changes (idle=eco, playing=normal)
6. Check no watchdog resets during transitions

**Phase 2 Complete When:**
- All states functional with enter/exit/update lifecycle
- State transitions work via game_transition_to()
- LED animations synchronised with states
- Manual test command works
- Serial logging clear and informative

---

## Phase 2.5: Event Bus Implementation

### Objective
Implement lightweight event bus architecture for decoupled module communication as specified in architecture.md v2.4 Section 3.5.

### Prerequisites
Phase 2 (State Machine Framework) must be complete.

### Components
- Event type enumeration (11 types)
- Circular queue (8 events, 176 bytes)
- Publisher/subscriber pattern
- Synchronous dispatch in main loop

### Files to Create
```
include/
  event_bus.h          // Event types, structures, API

src/
  event_bus.cpp        // Circular queue and dispatch logic
```

### Files to Modify
```
src/
  main.cpp             // Add event_bus_process() to main loop
  game.cpp             // Publish STATE_ENTERED/STATE_EXITED events
```

### Implementation Tasks

#### 2.5.1 Event Type Definitions

**Task:** Define all 11 event types and priority levels

**Code (event_bus.h):**
```cpp
#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include <Arduino.h>
#include "config.h"

// Event type enumeration (11 types as per architecture.md Section 3.5)
typedef enum {
    // Hardware events (4)
    NFC_DETECTED,
    NFC_REMOVED,
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

// Priority levels (4 levels)
typedef enum {
    PRIORITY_CRITICAL = 0,  // System errors, battery critical
    PRIORITY_HIGH,          // State transitions, NFC events
    PRIORITY_NORMAL,        // Activity events, audio complete
    PRIORITY_LOW            // Logging, diagnostics
} EventPriority;

// Event structure (22 bytes total)
typedef struct {
    EventType type;           // 1 byte
    EventPriority priority;   // 1 byte
    uint32_t timestamp;       // 4 bytes (millis() when published)

    // Payload union (16 bytes)
    union {
        struct {
            uint8_t uid[7];
            MoodCategory mood;
        } nfc_detected;       // 8 bytes

        struct {
            GameState old_state;
            GameState new_state;
        } state_transition;   // 2 bytes

        struct {
            uint8_t activity_id;
            MoodCategory mood;
        } activity;           // 2 bytes

        struct {
            ErrorCode error_code;
        } error;              // 1 byte

        uint8_t raw[16];      // Generic payload
    } payload;
} Event;  // Total: 22 bytes

// Subscriber callback function type
typedef void (*EventCallback)(const Event* event);

// API Functions
void event_bus_init();
void event_bus_publish(EventType type, EventPriority priority, const void* payload_data, uint8_t payload_size);
void event_bus_subscribe(EventType type, EventCallback callback);
void event_bus_process();  // Call every loop iteration

// Helper macros for common event publishing
#define PUBLISH_NFC_DETECTED(uid, mood) \
    do { \
        struct { uint8_t uid[7]; MoodCategory mood; } data = {0}; \
        memcpy(data.uid, uid, 7); \
        data.mood = mood; \
        event_bus_publish(NFC_DETECTED, PRIORITY_HIGH, &data, sizeof(data)); \
    } while(0)

#define PUBLISH_STATE_TRANSITION(old, new) \
    do { \
        struct { GameState old_state; GameState new_state; } data = {old, new}; \
        event_bus_publish(STATE_ENTERED, PRIORITY_HIGH, &data, sizeof(data)); \
    } while(0)

#define PUBLISH_AUDIO_COMPLETE() \
    event_bus_publish(AUDIO_COMPLETE, PRIORITY_NORMAL, NULL, 0)

#define PUBLISH_BATTERY_LOW() \
    event_bus_publish(BATTERY_LOW, PRIORITY_CRITICAL, NULL, 0)

#define PUBLISH_ERROR(code) \
    do { \
        struct { ErrorCode error_code; } data = {code}; \
        event_bus_publish(ERROR_OCCURRED, PRIORITY_CRITICAL, &data, sizeof(data)); \
    } while(0)

#endif
```

**Success Criteria:**
- ✅ All 11 event types defined
- ✅ Event structure exactly 22 bytes
- ✅ Helper macros for common events

#### 2.5.2 Circular Queue Implementation

**Task:** Implement circular event queue (8 events, 176 bytes)

**Code (event_bus.cpp):**
```cpp
#include "event_bus.h"

// Circular queue configuration
#define EVENT_QUEUE_SIZE 8

// Event queue storage (8 events × 22 bytes = 176 bytes)
static Event event_queue[EVENT_QUEUE_SIZE];
static uint8_t queue_head = 0;  // Write position
static uint8_t queue_tail = 0;  // Read position
static uint8_t queue_count = 0; // Number of events in queue

// Subscriber table (11 event types × 4 subscribers max = 44 pointers = 176 bytes on ESP32)
#define MAX_SUBSCRIBERS_PER_EVENT 4
static EventCallback subscribers[NUM_EVENT_TYPES][MAX_SUBSCRIBERS_PER_EVENT];
static uint8_t subscriber_counts[NUM_EVENT_TYPES];

void event_bus_init() {
    Serial.println("EventBus: Initialising...");

    // Clear queue
    queue_head = 0;
    queue_tail = 0;
    queue_count = 0;

    // Clear subscriber table
    for (uint8_t i = 0; i < NUM_EVENT_TYPES; i++) {
        subscriber_counts[i] = 0;
        for (uint8_t j = 0; j < MAX_SUBSCRIBERS_PER_EVENT; j++) {
            subscribers[i][j] = NULL;
        }
    }

    Serial.println("EventBus: Ready (queue size: 8 events, 176 bytes)");
}

void event_bus_publish(EventType type, EventPriority priority, const void* payload_data, uint8_t payload_size) {
    if (type >= NUM_EVENT_TYPES) {
        Serial.println("EventBus: ERROR - Invalid event type");
        return;
    }

    // Check if queue is full
    if (queue_count >= EVENT_QUEUE_SIZE) {
        Serial.println("EventBus: WARNING - Queue full, dropping oldest event");
        // Drop oldest event (tail)
        queue_tail = (queue_tail + 1) % EVENT_QUEUE_SIZE;
        queue_count--;
    }

    // Create new event
    Event* event = &event_queue[queue_head];
    event->type = type;
    event->priority = priority;
    event->timestamp = millis();

    // Copy payload if provided
    if (payload_data && payload_size > 0) {
        uint8_t copy_size = payload_size > 16 ? 16 : payload_size;
        memcpy(event->payload.raw, payload_data, copy_size);
    } else {
        memset(event->payload.raw, 0, 16);
    }

    // Advance head
    queue_head = (queue_head + 1) % EVENT_QUEUE_SIZE;
    queue_count++;

    Serial.print("EventBus: Published event type ");
    Serial.print(type);
    Serial.print(" (priority ");
    Serial.print(priority);
    Serial.print(", queue: ");
    Serial.print(queue_count);
    Serial.println("/8)");
}

void event_bus_subscribe(EventType type, EventCallback callback) {
    if (type >= NUM_EVENT_TYPES) {
        Serial.println("EventBus: ERROR - Invalid event type for subscription");
        return;
    }

    if (callback == NULL) {
        Serial.println("EventBus: ERROR - NULL callback");
        return;
    }

    // Check if subscriber limit reached
    if (subscriber_counts[type] >= MAX_SUBSCRIBERS_PER_EVENT) {
        Serial.println("EventBus: ERROR - Max subscribers reached for event type");
        return;
    }

    // Add subscriber
    subscribers[type][subscriber_counts[type]] = callback;
    subscriber_counts[type]++;

    Serial.print("EventBus: Subscribed to event type ");
    Serial.print(type);
    Serial.print(" (");
    Serial.print(subscriber_counts[type]);
    Serial.println(" subscribers)");
}

void event_bus_process() {
    // Process all events in queue (synchronous dispatch)
    uint32_t process_start = micros();
    uint8_t events_processed = 0;

    while (queue_count > 0) {
        // Get event from tail
        Event* event = &event_queue[queue_tail];

        // Dispatch to all subscribers
        uint8_t subscriber_count = subscriber_counts[event->type];
        for (uint8_t i = 0; i < subscriber_count; i++) {
            if (subscribers[event->type][i] != NULL) {
                subscribers[event->type][i](event);
            }
        }

        // Remove event from queue
        queue_tail = (queue_tail + 1) % EVENT_QUEUE_SIZE;
        queue_count--;
        events_processed++;
    }

    // Timing analysis (should be <200µs per architecture spec)
    if (events_processed > 0) {
        uint32_t process_time = micros() - process_start;
        Serial.print("EventBus: Processed ");
        Serial.print(events_processed);
        Serial.print(" events in ");
        Serial.print(process_time);
        Serial.println(" µs");

        if (process_time > 200) {
            Serial.println("EventBus: WARNING - Processing time exceeded 200µs threshold!");
        }
    }
}
```

**Success Criteria:**
- ✅ Circular queue holds 8 events
- ✅ Queue full handling (drops oldest)
- ✅ Publish/subscribe pattern works
- ✅ Processing time <200µs per iteration

#### 2.5.3 Integration into Main Loop

**Task:** Add event_bus_process() to main loop

**Code (main.cpp modifications):**
```cpp
#include "event_bus.h"

void setup() {
    Serial.begin(115200);

    // ...existing hardware init...

    // Initialise event bus
    event_bus_init();

    // Publish boot complete event
    event_bus_publish(BOOT_COMPLETE, PRIORITY_LOW, NULL, 0);

    Serial.println("Setup complete");
}

void loop() {
    wdt_reset();

    // Process events FIRST (before state machine update)
    event_bus_process();

    game_update();
    audio_loop();
    led_update();

    #ifndef WOKWI_SIMULATION
    handle_serial_commands();
    #endif
}
```

**Success Criteria:**
- ✅ Event bus initialised on boot
- ✅ event_bus_process() called every loop iteration
- ✅ BOOT_COMPLETE event published

#### 2.5.4 State Machine Integration

**Task:** Publish STATE_ENTERED and STATE_EXITED events from game_transition_to()

**Code (game.cpp modifications):**
```cpp
#include "event_bus.h"

void game_transition_to(GameState new_state) {
    if (new_state == current_state) return;

    GameState old_state = current_state;

    // Call exit handler
    if (state_handlers[current_state].exit) {
        state_handlers[current_state].exit();
    }

    // Publish STATE_EXITED event
    struct {
        GameState old_state;
        GameState new_state;
    } exit_data = {old_state, new_state};
    event_bus_publish(STATE_EXITED, PRIORITY_HIGH, &exit_data, sizeof(exit_data));

    // Change state
    current_state = new_state;
    state_entry_time = millis();

    // Call enter handler
    if (state_handlers[current_state].enter) {
        state_handlers[current_state].enter();
    }

    // Publish STATE_ENTERED event
    struct {
        GameState old_state;
        GameState new_state;
    } enter_data = {old_state, new_state};
    event_bus_publish(STATE_ENTERED, PRIORITY_HIGH, &enter_data, sizeof(enter_data));

    Serial.print("[STATE] Transitioned from ");
    Serial.print(old_state);
    Serial.print(" to ");
    Serial.println(new_state);
}
```

**Success Criteria:**
- ✅ STATE_EXITED published before state change
- ✅ STATE_ENTERED published after state change
- ✅ Events contain old and new state

#### 2.5.5 Wokwi Testing

**Task:** Test event bus in Wokwi simulation

**Test Procedure:**
1. Create test subscriber function:
```cpp
void test_event_subscriber(const Event* event) {
    Serial.print("TEST: Received event type ");
    Serial.print(event->type);
    Serial.print(" at ");
    Serial.println(event->timestamp);
}
```

2. Subscribe to BOOT_COMPLETE and STATE_ENTERED:
```cpp
void setup() {
    // ...
    event_bus_init();

    // Test subscriptions
    event_bus_subscribe(BOOT_COMPLETE, test_event_subscriber);
    event_bus_subscribe(STATE_ENTERED, test_event_subscriber);

    // Publish test event
    event_bus_publish(BOOT_COMPLETE, PRIORITY_LOW, NULL, 0);
}
```

3. Verify serial output:
```
EventBus: Initialising...
EventBus: Ready (queue size: 8 events, 176 bytes)
EventBus: Subscribed to event type 10 (1 subscribers)
EventBus: Subscribed to event type 4 (1 subscribers)
EventBus: Published event type 10 (priority 3, queue: 1/8)
EventBus: Processed 1 events in 45 µs
TEST: Received event type 10 at 1234
```

**Success Criteria:**
- ✅ Event bus initialises successfully
- ✅ Subscription works
- ✅ Events published and dispatched
- ✅ Processing time <200µs
- ✅ Test subscriber receives events

### Memory Budget Validation

**Expected Memory Usage:**
- Event queue: 8 events × 22 bytes = **176 bytes SRAM**
- Subscriber table: 11 types × 4 subscribers × 4 bytes = **176 bytes SRAM**
- Module overhead: queue indices, counts = **~54 bytes SRAM**
- **Total SRAM: ~406 bytes** (matches architecture specification)

**Flash Memory:**
- Event bus implementation: **~3.5KB** (estimated)

**Verification:**
After Phase 2.5 implementation, check memory report should increase by ~406 bytes SRAM.

### Integration Notes for Later Phases

**Phase 3 (NFC Reading):**
Add event publishing for NFC_DETECTED and NFC_REMOVED events:
```cpp
#include "event_bus.h"

// When NFC detected:
PUBLISH_NFC_DETECTED(uid, mood);

// When NFC removed:
event_bus_publish(NFC_REMOVED, PRIORITY_HIGH, NULL, 0);
```

**Phase 7 (Audio Playback):**
Add event publishing for AUDIO_COMPLETE:
```cpp
#include "event_bus.h"

// When audio completes:
PUBLISH_AUDIO_COMPLETE();
```

**Phase 9 (Battery Monitoring):**
Add event publishing for BATTERY_LOW:
```cpp
#include "event_bus.h"

// When battery low detected:
PUBLISH_BATTERY_LOW();
```

**Phase 10 (Error Handling):**
Add event publishing for ERROR_OCCURRED:
```cpp
#include "event_bus.h"

// When error occurs:
PUBLISH_ERROR(error_code);
```

### Wokwi Testing
Use same diagram from Phase 2.

**Verification Steps:**
1. Upload code to Wokwi
2. Observe event bus initialisation in serial monitor
3. Trigger state transitions and verify events published
4. Check event processing time (<200µs requirement)
5. Verify memory usage matches budget

**Phase 2.5 Complete When:**
- ✅ Event bus module implemented (event_bus.h/cpp)
- ✅ Circular queue functional (8 events, 176 bytes)
- ✅ Publish/subscribe pattern working
- ✅ event_bus_process() integrated into main loop
- ✅ State machine publishes STATE_ENTERED/STATE_EXITED
- ✅ Processing time <200µs verified
- ✅ Memory usage matches budget (406 bytes SRAM)
- ✅ Wokwi simulation tests pass

---

## Phase 3: NFC Reading with Retry Logic

### Objective
Integrate PN532 NFC reader with debouncing and 3-attempt retry strategy as specified in architecture v2.4.

### Additional Components
- PN532 NFC Reader (I2C mode)

### Files to Create/Modify
```
include/
  config.h          // [MODIFY] Add NFC pins and retry constants
  nfc_handler.h     // NFC interface

src/
  nfc_handler.cpp   // NFC implementation
  game.cpp          // [MODIFY] Add NFC detection to idle_update()
  hardware.cpp      // [MODIFY] Add NFC initialisation
```

### Implementation Tasks

#### 3.1 NFC Pin Configuration & Constants
**Task:** Add NFC definitions to config.h

**Code (config.h additions):**
```cpp
// I2C Pin Assignments (NFC)
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define PN532_I2C_ADDRESS 0x24

// NFC Retry Configuration (from architecture v2.4)
#define NFC_READ_ATTEMPTS 3
#define NFC_RETRY_DELAY_MS 200
#define NFC_DEBOUNCE_TIME_MS 100
#define NFC_READ_TIMEOUT_MS 1000
```

**Success Criteria:**
- ✅ Constants match architecture.md Section 7.4
- ✅ Pins match architecture.md Section 2.2

#### 3.2 NFC Handler Interface
**Task:** Create nfc_handler module with retry logic

**Code (nfc_handler.h):**
```cpp
#ifndef NFC_HANDLER_H
#define NFC_HANDLER_H

#include <stdint.h>

typedef struct {
    uint8_t attempt_count;
    uint32_t debounce_start;
    uint32_t retry_timestamp;
    bool token_stable;
} NfcReadState;

bool nfc_init();
bool nfc_token_detected();
bool nfc_read_uid(uint8_t uid[7]);
void nfc_reset_retry_state();
void nfc_test();

#endif
```

**Code (nfc_handler.cpp):**
```cpp
#include "nfc_handler.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_PN532.h>

Adafruit_PN532 nfc(I2C_SDA_PIN, I2C_SCL_PIN);

static NfcReadState nfc_state = {0};
static bool nfc_ready = false;

bool nfc_init() {
    Serial.println("NFC: Initialising PN532...");

    nfc.begin();

    // I2C runs at 400kHz Fast Mode (configured by Adafruit_PN532 library)
    // Architecture: I2C 400kHz, 7-bit addressing, PN532 address 0x24

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata) {
        Serial.println("NFC: PN532 not found!");
        return false;
    }

    Serial.print("NFC: Found chip PN5");
    Serial.println((versiondata >> 24) & 0xFF, HEX);
    Serial.print("NFC: Firmware ver. ");
    Serial.print((versiondata >> 16) & 0xFF, DEC);
    Serial.print('.');
    Serial.println((versiondata >> 8) & 0xFF, DEC);

    // Configure for ISO14443A cards
    nfc.SAMConfig();

    nfc_ready = true;
    Serial.println("NFC: Initialisation complete");
    return true;
}

bool nfc_token_detected() {
    if (!nfc_ready) return false;

    // Non-blocking detection (0ms timeout)
    uint8_t uid[7];
    uint8_t uidLength;
    return nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 0);
}

bool nfc_read_uid(uint8_t uid[7]) {
    if (!nfc_ready) return false;

    uint8_t uidLength;
    // Blocking read with timeout
    bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, NFC_READ_TIMEOUT_MS);

    if (success && uidLength == 7) {
        Serial.print("NFC: UID Read: ");
        for (uint8_t i = 0; i < 7; i++) {
            Serial.print(uid[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        return true;
    }

    Serial.println("NFC: Read failed or wrong length");
    return false;
}

void nfc_reset_retry_state() {
    nfc_state.attempt_count = 0;
    nfc_state.debounce_start = 0;
    nfc_state.retry_timestamp = 0;
    nfc_state.token_stable = false;
}

void nfc_test() {
    Serial.println("\n=== NFC TEST ===");
    Serial.println("Tap an NFC tag to test reading...");

    uint32_t start = millis();
    while (millis() - start < 10000) { // 10-second test window
        if (nfc_token_detected()) {
            Serial.println("Token detected!");
            uint8_t uid[7];
            if (nfc_read_uid(uid)) {
                Serial.println("SUCCESS: UID read successfully");
                return;
            } else {
                Serial.println("FAILED: Could not read UID");
            }
        }
        delay(100);
    }

    Serial.println("Test timeout - no tag detected");
}
```

**Success Criteria:**
- ✅ PN532 initialises successfully
- ✅ Firmware version detected and logged
- ✅ Non-blocking detection works
- ✅ UID read successful with 1000ms timeout
- ✅ Retry state structure defined

#### 3.3 Integrate Debouncing & Retry Logic into State Machine
**Task:** Update STATE_IDLE and STATE_NFC_DETECTED with retry logic

**Code (game.cpp modifications):**
```cpp
#include "nfc_handler.h"

static uint8_t nfc_uid[7];

// ============ STATE_IDLE ============
static void idle_enter() {
    Serial.println("[IDLE] Enter: LED pulsing white (eco mode)");
    led_set_animation(LED_IDLE);
    led_set_brightness(POWER_MODE_ECO);
    nfc_reset_retry_state();
}

static void idle_update() {
    static uint32_t debounce_start = 0;

    // Debounce token detection (100ms stable presence required)
    if (nfc_token_detected()) {
        if (!debounce_start) {
            debounce_start = millis();
            Serial.println("[IDLE] Token detected, debouncing...");
        } else if (millis() - debounce_start >= NFC_DEBOUNCE_TIME_MS) {
            Serial.println("[IDLE] Token stable, transitioning");
            debounce_start = 0;
            game_transition_to(STATE_NFC_DETECTED);
        }
    } else {
        if (debounce_start) {
            Serial.println("[IDLE] Token removed during debounce");
        }
        debounce_start = 0;
    }
}

static void idle_exit() {
    Serial.println("[IDLE] Exit");
}

// ============ STATE_NFC_DETECTED ============
static uint8_t attempt_count = 0;
static uint32_t retry_timestamp = 0;

static void nfc_detected_enter() {
    Serial.println("[NFC_DETECTED] Enter: Green flash");
    led_set_animation(LED_DETECTED);
    led_set_brightness(POWER_MODE_NORMAL);
    attempt_count = 0;
    retry_timestamp = millis();
}

static void nfc_detected_update() {
    // Retry logic with 200ms delays between attempts
    if (millis() - retry_timestamp < NFC_RETRY_DELAY_MS) {
        return; // Wait between attempts
    }

    Serial.print("[NFC_DETECTED] Attempt ");
    Serial.print(attempt_count + 1);
    Serial.print("/");
    Serial.println(NFC_READ_ATTEMPTS);

    if (nfc_read_uid(nfc_uid)) {
        Serial.println("[NFC_DETECTED] UID read successful");
        game_transition_to(STATE_VALIDATING);
    } else {
        attempt_count++;
        retry_timestamp = millis();

        if (attempt_count >= NFC_READ_ATTEMPTS) {
            Serial.println("[NFC_DETECTED] All attempts failed");
            game_transition_to(STATE_ERROR);
        }
    }
}

static void nfc_detected_exit() {
    Serial.println("[NFC_DETECTED] Exit");
    attempt_count = 0;
}
```

**Code (hardware.cpp addition):**
```cpp
#include "nfc_handler.h"

void hardware_init() {
    Serial.begin(115200);
    Serial.println("\n\n=== Emotion Station Booting ===");

    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);

    esp_task_wdt_init(WATCHDOG_TIMEOUT_MS / 1000, true);
    esp_task_wdt_add(NULL);

    // Initialise NFC
    if (!nfc_init()) {
        Serial.println("WARNING: NFC initialisation failed");
    }

    Serial.println("Hardware initialisation complete");
}
```

**Code (main.cpp addition for testing):**
```cpp
void setup() {
    hardware_init();
    led_init();
    game_init();

    Serial.println("\nCommands:");
    Serial.println("  t - Test state machine");
    Serial.println("  n - Test NFC reader");
}

void loop() {
    esp_task_wdt_reset();

    hardware_heartbeat();
    led_update();
    game_update();

    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 't') {
            game_test_transitions();
        } else if (cmd == 'n') {
            nfc_test();
        }
    }
}
```

**Success Criteria:**
- ✅ NFC reader initialises on boot
- ✅ Token detection requires 100ms stable presence
- ✅ UID read attempts up to 3 times
- ✅ 200ms delay between retry attempts
- ✅ Success transitions to VALIDATING
- ✅ Failure (3 attempts) transitions to ERROR
- ✅ Serial logs show attempt counts
- ✅ Test command 'n' verifies NFC hardware

### Wokwi Testing
**Note:** Wokwi does not currently support PN532 NFC readers. For this phase, we'll use a simulation approach.

**Simulation Strategy:**
Create a mock NFC handler for Wokwi testing that simulates tag detection via serial commands.

**Code (nfc_handler.cpp - Wokwi simulation version):**
```cpp
#ifdef WOKWI_SIMULATION
// Mock NFC for Wokwi testing
static bool mock_token_present = false;
static uint8_t mock_uid[7] = {0x04, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6};

bool nfc_init() {
    Serial.println("NFC: MOCK MODE (Wokwi simulation)");
    Serial.println("Use serial commands:");
    Serial.println("  'p' - Present token");
    Serial.println("  'r' - Remove token");
    nfc_ready = true;
    return true;
}

bool nfc_token_detected() {
    return mock_token_present;
}

bool nfc_read_uid(uint8_t uid[7]) {
    if (!mock_token_present) return false;

    // Simulate 200ms read time
    delay(200);

    memcpy(uid, mock_uid, 7);
    Serial.print("NFC: MOCK UID: ");
    for (int i = 0; i < 7; i++) {
        Serial.print(uid[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    return true;
}

void nfc_handle_serial_command(char cmd) {
    if (cmd == 'p') {
        mock_token_present = true;
        Serial.println("NFC: MOCK token present");
    } else if (cmd == 'r') {
        mock_token_present = false;
        Serial.println("NFC: MOCK token removed");
    }
}
#endif
```

**Build flag for Wokwi:**
```ini
[env:wokwi]
build_flags = -DWOKWI_SIMULATION
```

**Verification Steps (Wokwi):**
1. Upload code with WOKWI_SIMULATION defined
2. Observe NFC initialises in MOCK MODE
3. System enters IDLE state
4. Type 'p' to simulate token present
5. Watch debouncing (100ms delay)
6. Observe transition to NFC_DETECTED
7. Watch 3 retry attempts (600ms total)
8. Verify SUCCESS transitions to VALIDATING
9. Test failure: don't define MOCK UID read success path
10. Type 'r' to remove token, verify return to IDLE

**Phase 3 Complete When:**
- NFC reader initialises (real or mock)
- Debouncing prevents false positives
- Retry logic attempts 3 times with 200ms delays
- Success/failure paths work correctly
- Serial logging shows detailed retry progress
- Mock mode allows Wokwi testing

---

## Phase 4: UID Validation & Mood Mapping

### Objective
Implement NFC UID to mood category mapping and validation logic.

### Additional Components
None (builds on Phase 3)

### Files to Create/Modify
```
include/
  config.h          // [MODIFY] Add mood definitions
  nfc_handler.h     // [MODIFY] Add validation functions

src/
  nfc_handler.cpp   // [MODIFY] Add UID mapping table
  game.cpp          // [MODIFY] Implement validating_update()
```

### Implementation Tasks

#### 4.1 Mood Category Definitions
**Task:** Add mood types and UID mapping structure

**Code (config.h additions):**
```cpp
typedef enum {
    MOOD_HAPPY = 0,
    MOOD_SAD,
    MOOD_ANGRY,
    MOOD_WORRIED,
    MOOD_FRUSTRATED,
    MOOD_CALM,
    NUM_MOODS,
    MOOD_UNKNOWN = 255
} MoodCategory;

typedef struct {
    uint8_t uid[7];
    MoodCategory mood;
    const char* display_name;
} NfcMoodMapping;
```

**Success Criteria:**
- ✅ All 6 moods defined as per architecture.md Section 4
- ✅ MOOD_UNKNOWN for invalid UIDs
- ✅ Mapping structure matches architecture

#### 4.2 UID Mapping Table & Validation
**Task:** Create UID lookup table and validation function

**Code (nfc_handler.cpp additions):**
```cpp
#include "config.h"

// UID Mapping Table (from architecture.md Section 4)
static const NfcMoodMapping nfc_mappings[] = {
    {{0x04, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6}, MOOD_HAPPY,      "Happy"},
    {{0x04, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0xA1}, MOOD_SAD,        "Sad"},
    {{0x04, 0xC3, 0xD4, 0xE5, 0xF6, 0xA1, 0xB2}, MOOD_ANGRY,      "Angry"},
    {{0x04, 0xD4, 0xE5, 0xF6, 0xA1, 0xB2, 0xC3}, MOOD_WORRIED,    "Worried"},
    {{0x04, 0xE5, 0xF6, 0xA1, 0xB2, 0xC3, 0xD4}, MOOD_FRUSTRATED, "Frustrated"},
    {{0x04, 0xF6, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5}, MOOD_CALM,       "Calm"}
};

#define NUM_MAPPINGS (sizeof(nfc_mappings) / sizeof(nfc_mappings[0]))

MoodCategory nfc_validate_uid(const uint8_t uid[7]) {
    for (uint8_t i = 0; i < NUM_MAPPINGS; i++) {
        bool match = true;
        for (uint8_t j = 0; j < 7; j++) {
            if (uid[j] != nfc_mappings[i].uid[j]) {
                match = false;
                break;
            }
        }

        if (match) {
            Serial.print("NFC: UID matched to mood: ");
            Serial.println(nfc_mappings[i].display_name);
            return nfc_mappings[i].mood;
        }
    }

    Serial.println("NFC: UID not recognised");
    return MOOD_UNKNOWN;
}

const char* nfc_get_mood_name(MoodCategory mood) {
    if (mood >= NUM_MOODS) return "Unknown";
    return nfc_mappings[mood].display_name;
}
```

**Code (nfc_handler.h additions):**
```cpp
#include "config.h"

MoodCategory nfc_validate_uid(const uint8_t uid[7]);
const char* nfc_get_mood_name(MoodCategory mood);
```

**Success Criteria:**
- ✅ All 6 UIDs defined in mapping table
- ✅ Byte-by-byte comparison works correctly
- ✅ Unknown UIDs return MOOD_UNKNOWN
- ✅ Display names accessible

#### 4.3 Implement Validation State
**Task:** Update STATE_VALIDATING to perform mood lookup

**Code (game.cpp modifications):**
```cpp
static MoodCategory current_mood = MOOD_UNKNOWN;

// ============ STATE_VALIDATING ============
static void validating_enter() {
    Serial.println("[VALIDATING] Enter: Validating UID");
    current_mood = MOOD_UNKNOWN;
}

static void validating_update() {
    // Validate UID against mapping table
    current_mood = nfc_validate_uid(nfc_uid);

    if (current_mood == MOOD_UNKNOWN) {
        Serial.println("[VALIDATING] Invalid UID - transitioning to ERROR");
        game_transition_to(STATE_ERROR);
    } else {
        Serial.print("[VALIDATING] Valid mood: ");
        Serial.println(nfc_get_mood_name(current_mood));
        game_transition_to(STATE_SELECTING);
    }
}

static void validating_exit() {
    Serial.println("[VALIDATING] Exit");
}
```

**Success Criteria:**
- ✅ Valid UIDs transition to SELECTING
- ✅ Invalid UIDs transition to ERROR
- ✅ Mood category stored in current_mood
- ✅ Serial output shows mood name

### Wokwi Testing
**Update mock NFC handler:**
```cpp
#ifdef WOKWI_SIMULATION
static uint8_t mock_uid_index = 0;

// All test UIDs from mapping table
static const uint8_t test_uids[][7] = {
    {0x04, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6}, // Happy
    {0x04, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0xA1}, // Sad
    {0x04, 0xC3, 0xD4, 0xE5, 0xF6, 0xA1, 0xB2}, // Angry
    {0x04, 0xD4, 0xE5, 0xF6, 0xA1, 0xB2, 0xC3}, // Worried
    {0x04, 0xE5, 0xF6, 0xA1, 0xB2, 0xC3, 0xD4}, // Frustrated
    {0x04, 0xF6, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5}, // Calm
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}  // Invalid
};

void nfc_handle_serial_command(char cmd) {
    if (cmd == 'p') {
        mock_token_present = true;
        Serial.println("NFC: MOCK token present");
    } else if (cmd == 'r') {
        mock_token_present = false;
        Serial.println("NFC: MOCK token removed");
    } else if (cmd >= '0' && cmd <= '6') {
        // Select test UID (0=Happy, 1=Sad, ..., 6=Invalid)
        mock_uid_index = cmd - '0';
        Serial.print("NFC: Selected test UID: ");
        Serial.println(mock_uid_index);
    }
}

bool nfc_read_uid(uint8_t uid[7]) {
    if (!mock_token_present) return false;
    delay(200);
    memcpy(uid, test_uids[mock_uid_index], 7);
    return true;
}
#endif
```

**Verification Steps:**
1. Upload to Wokwi
2. Type '0' to select Happy UID
3. Type 'p' to present token
4. Verify: IDLE → NFC_DETECTED → VALIDATING → SELECTING
5. Repeat for moods 1-5
6. Type '6' for invalid UID
7. Type 'p' to present
8. Verify: IDLE → NFC_DETECTED → VALIDATING → ERROR
9. Confirm serial shows correct mood names

**Phase 4 Complete When:**
- All 6 moods validate correctly
- Invalid UIDs trigger ERROR state
- Mood names displayed in serial log
- Mock testing allows full validation coverage

---

## Phase 5: SD Card Integration & Activity Metadata

### Objective
Implement SD card file system, load activities.json, and parse activity metadata.

### Additional Components
- MicroSD Card Module (SPI)
- MicroSD Card (formatted FAT32)

### Files to Create/Modify
```
include/
  config.h              // [MODIFY] Add SD pins and activity structure
  activity_manager.h    // Activity selection interface

src/
  activity_manager.cpp  // Activity loading and selection
  hardware.cpp          // [MODIFY] Add SD init
  game.cpp              // [MODIFY] Implement selecting_update()
```

### Implementation Tasks

#### 5.1 SD Card Pin Configuration
**Task:** Add SPI pin definitions for SD card

**Code (config.h additions):**
```cpp
// SPI Pin Assignments (SD Card)
#define SD_SCK_PIN 18
#define SD_MISO_PIN 19
#define SD_MOSI_PIN 23
#define SD_CS_PIN 4

// Activity Configuration
#define MAX_ACTIVITIES 50
#define ACTIVITY_NAME_LENGTH 32
#define ACTIVITY_PATH_LENGTH 64
#define ACTIVITY_TYPE_LENGTH 16
```

**Success Criteria:**
- ✅ Pins match architecture.md Section 2.2
- ✅ Array sizes dimensioned for 44 activities + growth

#### 5.2 Activity Data Structures
**Task:** Define Activity struct and time-of-day enum

**Code (config.h additions):**
```cpp
typedef enum {
    TIME_MORNING = 0,    // 06:00-11:59
    TIME_AFTERNOON,      // 12:00-16:59
    TIME_EVENING,        // 17:00-20:59
    TIME_BEDTIME         // 21:00-05:59
} TimeOfDay;

typedef struct {
    uint8_t id;
    MoodCategory mood;
    char name[ACTIVITY_NAME_LENGTH];
    char file_path[ACTIVITY_PATH_LENGTH];
    uint16_t duration_seconds;
    bool time_flags[4];  // Indexed by TimeOfDay enum
    char type[ACTIVITY_TYPE_LENGTH];
} Activity;
```

**Success Criteria:**
- ✅ Structure matches architecture.md Section 4.2
- ✅ All fields present and correctly sized

#### 5.3 Activity Manager Module
**Task:** Implement SD initialisation and JSON parsing

**Add library dependency to platformio.ini:**
```ini
lib_deps =
    adafruit/Adafruit NeoPixel@^1.11.0
    adafruit/Adafruit PN532@^1.3.1
    bblanchon/ArduinoJson@^6.21.3
```

**Code (activity_manager.h):**
```cpp
#ifndef ACTIVITY_MANAGER_H
#define ACTIVITY_MANAGER_H

#include "config.h"

bool activity_manager_init();
Activity* activity_select(MoodCategory mood, TimeOfDay time);
uint8_t activity_get_count();
void activity_test_load();

#endif
```

**Code (activity_manager.cpp):**
```cpp
#include "activity_manager.h"
#include <SD.h>
#include <ArduinoJson.h>

static Activity activities[MAX_ACTIVITIES];
static uint8_t activity_count = 0;

bool activity_manager_init() {
    Serial.println("ActivityMgr: Initialising SD card...");

    // Initialize SD card (SPI Mode 0, 4MHz clock as per architecture.md)
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("ActivityMgr: SD card init FAILED");
        return false;
    }

    Serial.println("ActivityMgr: SD card mounted");

    // Load activities.json
    File file = SD.open("/activities.json");
    if (!file) {
        Serial.println("ActivityMgr: activities.json not found");
        return false;
    }

    // Allocate JSON document (8KB as per architecture)
    StaticJsonDocument<8192> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.print("ActivityMgr: JSON parse error: ");
        Serial.println(error.c_str());
        return false;
    }

    // Parse activities array
    JsonArray arr = doc["activities"];
    activity_count = 0;

    for (JsonObject obj : arr) {
        if (activity_count >= MAX_ACTIVITIES) break;

        Activity* a = &activities[activity_count];

        a->id = obj["id"];

        // Parse mood string to enum
        const char* mood_str = obj["mood"];
        if (strcmp(mood_str, "happy") == 0) a->mood = MOOD_HAPPY;
        else if (strcmp(mood_str, "sad") == 0) a->mood = MOOD_SAD;
        else if (strcmp(mood_str, "angry") == 0) a->mood = MOOD_ANGRY;
        else if (strcmp(mood_str, "worried") == 0) a->mood = MOOD_WORRIED;
        else if (strcmp(mood_str, "frustrated") == 0) a->mood = MOOD_FRUSTRATED;
        else if (strcmp(mood_str, "calm") == 0) a->mood = MOOD_CALM;
        else continue; // Skip invalid mood

        strlcpy(a->name, obj["name"], ACTIVITY_NAME_LENGTH);
        strlcpy(a->file_path, obj["file_path"], ACTIVITY_PATH_LENGTH);
        a->duration_seconds = obj["duration_seconds"];

        a->time_flags[TIME_MORNING] = obj["morning"];
        a->time_flags[TIME_AFTERNOON] = obj["afternoon"];
        a->time_flags[TIME_EVENING] = obj["evening"];
        a->time_flags[TIME_BEDTIME] = obj["bedtime"];

        strlcpy(a->type, obj["type"], ACTIVITY_TYPE_LENGTH);

        activity_count++;
    }

    Serial.print("ActivityMgr: Loaded ");
    Serial.print(activity_count);
    Serial.println(" activities");

    // Initialize random number generator for activity selection
    // Use floating ADC pin for entropy (GPIO34)
    randomSeed(analogRead(34));
    Serial.println("ActivityMgr: Random seed initialised");

    return true;
}

Activity* activity_select(MoodCategory mood, TimeOfDay time) {
    // Simple selection for now (Phase 6 will add proper algorithm)
    // Just return first match for mood
    for (uint8_t i = 0; i < activity_count; i++) {
        if (activities[i].mood == mood) {
            Serial.print("ActivityMgr: Selected: ");
            Serial.println(activities[i].name);
            return &activities[i];
        }
    }

    Serial.println("ActivityMgr: No activity found for mood");
    return nullptr;
}

uint8_t activity_get_count() {
    return activity_count;
}

void activity_test_load() {
    Serial.println("\n=== ACTIVITY LOAD TEST ===");

    Serial.print("Total activities loaded: ");
    Serial.println(activity_count);

    // Print first 5 activities
    for (uint8_t i = 0; i < min(5, activity_count); i++) {
        Activity* a = &activities[i];
        Serial.print(a->id);
        Serial.print(": ");
        Serial.print(a->name);
        Serial.print(" (");
        Serial.print(nfc_get_mood_name(a->mood));
        Serial.print(", ");
        Serial.print(a->duration_seconds);
        Serial.println("s)");
    }

    Serial.println("=== TEST COMPLETE ===\n");
}
```

**Success Criteria:**
- ✅ SD card mounts successfully
- ✅ activities.json opens and parses
- ✅ All activities loaded into array
- ✅ Mood strings convert to enums correctly
- ✅ Time flags populated
- ✅ Test function displays loaded data

#### 5.4 Create Test activities.json
**Task:** Create minimal test JSON file for SD card

**File: activities.json (place on SD card root):**
```json
{
  "activities": [
    {
      "id": 1,
      "mood": "angry",
      "name": "Dragon Breath",
      "file_path": "/audio/angry/1_dragon_breath.mp3",
      "duration_seconds": 180,
      "morning": true,
      "afternoon": true,
      "evening": true,
      "bedtime": false,
      "type": "breathing"
    },
    {
      "id": 17,
      "mood": "sad",
      "name": "Rainbow Breath",
      "file_path": "/audio/sad/17_rainbow_breath.mp3",
      "duration_seconds": 180,
      "morning": true,
      "afternoon": true,
      "evening": true,
      "bedtime": false,
      "type": "breathing"
    },
    {
      "id": 25,
      "mood": "happy",
      "name": "Sunshine Dance",
      "file_path": "/audio/happy/25_sunshine_dance.mp3",
      "duration_seconds": 180,
      "morning": true,
      "afternoon": true,
      "evening": true,
      "bedtime": false,
      "type": "movement"
    }
  ]
}
```

**Success Criteria:**
- ✅ Valid JSON format
- ✅ At least 3 activities (one per mood for testing)
- ✅ All required fields present

#### 5.5 Integrate with State Machine
**Task:** Update STATE_SELECTING to load activity

**Code (game.cpp modifications):**
```cpp
#include "activity_manager.h"

static Activity* selected_activity = nullptr;

// ============ STATE_SELECTING ============
static void selecting_enter() {
    Serial.println("[SELECTING] Enter: Choosing activity");
    selected_activity = nullptr;
}

static void selecting_update() {
    // Select activity based on mood and time
    TimeOfDay current_time = TIME_AFTERNOON; // Hardcoded for now

    selected_activity = activity_select(current_mood, current_time);

    if (selected_activity == nullptr) {
        Serial.println("[SELECTING] No activities available - ERROR");
        game_transition_to(STATE_ERROR);
    } else {
        Serial.print("[SELECTING] Selected: ");
        Serial.println(selected_activity->name);
        game_transition_to(STATE_PLAYING_ACTIVITY);
    }
}

static void selecting_exit() {
    Serial.println("[SELECTING] Exit");
}
```

**Code (hardware.cpp modification):**
```cpp
#include "activity_manager.h"

void hardware_init() {
    Serial.begin(115200);
    Serial.println("\n\n=== Emotion Station Booting ===");

    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);

    esp_task_wdt_init(WATCHDOG_TIMEOUT_MS / 1000, true);
    esp_task_wdt_add(NULL);

    if (!nfc_init()) {
        Serial.println("WARNING: NFC initialisation failed");
    }

    if (!activity_manager_init()) {
        Serial.println("ERROR: Activity manager init failed");
    }

    Serial.println("Hardware initialisation complete");
}
```

**Code (main.cpp - add test command):**
```cpp
void loop() {
    esp_task_wdt_reset();

    hardware_heartbeat();
    led_update();
    game_update();

    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 't') {
            game_test_transitions();
        } else if (cmd == 'n') {
            nfc_test();
        } else if (cmd == 'a') {
            activity_test_load();
        }
    }
}
```

**Success Criteria:**
- ✅ SD card initialises on boot
- ✅ activities.json loads successfully
- ✅ STATE_SELECTING finds activity for mood
- ✅ Null check prevents crash if no activity
- ✅ Test command 'a' displays loaded activities

### Wokwi Testing
**Wokwi SD Card Support:**
Wokwi does not support SD cards directly in the browser simulator. For this phase, testing requires physical hardware OR creating a mock activity manager.

**Mock Activity Manager for Wokwi:**
```cpp
#ifdef WOKWI_SIMULATION
// Hardcoded activities for testing
static Activity mock_activities[] = {
    {1, MOOD_ANGRY, "Dragon Breath", "/audio/angry/1.mp3", 180, {true, true, true, false}, "breathing"},
    {17, MOOD_SAD, "Rainbow Breath", "/audio/sad/17.mp3", 180, {true, true, true, false}, "breathing"},
    {25, MOOD_HAPPY, "Sunshine Dance", "/audio/happy/25.mp3", 180, {true, true, true, false}, "movement"}
};

bool activity_manager_init() {
    Serial.println("ActivityMgr: MOCK MODE (Wokwi)");
    activity_count = 3;
    memcpy(activities, mock_activities, sizeof(mock_activities));
    Serial.println("ActivityMgr: 3 mock activities loaded");
    return true;
}
#endif
```

**Verification Steps:**
1. Upload with WOKWI_SIMULATION flag
2. Observe activity manager loads 3 mock activities
3. Type 'a' to test activity loading
4. Verify 3 activities displayed
5. Simulate full flow: 'p' token → mood validated → activity selected
6. Confirm correct activity chosen for each mood

**Phase 5 Complete When:**
- SD card mounts and reads JSON (physical hardware)
- Mock activities work in Wokwi simulation
- Activities parsed into Activity structures
- STATE_SELECTING successfully finds activities
- Error handling for missing activities works

---

## Phase 6: Activity Selection Algorithm

### Objective
Implement complete activity selection algorithm with time-of-day filtering and history tracking as specified in architecture.md Section 6.

### Additional Components
None (builds on Phase 5)

### Files to Modify
```
src/
  activity_manager.cpp  // Implement full selection algorithm
  game.cpp              // Add time-of-day logic
```

### Implementation Tasks

#### 6.1 Activity History Structure
**Task:** Implement circular buffer for activity history

**Code (activity_manager.cpp additions):**
```cpp
#define HISTORY_SIZE 5

typedef struct {
    uint8_t recent_activities[NUM_MOODS][HISTORY_SIZE];
    uint8_t history_index[NUM_MOODS];
} ActivityHistory;

static ActivityHistory activity_history = {0};

void activity_add_to_history(MoodCategory mood, uint8_t activity_id) {
    if (mood >= NUM_MOODS) return;

    uint8_t idx = activity_history.history_index[mood];
    activity_history.recent_activities[mood][idx] = activity_id;
    activity_history.history_index[mood] = (idx + 1) % HISTORY_SIZE;

    Serial.print("ActivityMgr: Added to history: ");
    Serial.print(activity_id);
    Serial.print(" for mood ");
    Serial.println(mood);
}

bool activity_is_recent(MoodCategory mood, uint8_t activity_id) {
    if (mood >= NUM_MOODS) return false;

    for (int i = 0; i < HISTORY_SIZE; i++) {
        if (activity_history.recent_activities[mood][i] == activity_id) {
            return true;
        }
    }
    return false;
}

void activity_clear_history(MoodCategory mood) {
    if (mood >= NUM_MOODS) return;

    for (int i = 0; i < HISTORY_SIZE; i++) {
        activity_history.recent_activities[mood][i] = 0;
    }
    activity_history.history_index[mood] = 0;

    Serial.print("ActivityMgr: Cleared history for mood ");
    Serial.println(mood);
}
```

**Success Criteria:**
- ✅ History tracks last 5 activities per mood
- ✅ Circular buffer wraps correctly
- ✅ Recent check works for all 5 entries

#### 6.2 Time-of-Day Detection
**Task:** Implement hour-based time-of-day calculation

**Code (activity_manager.cpp additions):**
```cpp
TimeOfDay activity_get_time_of_day() {
    // For now, use millis() to simulate time progression
    // In production, use ESP32 RTC

    uint32_t seconds_since_boot = millis() / 1000;
    uint8_t simulated_hour = (seconds_since_boot / 60) % 24; // 1 minute = 1 hour

    Serial.print("ActivityMgr: Simulated hour: ");
    Serial.println(simulated_hour);

    if (simulated_hour >= 6 && simulated_hour < 12) {
        return TIME_MORNING;
    } else if (simulated_hour >= 12 && simulated_hour < 17) {
        return TIME_AFTERNOON;
    } else if (simulated_hour >= 17 && simulated_hour < 21) {
        return TIME_EVENING;
    } else {
        return TIME_BEDTIME;
    }
}

const char* activity_get_time_name(TimeOfDay time) {
    const char* names[] = {"Morning", "Afternoon", "Evening", "Bedtime"};
    return names[time];
}
```

**Success Criteria:**
- ✅ Time boundaries match architecture.md Section 6.2
- ✅ Simulation allows testing all time periods
- ✅ Production can use ESP32 RTC

**⚠️ IMPORTANT NOTE:**
This simulation mode allows Wokwi testing but won't work in production. See **Phase 6.5: Real-Time Clock Integration** for production implementation.

#### 6.3 Complete Selection Algorithm
**Task:** Implement multi-stage filtering

**Code (activity_manager.cpp - replace activity_select):**
```cpp
Activity* activity_select(MoodCategory mood, TimeOfDay time) {
    Serial.println("\n=== ACTIVITY SELECTION ===");
    Serial.print("Mood: ");
    Serial.print(nfc_get_mood_name(mood));
    Serial.print(", Time: ");
    Serial.println(activity_get_time_name(time));

    // Stage 1: Filter by mood
    uint8_t candidates[MAX_ACTIVITIES];
    uint8_t candidate_count = 0;

    for (uint8_t i = 0; i < activity_count; i++) {
        if (activities[i].mood == mood) {
            candidates[candidate_count++] = i;
        }
    }

    Serial.print("Stage 1 (mood filter): ");
    Serial.print(candidate_count);
    Serial.println(" candidates");

    if (candidate_count == 0) {
        Serial.println("No activities for mood!");
        return nullptr;
    }

    // Stage 2: Filter by time-of-day
    uint8_t time_candidates[MAX_ACTIVITIES];
    uint8_t time_count = 0;

    for (uint8_t i = 0; i < candidate_count; i++) {
        Activity* a = &activities[candidates[i]];
        if (a->time_flags[time]) {
            time_candidates[time_count++] = candidates[i];
        }
    }

    Serial.print("Stage 2 (time filter): ");
    Serial.print(time_count);
    Serial.println(" candidates");

    // If time filter too restrictive, keep mood candidates
    if (time_count == 0) {
        Serial.println("No time-appropriate activities, using all mood matches");
        time_count = candidate_count;
        memcpy(time_candidates, candidates, candidate_count);
    }

    // Stage 3: Remove recently played
    uint8_t fresh_candidates[MAX_ACTIVITIES];
    uint8_t fresh_count = 0;

    for (uint8_t i = 0; i < time_count; i++) {
        Activity* a = &activities[time_candidates[i]];
        if (!activity_is_recent(mood, a->id)) {
            fresh_candidates[fresh_count++] = time_candidates[i];
        }
    }

    Serial.print("Stage 3 (history filter): ");
    Serial.print(fresh_count);
    Serial.println(" candidates");

    // If all filtered out, clear history and retry
    if (fresh_count == 0) {
        Serial.println("All activities recent, clearing history");
        activity_clear_history(mood);
        fresh_count = time_count;
        memcpy(fresh_candidates, time_candidates, time_count);
    }

    // Stage 4: Random selection
    uint8_t selected_idx = fresh_candidates[random(fresh_count)];
    Activity* selected = &activities[selected_idx];

    Serial.print("Selected: ");
    Serial.println(selected->name);
    Serial.println("=========================\n");

    // Add to history
    activity_add_to_history(mood, selected->id);

    return selected;
}
```

**Success Criteria:**
- ✅ All 4 filtering stages work correctly
- ✅ Fallback logic prevents empty results
- ✅ History clearing works when all filtered
- ✅ Random selection distributes evenly
- ✅ Serial output shows filtering progress

#### 6.4 Update State Machine
**Task:** Use real time-of-day in selection

**Code (game.cpp modification):**
```cpp
static void selecting_update() {
    TimeOfDay current_time = activity_get_time_of_day();

    selected_activity = activity_select(current_mood, current_time);

    if (selected_activity == nullptr) {
        Serial.println("[SELECTING] No activities available - ERROR");
        game_transition_to(STATE_ERROR);
    } else {
        Serial.print("[SELECTING] Selected: ");
        Serial.println(selected_activity->name);
        game_transition_to(STATE_PLAYING_ACTIVITY);
    }
}
```

**Success Criteria:**
- ✅ Time-of-day calculated automatically
- ✅ Selection algorithm receives correct time
- ✅ Activities filtered appropriately

### Testing

**Test Plan:**
1. Create test activities.json with:
   - 3 happy activities (1 morning-only, 1 afternoon-only, 1 all-day)
   - 3 sad activities (similar time distribution)
   - 3 angry activities
2. Test mood filtering (happy token → only happy activities)
3. Test time filtering (boot at different simulated times)
4. Test history (select same mood 6 times, verify no immediate repeats)
5. Test fallback (select mood with only 3 activities 6 times, verify history clears)

**Verification Steps:**
1. Upload code with test JSON
2. Use 'a' command to verify activities loaded
3. Select token with mood "happy"
4. Observe selection filtering in serial output
5. Repeat 5 times, verify no repeats until history cleared
6. Test different moods
7. Let device run to cycle through simulated time periods
8. Verify time-appropriate activities selected

**Phase 6 Complete When:**
- All 4 selection stages functional
- History tracking prevents repeats
- Time-of-day filtering works
- Fallback logic handles edge cases
- Random distribution verified over 20+ selections

---

## Phase 6.5: Real-Time Clock Integration

### Objective
Replace millis() simulation with ESP32 internal RTC for production-ready time-of-day filtering.

### Prerequisites
Phase 6 must be complete and tested with simulation.

### Files to Modify
```
src/
  activity_manager.cpp  // Replace simulation with real RTC
  main.cpp              // Add RTC initialisation
```

### Implementation Tasks

#### 6.5.1 RTC Initialisation
**Task:** Set up ESP32 internal RTC with compile-time timestamp

**Code (main.cpp additions):**
```cpp
#include <time.h>

void rtc_init() {
    Serial.println("RTC: Initialising with compile-time timestamp");

    // Parse compile-time macros
    struct tm timeinfo;
    timeinfo.tm_year = 2026 - 1900;  // Update annually or parse __DATE__
    timeinfo.tm_mon = 1;             // February (0-indexed)
    timeinfo.tm_mday = 10;
    timeinfo.tm_hour = 14;
    timeinfo.tm_min = 0;
    timeinfo.tm_sec = 0;

    time_t t = mktime(&timeinfo);
    struct timeval now = { .tv_sec = t };
    settimeofday(&now, NULL);

    Serial.print("RTC: Set to ");
    Serial.print(asctime(&timeinfo));
}

void setup() {
    // ...existing setup...

    #ifndef WOKWI_SIMULATION
    rtc_init();  // Only use RTC on real hardware
    #endif

    // ...rest of setup...
}
```

**Success Criteria:**
- ✅ RTC initialises with compile-time timestamp
- ✅ Time persists across loop iterations (but NOT across power cycles)
- ✅ Serial output shows correct time

#### 6.5.2 Production Time-of-Day Function
**Task:** Replace millis() simulation with real RTC reading

**Code (activity_manager.cpp modifications):**
```cpp
TimeOfDay activity_get_time_of_day() {
    #ifdef WOKWI_SIMULATION
    // Simulation mode: 1 minute = 1 hour
    uint32_t seconds_since_boot = millis() / 1000;
    uint8_t simulated_hour = (seconds_since_boot / 60) % 24;

    Serial.print("ActivityMgr: Simulated hour: ");
    Serial.println(simulated_hour);

    uint8_t hour = simulated_hour;

    #else
    // Production mode: use ESP32 RTC
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    uint8_t hour = timeinfo.tm_hour;

    Serial.print("ActivityMgr: RTC hour: ");
    Serial.print(hour);
    Serial.print(" (");
    Serial.print(timeinfo.tm_year + 1900);
    Serial.print("-");
    Serial.print(timeinfo.tm_mon + 1);
    Serial.print("-");
    Serial.print(timeinfo.tm_mday);
    Serial.println(")");
    #endif

    // Time-of-day calculation (same for both modes)
    if (hour >= 6 && hour < 12) {
        return TIME_MORNING;
    } else if (hour >= 12 && hour < 17) {
        return TIME_AFTERNOON;
    } else if (hour >= 17 && hour < 21) {
        return TIME_EVENING;
    } else {
        return TIME_BEDTIME;
    }
}
```

**Success Criteria:**
- ✅ Wokwi simulation continues to work (uses millis())
- ✅ Hardware uses real RTC
- ✅ Time boundaries (05:59/06:00, 11:59/12:00, etc.) correctly identified

#### 6.5.3 Serial Time-Setting Command
**Task:** Add ability to set RTC time via serial for testing

**Code (main.cpp additions):**
```cpp
void handle_serial_commands() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        // Format: "SETTIME YYYY-MM-DD HH:MM:SS"
        if (cmd.startsWith("SETTIME ")) {
            int year, month, day, hour, min, sec;
            if (sscanf(cmd.c_str(), "SETTIME %d-%d-%d %d:%d:%d",
                      &year, &month, &day, &hour, &min, &sec) == 6) {

                struct tm timeinfo;
                timeinfo.tm_year = year - 1900;
                timeinfo.tm_mon = month - 1;
                timeinfo.tm_mday = day;
                timeinfo.tm_hour = hour;
                timeinfo.tm_min = min;
                timeinfo.tm_sec = sec;

                time_t t = mktime(&timeinfo);
                struct timeval now = { .tv_sec = t };
                settimeofday(&now, NULL);

                Serial.print("RTC: Time set to ");
                Serial.println(asctime(&timeinfo));
            } else {
                Serial.println("ERROR: Format is SETTIME YYYY-MM-DD HH:MM:SS");
            }
        }
    }
}

void loop() {
    wdt_reset();

    #ifndef WOKWI_SIMULATION
    handle_serial_commands();  // Only on real hardware
    #endif

    game_update();
    // ...rest of loop...
}
```

**Success Criteria:**
- ✅ Can set time via serial monitor
- ✅ Time persists until power cycle
- ✅ Can test all time-of-day boundaries

#### 6.5.4 Boundary Testing
**Task:** Verify time-of-day transitions work correctly

**Test Cases:**
```bash
# Test boundary transitions
SETTIME 2026-02-10 05:59:00  # Should be BEDTIME
SETTIME 2026-02-10 06:00:00  # Should be MORNING
SETTIME 2026-02-10 11:59:00  # Should be MORNING
SETTIME 2026-02-10 12:00:00  # Should be AFTERNOON
SETTIME 2026-02-10 16:59:00  # Should be AFTERNOON
SETTIME 2026-02-10 17:00:00  # Should be EVENING
SETTIME 2026-02-10 20:59:00  # Should be EVENING
SETTIME 2026-02-10 21:00:00  # Should be BEDTIME
```

**Verification:**
1. Set time via serial command
2. Tap NFC token
3. Check serial output: "ActivityMgr: RTC hour: X"
4. Verify correct time period detected
5. Verify activities filtered appropriately

**Success Criteria:**
- ✅ All 8 boundary transitions correct
- ✅ Activities filtered according to time_flags
- ✅ Serial output confirms correct hour and period

### Limitations & Future Enhancements

**Current Limitations:**
- ⚠️ Time resets to compile-time on power cycle
- ⚠️ No network time sync (NTP)
- ⚠️ No external RTC battery backup

**Future Enhancements (v2.0):**
- WiFi + NTP synchronisation
- External RTC module (DS3231) with battery
- Automatic timezone detection
- Daylight saving time support

**Phase 6.5 Complete When:**
- RTC initialises on hardware boot
- Time-of-day detection uses real RTC
- Serial time-setting command works
- All boundary transitions verified
- Simulation mode still works for Wokwi testing

---

## Phase 7: Audio Playback (Placeholder)

### Objective
Prepare audio playback infrastructure with test tones. Full MP3 playback will be verified on hardware (Wokwi has limited audio support).

### Additional Components
- MAX98357A I2S DAC
- 3W 4Ω Speaker

### Files to Create/Modify
```
include/
  audio_player.h        // Audio interface

src/
  audio_player.cpp      // Audio implementation
  hardware.cpp          // [MODIFY] Add I2S init
  game.cpp              // [MODIFY] Add audio to PLAYING state
```

### Implementation Tasks

#### 7.1 Audio Pin Configuration
**Task:** Add I2S pin definitions

**Code (config.h additions):**
```cpp
// I2S Pin Assignments (Audio)
#define I2S_BCLK_PIN 25
#define I2S_LRC_PIN 26
#define I2S_DOUT_PIN 27
#define AUDIO_VOLUME 18  // 0-21 scale (18 = comfortable listening level per architecture.md)
```

**Success Criteria:**
- ✅ Pins match architecture.md Section 2.2

#### 7.2 Audio Player Module
**Task:** Create audio interface (placeholder for Wokwi, full implementation for hardware)

**Add library:**
```ini
lib_deps =
    ...existing...
    https://github.com/schreibfaul1/ESP32-audioI2S.git#v2.0.0
```

**Code (audio_player.h):**
```cpp
#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <stdint.h>

bool audio_init();
void audio_loop();
void audio_play(const char* file_path);
void audio_stop();
bool audio_is_running();
void audio_test();

#endif
```

**Code (audio_player.cpp):**
```cpp
#include "audio_player.h"
#include "config.h"

#ifdef WOKWI_SIMULATION
// Placeholder for Wokwi (no I2S audio support)
static bool mock_playing = false;
static uint32_t mock_start_time = 0;
static uint32_t mock_duration = 0;

bool audio_init() {
    Serial.println("Audio: MOCK MODE (Wokwi)");
    return true;
}

void audio_loop() {
    if (mock_playing && millis() - mock_start_time >= mock_duration) {
        mock_playing = false;
        Serial.println("Audio: MOCK playback complete");
    }
}

void audio_play(const char* file_path) {
    Serial.print("Audio: MOCK play: ");
    Serial.println(file_path);
    mock_playing = true;
    mock_start_time = millis();
    mock_duration = 5000; // 5-second mock playback
}

void audio_stop() {
    Serial.println("Audio: MOCK stop");
    mock_playing = false;
}

bool audio_is_running() {
    return mock_playing;
}

void audio_test() {
    Serial.println("Audio: MOCK test (5s playback)");
    audio_play("/test.mp3");
}

#else
// Real implementation for hardware
#include "Audio.h"

Audio audio;

bool audio_init() {
    Serial.println("Audio: Initialising I2S...");

    audio.setPinout(I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DOUT_PIN);
    audio.setVolume(AUDIO_VOLUME);

    Serial.println("Audio: I2S ready");
    return true;
}

void audio_loop() {
    audio.loop(); // MUST be called every iteration
}

void audio_play(const char* file_path) {
    Serial.print("Audio: Playing: ");
    Serial.println(file_path);

    audio.connecttoFS(SD, file_path);
}

void audio_stop() {
    Serial.println("Audio: Stopping");
    audio.stopSong();
}

bool audio_is_running() {
    return audio.isRunning();
}

void audio_test() {
    Serial.println("Audio: Testing with startup sound");
    audio_play("/audio/system/startup.mp3");
}
#endif
```

**Success Criteria:**
- ✅ Mock mode compiles for Wokwi
- ✅ Real mode compiles for hardware
- ✅ Mock simulates playback duration
- ✅ Real mode uses ESP32-audioI2S library

#### 7.3 Integrate with State Machine
**Task:** Add audio to PLAYING_ACTIVITY state

**Code (game.cpp modifications):**
```cpp
#include "audio_player.h"

// ============ STATE_PLAYING_ACTIVITY ============
static void playing_activity_enter() {
    Serial.println("[PLAYING] Enter: Starting audio");
    led_set_animation(LED_BREATHING);
    led_set_brightness(POWER_MODE_NORMAL);

    if (selected_activity) {
        audio_play(selected_activity->file_path);
    }
}

static void playing_activity_update() {
    // Check if audio finished
    if (!audio_is_running()) {
        Serial.println("[PLAYING] Audio complete");
        game_transition_to(STATE_ACTIVITY_COMPLETE);
    }

    // Also allow manual skip after 2 seconds (for testing)
    if (millis() - state_entry_time > 2000) {
        if (Serial.available() && Serial.read() == 's') {
            Serial.println("[PLAYING] Manual skip");
            audio_stop();
            game_transition_to(STATE_ACTIVITY_COMPLETE);
        }
    }
}

static void playing_activity_exit() {
    Serial.println("[PLAYING] Exit: Stopping audio");
    audio_stop();
}
```

**Code (hardware.cpp modification):**
```cpp
#include "audio_player.h"

void hardware_init() {
    // ...existing init...

    if (!audio_init()) {
        Serial.println("WARNING: Audio init failed");
    }

    Serial.println("Hardware initialisation complete");
}
```

**Code (main.cpp modification):**
```cpp
void loop() {
    esp_task_wdt_reset();

    hardware_heartbeat();
    led_update();
    audio_loop();  // Add audio loop
    game_update();

    // ...existing serial commands...
}
```

**Success Criteria:**
- ✅ Audio starts when entering PLAYING state
- ✅ audio_loop() called every iteration
- ✅ State transitions when audio completes
- ✅ Manual skip works for testing

### Testing

**Wokwi Testing:**
1. Upload with WOKWI_SIMULATION flag
2. Simulate full flow to PLAYING state
3. Verify serial shows "MOCK play" message
4. Wait 5 seconds, verify transition to COMPLETE
5. Test manual skip with 's' key

**Hardware Testing (when available):**
1. Upload without WOKWI_SIMULATION
2. Place test MP3 on SD card: /audio/system/startup.mp3
3. Power on, verify I2S initialises
4. Type 'a' command (audio test)
5. Listen for audio playback
6. Test full flow with activity MP3 files

**Phase 7 Complete When:**
- Mock audio works in Wokwi simulation
- Real audio compiles for hardware
- State machine integrates audio correctly
- Manual skip command works
- audio_loop() called every iteration (non-blocking verified)

### Audio File Specifications

**Format Requirements:**
As specified in architecture.md Section 5, all audio files must meet these specifications:

- **Format:** MP3 (MPEG-1 Audio Layer 3)
- **Sample Rate:** 44.1kHz
- **Bit Depth:** 16-bit
- **Channels:** Mono
- **Bitrate:** 128kbps CBR (Constant Bit Rate)
- **File Size:** ~1MB per minute of audio

**Creating Audio Files with FFmpeg:**
```bash
# Convert any audio file to correct format
ffmpeg -i input.wav -ar 44100 -ac 1 -b:a 128k -acodec libmp3lame output.mp3

# Parameters explained:
#   -ar 44100     = Sample rate 44.1kHz
#   -ac 1         = Mono (1 channel)
#   -b:a 128k     = Bitrate 128kbps
#   -acodec libmp3lame = MP3 encoder
```

**Audio Normalisation (Recommended):**
```bash
# Normalise audio levels to -14 LUFS (comfortable listening level)
ffmpeg -i input.wav -af loudnorm=I=-14:TP=-1.5:LRA=11 -ar 44100 -ac 1 -b:a 128k output.mp3
```

**Validation:**
After encoding, verify:
- Duration: 2-5 minutes recommended (3-4 minutes typical for guided activities)
- File size: ~1MB per minute (3-5MB for typical activity)
- No DRM or watermarks
- No clipping or distortion
- Clear and age-appropriate for children

**Required System Audio Files:**
```
/audio/system/startup.mp3        (3s) - "Hello, how are you feeling today?"
/audio/system/error.mp3          (2s) - Gentle error tone
/audio/system/completion.mp3    (2s) - Success chime
/audio/system/low_battery.mp3   (3s) - "Time to charge me"
```

**Content Guidelines:**
- Voice: Warm, gentle, age-appropriate
- Pace: Slow and clear for children
- Language: Simple, encouraging
- Background: Minimal music, no distracting sound effects
- Silence: Include 2-3 second pauses between instructions

**Testing Audio Files:**
1. Copy test files to SD card `/audio/` directory
2. Use serial command: `TESTAUDIO /audio/system/startup.mp3`
3. Verify playback smooth and volume appropriate
4. Check ESP32-audioI2S library compatibility

---

## Phase 8: Data Logging

### Objective
Implement session logging to SD card in CSV format as specified in architecture.md Section 4.3.

### Additional Components
None (uses SD card from Phase 5)

### Files to Create/Modify
```
include/
  data_logger.h         // Logging interface

src/
  data_logger.cpp       // CSV logging implementation
  game.cpp              // [MODIFY] Add logging to states
```

### Implementation Tasks

#### 8.1 Session Log Structure
**Task:** Define SessionLog data structure

**Code (config.h additions):**
```cpp
typedef struct {
    uint32_t timestamp;      // Unix timestamp (seconds since boot for now)
    MoodCategory mood;
    uint8_t activity_id;
    char activity_name[ACTIVITY_NAME_LENGTH];
    uint16_t duration_seconds;
    bool completed;
    TimeOfDay time_of_day;
} SessionLog;
```

**Success Criteria:**
- ✅ Structure matches architecture.md Section 4.3

#### 8.2 Data Logger Module
**Task:** Implement CSV writer

**Code (data_logger.h):**
```cpp
#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include "config.h"

bool logger_init();
bool logger_log_session(const SessionLog* session);
void logger_test();

#endif
```

**Code (data_logger.cpp):**
```cpp
#include "data_logger.h"
#include <SD.h>

#define LOG_FILE "/sessions.csv"

bool logger_init() {
    Serial.println("Logger: Initialising");

    // Check if log file exists
    if (!SD.exists(LOG_FILE)) {
        Serial.println("Logger: Creating new log file");

        // Create with CSV header
        File file = SD.open(LOG_FILE, FILE_WRITE);
        if (!file) {
            Serial.println("Logger: Failed to create log file");
            return false;
        }

        file.println("timestamp,mood,activity_id,activity_name,duration_seconds,completed,time_of_day");
        file.close();

        Serial.println("Logger: Log file created");
    } else {
        Serial.println("Logger: Using existing log file");
    }

    return true;
}

bool logger_log_session(const SessionLog* session) {
    if (!session) return false;

    Serial.println("Logger: Writing session...");

    File file = SD.open(LOG_FILE, FILE_APPEND);
    if (!file) {
        Serial.println("Logger: Failed to open log file");
        return false;
    }

    // Format: timestamp,mood,activity_id,activity_name,duration,completed,time_of_day
    file.print(session->timestamp);
    file.print(",");
    file.print(nfc_get_mood_name(session->mood));
    file.print(",");
    file.print(session->activity_id);
    file.print(",");
    file.print(session->activity_name);
    file.print(",");
    file.print(session->duration_seconds);
    file.print(",");
    file.print(session->completed ? "true" : "false");
    file.print(",");
    file.println(activity_get_time_name(session->time_of_day));

    file.close();

    Serial.println("Logger: Session logged");
    return true;
}

void logger_test() {
    Serial.println("\n=== LOGGER TEST ===");

    SessionLog test_session;
    test_session.timestamp = millis() / 1000;
    test_session.mood = MOOD_HAPPY;
    test_session.activity_id = 25;
    strcpy(test_session.activity_name, "Sunshine Dance");
    test_session.duration_seconds = 180;
    test_session.completed = true;
    test_session.time_of_day = TIME_AFTERNOON;

    if (logger_log_session(&test_session)) {
        Serial.println("Test session logged successfully");

        // Read back and display
        File file = SD.open(LOG_FILE);
        if (file) {
            Serial.println("\nCurrent log contents:");
            while (file.available()) {
                Serial.write(file.read());
            }
            file.close();
        }
    }

    Serial.println("=== TEST COMPLETE ===\n");
}
```

**Success Criteria:**
- ✅ CSV file created with header
- ✅ Sessions append to file
- ✅ CSV format matches architecture specification
- ✅ File survives power cycles

#### 8.3 Integrate Logging with State Machine
**Task:** Log session start and completion

**Code (game.cpp additions):**
```cpp
#include "data_logger.h"

static SessionLog current_session;
static uint32_t activity_start_time = 0;

// ============ STATE_PLAYING_ACTIVITY ============
static void playing_activity_enter() {
    Serial.println("[PLAYING] Enter: Starting audio");
    led_set_animation(LED_BREATHING);
    led_set_brightness(POWER_MODE_NORMAL);

    if (selected_activity) {
        audio_play(selected_activity->file_path);

        // Initialise session log
        activity_start_time = millis();
        current_session.timestamp = millis() / 1000;
        current_session.mood = current_mood;
        current_session.activity_id = selected_activity->id;
        strcpy(current_session.activity_name, selected_activity->name);
        current_session.duration_seconds = 0;
        current_session.completed = false;
        current_session.time_of_day = activity_get_time_of_day();

        Serial.println("[PLAYING] Session started");
    }
}

static void playing_activity_update() {
    if (!audio_is_running()) {
        Serial.println("[PLAYING] Audio complete");

        // Mark as completed
        current_session.completed = true;
        current_session.duration_seconds = (millis() - activity_start_time) / 1000;

        game_transition_to(STATE_ACTIVITY_COMPLETE);
    }
}

static void playing_activity_exit() {
    Serial.println("[PLAYING] Exit: Stopping audio");
    audio_stop();

    // Log session (whether completed or cancelled)
    if (current_session.duration_seconds == 0) {
        current_session.duration_seconds = (millis() - activity_start_time) / 1000;
    }

    logger_log_session(&current_session);
}
```

**Code (hardware.cpp modification):**
```cpp
#include "data_logger.h"

void hardware_init() {
    // ...existing init...

    if (!logger_init()) {
        Serial.println("WARNING: Logger init failed");
    }

    Serial.println("Hardware initialisation complete");
}
```

**Success Criteria:**
- ✅ Session logged when activity starts
- ✅ Completion status tracked correctly
- ✅ Duration calculated accurately
- ✅ Cancelled sessions logged with completed=false

### Testing

**Test Plan:**
1. Complete full user flow (token tap → activity → completion)
2. Verify CSV entry created
3. Test cancelled session (remove token during playback)
4. Verify cancelled session logged with completed=false
5. Power cycle device, verify log persists
6. Test 10 sessions, verify all logged correctly

**Verification Steps:**
1. Upload code to hardware with SD card
2. Complete 3 full sessions (different moods)
3. Cancel 1 session mid-playback
4. Type 'l' to display log contents
5. Remove SD card, open in computer
6. Verify sessions.csv opens in Excel
7. Confirm all fields populated correctly

**Phase 8 Complete When:**
- CSV file created automatically
- Sessions log on activity start
- Completion status tracked
- Cancelled sessions logged
- Log persists across power cycles
- CSV readable in Excel/Google Sheets

---

## Phase 8.5: Activity History Persistence

### Objective
Load activity history from sessions.csv on boot to maintain history across power cycles. This prevents repeated activities even after device restart.

### Prerequisites
Phase 8 must be complete and sessions.csv logging functional.

### Files to Modify
```
src/
  logger.cpp            // Add history loading function
  activity_manager.cpp  // Add history restoration API
  main.cpp              // Load history during setup
```

### Implementation Tasks

#### 8.5.1 History Restoration API
**Task:** Add function to populate activity history from session data

**Code (activity_manager.h additions):**
```cpp
// Restore history from external data
void activity_restore_history(MoodCategory mood, uint8_t activity_id);
```

**Code (activity_manager.cpp additions):**
```cpp
void activity_restore_history(MoodCategory mood, uint8_t activity_id) {
    if (mood >= NUM_MOODS) return;

    // Add to history without logging
    uint8_t idx = activity_history.history_index[mood];
    activity_history.recent_activities[mood][idx] = activity_id;
    activity_history.history_index[mood] = (idx + 1) % HISTORY_SIZE;

    Serial.print("ActivityMgr: Restored to history: ");
    Serial.print(activity_id);
    Serial.print(" for mood ");
    Serial.println(mood);
}
```

**Success Criteria:**
- ✅ Can populate history from external source
- ✅ History prevents repeats as expected

#### 8.5.2 CSV History Loader
**Task:** Parse sessions.csv and extract last 5 activities per mood

**Code (logger.cpp additions):**
```cpp
#include "activity_manager.h"

void logger_load_history() {
    Serial.println("Logger: Loading activity history from sessions.csv");

    File file = SD.open("/sessions.csv");
    if (!file) {
        Serial.println("Logger: No sessions.csv found, starting fresh");
        return;
    }

    // Skip header line
    file.readStringUntil('\n');

    // Track last 5 activities per mood (simple approach: read entire file)
    // More sophisticated: read file in reverse, but Arduino SD doesn't support seek from end

    struct SessionEntry {
        MoodCategory mood;
        uint8_t activity_id;
        uint32_t timestamp;
    };

    // Temporary array to hold ALL sessions (memory intensive but simple)
    #define MAX_SESSION_HISTORY 100
    SessionEntry sessions[MAX_SESSION_HISTORY];
    uint8_t session_count = 0;

    // Parse all sessions
    while (file.available() && session_count < MAX_SESSION_HISTORY) {
        String line = file.readStringUntil('\n');
        if (line.length() == 0) continue;

        // Parse CSV: timestamp,mood_name,mood_id,activity_id,activity_name,duration,completed
        uint32_t timestamp;
        char mood_name[20];
        uint8_t mood_id, activity_id;
        char activity_name[100];
        uint32_t duration;
        char completed[10];

        int parsed = sscanf(line.c_str(), "%lu,%19[^,],%hhu,%hhu,%99[^,],%lu,%9s",
                           &timestamp, mood_name, &mood_id, &activity_id,
                           activity_name, &duration, completed);

        if (parsed == 7 && strcmp(completed, "true") == 0) {
            // Only include completed sessions in history
            sessions[session_count].mood = (MoodCategory)mood_id;
            sessions[session_count].activity_id = activity_id;
            sessions[session_count].timestamp = timestamp;
            session_count++;
        }
    }

    file.close();

    Serial.print("Logger: Parsed ");
    Serial.print(session_count);
    Serial.println(" completed sessions");

    // Now populate history with last 5 per mood
    // Process in reverse order (newest first)
    for (MoodCategory m = MOOD_HAPPY; m < NUM_MOODS; m = (MoodCategory)(m + 1)) {
        uint8_t count = 0;

        // Iterate backwards through sessions
        for (int i = session_count - 1; i >= 0 && count < HISTORY_SIZE; i--) {
            if (sessions[i].mood == m) {
                activity_restore_history(m, sessions[i].activity_id);
                count++;
            }
        }

        Serial.print("Logger: Restored ");
        Serial.print(count);
        Serial.print(" activities for mood ");
        Serial.println(nfc_get_mood_name(m));
    }

    Serial.println("Logger: History restoration complete");
}
```

**Success Criteria:**
- ✅ Parses sessions.csv correctly
- ✅ Extracts last 5 completed sessions per mood
- ✅ Restores history in correct order (newest first)
- ✅ Handles missing or corrupt CSV gracefully

#### 8.5.3 Integration into Setup
**Task:** Load history during system initialisation

**Code (logger.h additions):**
```cpp
void logger_load_history();
```

**Code (main.cpp modifications):**
```cpp
void setup() {
    // ...existing setup...

    // Initialise SD card and logger
    if (sd_init()) {
        logger_init();

        #ifndef WOKWI_SIMULATION
        // Load activity history from previous sessions
        logger_load_history();
        #endif
    } else {
        Serial.println("ERROR: SD card failed, history will not persist");
    }

    // ...rest of setup...
}
```

**Success Criteria:**
- ✅ History loaded before first NFC read
- ✅ Activity selection reflects loaded history immediately
- ✅ Simulation mode skips history loading (Wokwi limitation)

#### 8.5.4 Testing History Persistence
**Task:** Verify history survives power cycles

**Test Procedure:**
```
1. Power on device (fresh start)
2. Select "happy" token
3. Note which activity is played (e.g., Activity 3)
4. Complete activity
5. Select "happy" token again
6. Note second activity (should be different, e.g., Activity 7)
7. Power cycle device (simulate battery removal)
8. Power on device
9. Select "happy" token
10. Verify: Activity is NOT 3 or 7 (history preserved)
```

**Expected Behaviour:**
- First 5 selections create history
- Power cycle
- Next selections avoid those 5 activities
- After 6th selection, oldest activity (first) becomes available again

**Serial Output Verification:**
```
Logger: Loading activity history from sessions.csv
Logger: Parsed 5 completed sessions
Logger: Restored 5 activities for mood happy
Logger: Restored 0 activities for mood sad
ActivityMgr: Restored to history: 3 for mood 0
ActivityMgr: Restored to history: 7 for mood 0
...
```

**Success Criteria:**
- ✅ History persists across power cycles
- ✅ Activity repeats prevented even after restart
- ✅ CSV parsing handles edge cases (empty file, corrupt data)
- ✅ Memory usage acceptable (100 session entries = ~1.2KB)

### Memory Considerations

**RAM Usage:**
- Temporary session array: 100 entries × 12 bytes = 1200 bytes
- Released after logger_load_history() completes
- Only during boot, not continuous

**Optimisation Notes:**
- Current implementation simple but memory-intensive
- For v2.0: consider reading CSV in reverse (requires custom SD seek)
- Alternative: maintain separate history.csv file (smaller, faster to parse)

**Phase 8.5 Complete When:**
- History loads from sessions.csv on boot
- Activity selection respects loaded history
- Power cycle testing passes
- Memory usage within acceptable bounds
- Error handling prevents crashes on corrupt CSV

---

## Phase 9: Battery Monitoring & Low Power State

### Objective
Implement battery voltage monitoring and STATE_LOW_BATTERY with critical brightness mode.

### Additional Components
- Voltage divider for battery monitoring (2x 10kΩ resistors)

### Files to Modify
```
include/
  config.h          // Add battery constants
  hardware.h        // Add battery monitoring API

src/
  hardware.cpp      // Implement ADC reading
  game.cpp          // Add battery checks
```

### Implementation Tasks

#### 9.1 Battery Monitoring Configuration
**Task:** Add ADC configuration for battery reading

**Code (config.h additions):**
```cpp
// Battery Monitoring
#define BATTERY_ADC_PIN 34
#define BATTERY_VOLTAGE_DIVIDER_RATIO 2.0  // Using 2x 10kΩ resistors
#define BATTERY_LOW_THRESHOLD 3.4
#define BATTERY_CRITICAL_THRESHOLD 3.3
#define BATTERY_CHECK_INTERVAL_MS 10000  // Check every 10 seconds
```

**Success Criteria:**
- ✅ Pin matches architecture.md (GPIO34, analogue-only)
- ✅ Thresholds match architecture specifications

#### 9.2 Battery Reading Function
**Task:** Implement ADC reading with voltage conversion

**Code (hardware.h additions):**
```cpp
float battery_get_voltage();
bool battery_is_low();
bool battery_is_critical();
```

**Code (hardware.cpp additions):**
```cpp
static float last_battery_voltage = 4.2;

float battery_get_voltage() {
    // Read ADC (0-4095 for 0-3.3V)
    int adc_value = analogRead(BATTERY_ADC_PIN);

    // Convert to voltage (accounting for voltage divider)
    float voltage = (adc_value / 4095.0) * 3.3 * BATTERY_VOLTAGE_DIVIDER_RATIO;

    // Smooth reading (simple exponential moving average)
    last_battery_voltage = (last_battery_voltage * 0.9) + (voltage * 0.1);

    return last_battery_voltage;
}

bool battery_is_low() {
    return last_battery_voltage < BATTERY_LOW_THRESHOLD;
}

bool battery_is_critical() {
    return last_battery_voltage < BATTERY_CRITICAL_THRESHOLD;
}

void hardware_init() {
    // ...existing init...

    // Initialise ADC for battery monitoring
    pinMode(BATTERY_ADC_PIN, INPUT);
    analogSetAttenuation(ADC_11db); // 0-3.3V range

    // Read initial battery voltage
    battery_get_voltage();
    Serial.print("Battery voltage: ");
    Serial.print(last_battery_voltage);
    Serial.println("V");

    Serial.println("Hardware initialisation complete");
}
```

**Success Criteria:**
- ✅ ADC reads correctly
- ✅ Voltage divider calculation accurate
- ✅ Smoothing reduces noise
- ✅ Low/critical thresholds work

#### 9.3 Integrate Battery Monitoring into State Machine
**Task:** Add battery checks to IDLE and implement LOW_BATTERY state

**Code (game.cpp modifications):**
```cpp
// ============ STATE_IDLE ============
static void idle_update() {
    static uint32_t last_battery_check = 0;
    static uint32_t debounce_start = 0;

    // Check battery voltage periodically
    if (millis() - last_battery_check >= BATTERY_CHECK_INTERVAL_MS) {
        last_battery_check = millis();

        float voltage = battery_get_voltage();
        Serial.print("[IDLE] Battery: ");
        Serial.print(voltage);
        Serial.println("V");

        if (battery_is_low()) {
            Serial.println("[IDLE] Battery low, transitioning");
            game_transition_to(STATE_LOW_BATTERY);
            return;
        }
    }

    // ...existing NFC debouncing code...
}

// ============ STATE_LOW_BATTERY ============
static void low_battery_enter() {
    Serial.println("[LOW_BATTERY] Enter: Critical power mode");
    led_set_animation(LED_ERROR); // Amber pulse
    led_set_brightness(POWER_MODE_CRITICAL);

    // Play low battery warning (if audio available)
    #ifndef WOKWI_SIMULATION
    audio_play("/audio/system/low_battery.mp3");
    #endif
}

static void low_battery_update() {
    static uint32_t last_check = 0;

    // Check battery every 10 seconds
    if (millis() - last_check >= 10000) {
        last_check = millis();

        float voltage = battery_get_voltage();
        Serial.print("[LOW_BATTERY] Voltage: ");
        Serial.println(voltage);

        if (battery_is_critical()) {
            Serial.println("[LOW_BATTERY] CRITICAL - entering deep sleep");
            // TODO: Implement deep sleep in Phase 10
        } else if (voltage > 3.5) {
            Serial.println("[LOW_BATTERY] Voltage recovered, returning to IDLE");
            game_transition_to(STATE_IDLE);
        }
    }
}

static void low_battery_exit() {
    Serial.println("[LOW_BATTERY] Exit");
}
```

**Success Criteria:**
- ✅ Battery checked every 10 seconds in IDLE
- ✅ Transition to LOW_BATTERY when <3.4V
- ✅ LED brightness reduces to 25% (CRITICAL mode)
- ✅ Recovery to IDLE when voltage rises above 3.5V

### Testing

**Wokwi Testing:**
Mock battery voltage via serial commands.

**Code (hardware.cpp - Wokwi addition):**
```cpp
#ifdef WOKWI_SIMULATION
void battery_set_mock_voltage(float voltage) {
    last_battery_voltage = voltage;
    Serial.print("Battery: MOCK voltage set to ");
    Serial.println(voltage);
}
#endif
```

**Main.cpp serial command:**
```cpp
if (cmd >= '0' && cmd <= '9') {
    // Set mock battery voltage: 0=4.2V, 5=3.5V, 9=3.0V
    float voltage = 4.2 - ((cmd - '0') * 0.15);
    battery_set_mock_voltage(voltage);
}
```

**Verification Steps:**
1. Upload to Wokwi with WOKWI_SIMULATION
2. Observe initial battery reading (should be ~4.2V)
3. Type '5' to set 3.5V (near threshold)
4. Wait for battery check, verify still IDLE
5. Type '6' to set 3.3V (low threshold)
6. Wait up to 10s, verify transition to LOW_BATTERY
7. Observe LED brightness reduces to 25%
8. Type '4' to set 3.7V (recovery)
9. Wait 10s, verify return to IDLE

**Phase 9 Complete When:**
- Battery voltage reads correctly
- Low threshold triggers state transition
- LED brightness reduces in low power mode
- Recovery works when voltage rises
- Mock testing allows full coverage in Wokwi

---

## Phase 9.5: Battery Calibration & Validation

### Objective
Validate battery voltage thresholds with actual hardware and document discharge characteristics for the 2000mAh 3.7V LiPo battery.

### Prerequisites
- Phase 9 complete with functional battery monitoring
- Hardware build with actual battery connected

### Hardware Requirements
- 3.7V LiPo battery (2000mAh single-cell)
- Voltage divider circuit (2× 10kΩ resistors) installed
- Fully charged battery (4.2V)
- Multimeter for reference measurements

### Implementation Tasks

#### 9.5.1 Battery Chemistry Documentation
**Task:** Document battery specifications and expected behaviour

**Battery Specifications:**
```
Chemistry: LiPo (Lithium Polymer) 3.7V nominal
Capacity: 2000mAh
Cell Configuration: Single-cell (1S)

Voltage Range:
- Maximum (fully charged): 4.2V
- Nominal (rated voltage): 3.7V
- 50% capacity: ~3.7V
- 20% capacity: ~3.5V
- 10% capacity: ~3.4V
- 5% capacity (critical): ~3.3V
- Minimum safe discharge: 3.0V (never discharge below this)

Expected Runtime:
- Average current: 352mA (from architecture.md Section 2.2)
- Capacity: 2000mAh
- Runtime = 2000mAh / 352mA = 5.7 hours
```

**Code (config.h - add documentation):**
```cpp
// Battery Monitoring
// Battery: 3.7V LiPo 2000mAh (single-cell)
// Voltage divider: 2x 10kΩ (divides by 2)
// ADC range: 0-3.3V (with 11dB attenuation)
// Max measurable battery voltage: 3.3V × 2 = 6.6V (sufficient for 4.2V max)

#define BATTERY_ADC_PIN 34
#define BATTERY_VOLTAGE_DIVIDER_RATIO 2.0

// Thresholds (validated with actual discharge testing)
#define BATTERY_LOW_THRESHOLD 3.4      // ~10% capacity remaining
#define BATTERY_CRITICAL_THRESHOLD 3.3 // ~5% capacity, prevent damage
#define BATTERY_RECOVERY_THRESHOLD 3.5 // Return to normal operation
#define BATTERY_CHECK_INTERVAL_MS 10000
```

**Success Criteria:**
- ✅ Battery chemistry documented
- ✅ Voltage ranges understood
- ✅ Expected runtime calculated

#### 9.5.2 ADC Calibration
**Task:** Verify ADC readings match actual battery voltage

**Test Procedure:**
1. Fully charge battery to 4.2V
2. Connect multimeter to battery terminals
3. Read battery_get_voltage() from serial monitor
4. Compare values:
   - Multimeter: 4.2V
   - ESP32 reading: should be 4.15-4.25V (±50mV acceptable)
5. Repeat at various charge levels (4.0V, 3.7V, 3.5V, 3.3V)

**Calibration Code (if needed):**
```cpp
// Add to config.h if calibration factor required
#define BATTERY_ADC_CALIBRATION_FACTOR 1.0  // Adjust if readings off

// Modify battery_get_voltage()
float voltage = (adc_value / 4095.0) * 3.3 * BATTERY_VOLTAGE_DIVIDER_RATIO;
voltage *= BATTERY_ADC_CALIBRATION_FACTOR;  // Apply calibration
```

**Acceptance Criteria:**
- ✅ ADC readings within ±100mV of multimeter
- ✅ Consistent readings (< ±50mV variation between samples)

#### 9.5.3 Discharge Curve Testing
**Task:** Measure actual battery voltage under load over time

**Test Setup:**
1. Fully charge battery (4.2V)
2. Connect to ESP32 with all peripherals active
3. Run device continuously with logging
4. Record voltage every 30 minutes
5. Continue until critical threshold reached

**Data Collection Code:**
```cpp
// Add to main.cpp for discharge testing
void log_discharge_data() {
    static uint32_t last_log = 0;

    if (millis() - last_log >= 1800000) {  // Every 30 minutes
        last_log = millis();

        float voltage = battery_get_voltage();
        uint32_t uptime_hours = millis() / 3600000;

        Serial.print("DISCHARGE_LOG,");
        Serial.print(uptime_hours);
        Serial.print(",");
        Serial.println(voltage, 3);  // 3 decimal places
    }
}
```

**Expected Discharge Profile:**
```
Time (h) | Voltage (V) | Capacity Remaining
---------|-------------|-------------------
0.0      | 4.2         | 100%
1.0      | 3.9         | ~80%
2.0      | 3.8         | ~60%
3.0      | 3.75        | ~40%
4.0      | 3.65        | ~20%
5.0      | 3.5         | ~10%
5.5      | 3.4         | ~5% (LOW threshold)
5.7      | 3.3         | <5% (CRITICAL threshold)
```

**Success Criteria:**
- ✅ Discharge curve follows expected LiPo profile
- ✅ Runtime within 20% of calculated 5.7 hours
- ✅ Thresholds trigger at appropriate capacity levels

#### 9.5.4 Threshold Validation
**Task:** Verify low/critical thresholds prevent battery damage

**Safety Requirements:**
- ⚠️ NEVER discharge LiPo below 3.0V (causes permanent damage)
- ⚠️ Critical threshold (3.3V) must trigger BEFORE 3.0V reached
- ⚠️ Device must enter deep sleep before 3.0V

**Test Procedure:**
1. Discharge battery to 3.6V (above low threshold)
2. Let device run normally
3. Observe transition to STATE_LOW_BATTERY at 3.4V
4. Continue monitoring
5. Verify critical shutdown or deep sleep at 3.3V
6. Measure voltage after shutdown (should be > 3.2V)

**Validation Checklist:**
```
✅ Low threshold (3.4V) triggers warning
✅ Critical threshold (3.3V) triggers shutdown/deep sleep
✅ Voltage never drops below 3.1V before sleep
✅ Recovery threshold (3.5V) returns to normal operation
✅ Hysteresis prevents rapid state transitions
```

**Success Criteria:**
- ✅ Low threshold accurate (±100mV)
- ✅ Critical threshold prevents damage
- ✅ Deep sleep activates before dangerous voltage
- ✅ Device cannot be bricked by deep discharge

#### 9.5.5 Current Consumption Validation
**Task:** Verify actual current draw matches architecture specifications

**Test Equipment:**
- USB power meter or multimeter in series with battery

**Test Procedure:**
1. **IDLE State:**
   - Measure current with LEDs breathing
   - Expected: 50-100mA
2. **READING State:**
   - NFC active, LEDs on
   - Expected: 150-200mA
3. **PLAYING State:**
   - Audio playing, LEDs animating
   - Expected: 300-400mA
4. **Average:**
   - Run full session cycle
   - Expected average: ~352mA (per architecture)

**Code (optional current monitoring):**
```cpp
// If using INA219 current sensor (future enhancement)
void display_power_stats() {
    Serial.print("Current: ");
    Serial.print(current_mA);
    Serial.print("mA, Voltage: ");
    Serial.print(voltage);
    Serial.print("V, Power: ");
    Serial.print(power_mW);
    Serial.println("mW");
}
```

**Success Criteria:**
- ✅ Idle current < 100mA
- ✅ Active current < 400mA
- ✅ Average current within 20% of 352mA
- ✅ No unexpected current spikes

### Calibration Results Documentation

**Create calibration report in docs/battery-calibration.md:**
```markdown
# Battery Calibration Report

## Test Setup
- Battery: [Brand/Model] 3.7V 2000mAh LiPo
- Date: [Test Date]
- Temperature: [Ambient temp during test]

## ADC Calibration
| Multimeter (V) | ESP32 Reading (V) | Error (mV) |
|----------------|-------------------|------------|
| 4.20           |                   |            |
| 4.00           |                   |            |
| 3.70           |                   |            |
| 3.50           |                   |            |
| 3.30           |                   |            |

Calibration Factor: [1.0 or adjusted value]

## Discharge Test Results
| Time (h) | Voltage (V) | Notes |
|----------|-------------|-------|
| 0.0      | 4.20        | Fully charged |
| ...      | ...         | ... |
| [X]      | 3.40        | Low threshold triggered |
| [Y]      | 3.30        | Critical threshold triggered |

Total Runtime: [X.X hours]

## Current Consumption
| State        | Current (mA) |
|--------------|--------------|
| IDLE         |              |
| READING      |              |
| PLAYING      |              |
| Average      |              |

## Conclusions
- Thresholds validated: [Yes/No]
- Runtime acceptable: [Yes/No]
- Adjustments needed: [None/List changes]
```

**Phase 9.5 Complete When:**
- ADC calibration verified with multimeter
- Discharge curve measured and documented
- Thresholds validated under load
- Current consumption within specifications
- Calibration report completed
- Battery safety confirmed (never drops below 3.0V)

---

## Phase 10: Error Handling & Polish

### Objective
Implement comprehensive error handling, recovery mechanisms, and final polish for production readiness.

### Files to Modify
All files (refinement pass)

### Implementation Tasks

#### 10.1 Error Code System
**Task:** Implement complete error handling

**Code (config.h additions):**
```cpp
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
```

**Code (game.cpp additions):**
```cpp
static ErrorCode current_error = ERROR_NONE;

void handle_error(ErrorCode error) {
    current_error = error;

    Serial.print("ERROR: ");
    switch (error) {
        case ERROR_NFC_READ_TIMEOUT:
            Serial.println("NFC read timeout after 3 attempts");
            break;
        case ERROR_INVALID_UID:
            Serial.println("NFC UID not recognised");
            break;
        case ERROR_NO_ACTIVITIES:
            Serial.println("No activities available for mood");
            break;
        // ...other cases...
        default:
            Serial.println("Unknown error");
    }

    game_transition_to(STATE_ERROR);
}

// Update ERROR state
static void error_update() {
    // 5-second error display, then auto-recover
    if (millis() - state_entry_time > 5000) {
        Serial.println("[ERROR] Timeout, recovering to IDLE");
        current_error = ERROR_NONE;
        game_transition_to(STATE_IDLE);
    }
}
```

**Success Criteria:**
- ✅ All error types defined
- ✅ Errors logged with descriptions
- ✅ Auto-recovery after timeout
- ✅ Clear user feedback via LED

#### 10.2 Watchdog Reset Detection
**Task:** Detect and log watchdog resets

**Code (main.cpp additions):**
```cpp
#include <esp_system.h>

void setup() {
    // Check reset reason
    esp_reset_reason_t reset_reason = esp_reset_reason();

    hardware_init();
    led_init();
    game_init();

    if (reset_reason == ESP_RST_WDT || reset_reason == ESP_RST_TASK_WDT) {
        Serial.println("WARNING: Watchdog reset detected!");
        handle_error(ERROR_WATCHDOG_RESET);
    }

    Serial.println("\n=== Ready ===");
}
```

**Success Criteria:**
- ✅ Watchdog resets detected
- ✅ Logged to serial and SD card
- ✅ System recovers gracefully

#### 10.3 Deep Sleep Mode
**Task:** Implement deep sleep for critical battery

**Code (hardware.cpp addition):**
```cpp
#include <esp_sleep.h>

void hardware_enter_deep_sleep() {
    Serial.println("Entering deep sleep mode...");
    Serial.flush();

    // Save any pending data
    logger_log_session(&current_session);

    // Turn off LEDs
    pixels.clear();
    pixels.show();

    // Enter deep sleep (wake on reset only)
    esp_deep_sleep_start();
}
```

**Code (game.cpp - LOW_BATTERY update):**
```cpp
static void low_battery_update() {
    static uint32_t last_check = 0;

    if (millis() - last_check >= 10000) {
        last_check = millis();

        float voltage = battery_get_voltage();

        if (battery_is_critical()) {
            Serial.println("[LOW_BATTERY] CRITICAL - entering deep sleep");
            hardware_enter_deep_sleep();
        }
        // ...rest of function...
    }
}
```

**Success Criteria:**
- ✅ Deep sleep triggered at <3.3V
- ✅ LEDs turn off before sleep
- ✅ Wake on reset button works

#### 10.4 Production Optimisations
**Task:** Final polish and optimisations

**Changes:**
1. Remove test code (game_test_transitions, etc.)
2. Reduce serial logging verbosity
3. Add compile-time debug flag
4. Optimise memory usage
5. Verify all watchdog resets cleared

**Code (config.h addition):**
```cpp
// Debug configuration
#define DEBUG_SERIAL 1  // Set to 0 for production

#if DEBUG_SERIAL
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
#endif
```

**Success Criteria:**
- ✅ Production build compiles with DEBUG_SERIAL=0
- ✅ Memory usage within budget (25% SRAM)
- ✅ No watchdog resets during 1-hour stress test
- ✅ All features functional

### Final Integration Testing

**Test Suite:**
1. **Happy Path Test**
   - Power on → tap token → activity plays → completes → log created
   - Repeat for all 6 moods
   - Verify: LED animations, audio, logging

2. **Error Recovery Test**
   - Invalid UID → ERROR → auto-recover to IDLE
   - NFC timeout → ERROR → recover
   - Missing activity → ERROR → recover

3. **Battery Test**
   - Reduce voltage to 3.4V → LOW_BATTERY state
   - Reduce to 3.3V → deep sleep
   - Recover voltage → return to IDLE

4. **Stress Test**
   - Run 50 consecutive sessions
   - Verify: no memory leaks, no watchdog resets
   - Check: log file integrity, history tracking

5. **Power Cycle Test**
   - Power off mid-activity
   - Power on, verify recovery
   - Check: incomplete session logged

**Success Criteria:**
- ✅ All tests pass
- ✅ No crashes or resets
- ✅ User experience smooth and responsive
- ✅ Data integrity maintained

**Phase 10 Complete When:**
- All error cases handled gracefully
- Watchdog reset detection working
- Deep sleep functional
- Production optimisations applied
- Full integration test suite passes
- **NEW:** Watchdog timing analysis complete (max loop time measured)
- **NEW:** Edge case test suite passes

---

## Phase 10 Addendum: Watchdog Timing Analysis & Edge Cases

### Objective
Validate that main loop completes well within watchdog timeout and test edge cases in activity selection.

### 10.A Watchdog Timing Analysis

**Architecture Requirement:**
- Watchdog timeout: 4 seconds (WDTO_4S)
- All operations must complete <1 second per frame
- Event processing: <200µs per iteration

**Task:** Measure actual loop iteration time to confirm safety margin

**Implementation Code (main.cpp):**
```cpp
// Add to main.cpp for timing analysis
static uint32_t max_loop_time = 0;
static uint32_t total_loops = 0;
static uint64_t total_time = 0;

void loop() {
    uint32_t loop_start = micros();

    wdt_reset();
    game_update();
    audio_loop();
    led_update();

    #ifndef WOKWI_SIMULATION
    handle_serial_commands();
    #endif

    uint32_t loop_duration = micros() - loop_start;

    // Track statistics
    total_loops++;
    total_time += loop_duration;

    if (loop_duration > max_loop_time) {
        max_loop_time = loop_duration;
        Serial.print("⚠️  New max loop time: ");
        Serial.print(max_loop_time);
        Serial.print(" µs (");
        Serial.print(max_loop_time / 1000.0);
        Serial.println(" ms)");
    }

    // Report statistics every 10 seconds
    static uint32_t last_report = 0;
    if (millis() - last_report >= 10000) {
        last_report = millis();

        uint32_t avg_time = total_time / total_loops;

        Serial.println("\n=== LOOP TIMING STATS ===");
        Serial.print("Average loop time: ");
        Serial.print(avg_time);
        Serial.println(" µs");

        Serial.print("Max loop time: ");
        Serial.print(max_loop_time);
        Serial.print(" µs (");
        Serial.print(max_loop_time / 1000.0);
        Serial.println(" ms)");

        Serial.print("Watchdog safety margin: ");
        Serial.print(4000.0 - (max_loop_time / 1000.0));
        Serial.println(" ms");

        Serial.print("Total loops: ");
        Serial.println(total_loops);
        Serial.println("========================\n");
    }
}
```

**Test Procedure:**
1. Run device through all states
2. Trigger all state transitions
3. Play audio
4. Log to SD card
5. Monitor serial output for timing reports
6. Record maximum loop time

**Expected Results:**
```
Average loop time: ~500-2000 µs (0.5-2 ms)
Max loop time: <100,000 µs (100 ms)
Watchdog safety margin: >3900 ms
```

**Success Criteria:**
- ✅ Average loop time <5 ms
- ✅ Maximum loop time <100 ms
- ✅ Watchdog safety margin >3.5 seconds
- ✅ No watchdog resets during normal operation
- ✅ Timing consistent across all states

**Warning Threshold:**
If max loop time exceeds 500ms, investigate:
- SD card write blocking?
- Audio processing delays?
- LED animation calculations?

### 10.B Edge Case Test Suite

**Objective:** Validate activity selection algorithm handles unusual scenarios gracefully.

#### Test Case 1: Single Activity Per Mood
**Scenario:** Mood category has only 1 activity available

**Test Data (activities.json):**
```json
{
  "activities": [
    {
      "id": 1,
      "mood": "happy",
      "name": "Only Happy Activity",
      "audio_file": "/audio/happy/only.mp3",
      "time_flags": [true, true, true, true]
    }
  ]
}
```

**Test Procedure:**
1. Load JSON with single activity for "happy" mood
2. Select happy token 10 times consecutively
3. Verify activity plays every time
4. Check serial output for "Clearing history" messages

**Expected Behaviour:**
- First 5 selections: Activity 1 added to history
- 6th selection: History full, all activities recent, history cleared
- Activity 1 selected again (only option available)
- This repeats indefinitely

**Success Criteria:**
- ✅ No crashes or infinite loops
- ✅ Activity plays every time despite being "recent"
- ✅ History clearing logic activates correctly

#### Test Case 2: No Time-Appropriate Activities
**Scenario:** All activities for mood have time_flags = false for current time

**Test Data (activities.json):**
```json
{
  "activities": [
    {
      "id": 2,
      "mood": "calm",
      "name": "Never Appropriate",
      "audio_file": "/audio/calm/never.mp3",
      "time_flags": [false, false, false, false]
    },
    {
      "id": 3,
      "mood": "calm",
      "name": "Also Never",
      "audio_file": "/audio/calm/also_never.mp3",
      "time_flags": [false, false, false, false]
    }
  ]
}
```

**Test Procedure:**
1. Load JSON with activities that have all time_flags = false
2. Select calm token at any time of day
3. Observe behaviour

**Expected Behaviour:**
- Stage 1 (mood filter): 2 candidates
- Stage 2 (time filter): 0 candidates
- Stage 3 (history filter): Skipped (0 candidates)
- Fallback: Select from mood-filtered candidates randomly

**Success Criteria:**
- ✅ No crashes
- ✅ Activity still selected (time filter bypassed when no matches)
- ✅ Serial output shows fallback logic activated

#### Test Case 3: Random Seed Validation
**Scenario:** Verify random selection doesn't repeat same pattern each boot

**Implementation:**
```cpp
// Add to setup() in main.cpp
void setup() {
    // ...existing setup...

    // Seed random number generator with analogue noise
    randomSeed(analogRead(34));  // Read floating pin for entropy

    Serial.print("Random seed initialised: ");
    Serial.println(analogRead(34));

    // ...rest of setup...
}
```

**Test Procedure:**
1. Add randomSeed() to setup
2. Boot device 3 times
3. Select same mood token each time
4. Record which activity selected

**Expected Results:**
- Boot 1: Activity 3
- Boot 2: Activity 7 (different)
- Boot 3: Activity 1 (different again)

**Success Criteria:**
- ✅ Different activity selected each boot (for first selection)
- ✅ Selection order varies across boots
- ✅ Distribution appears random over 10+ boots

#### Test Case 4: Rapid Token Taps
**Scenario:** User taps token multiple times quickly (debouncing test)

**Test Procedure:**
1. Tap token 5 times in 5 seconds
2. Observe how many sessions start

**Expected Behaviour:**
- First tap: Transition to READING state
- Second tap (within debounce period): Ignored
- Third tap (if NFC removed and re-presented): New session

**Success Criteria:**
- ✅ Debouncing prevents duplicate sessions
- ✅ Only valid tap-remove-tap sequence starts new session
- ✅ No spurious state transitions

#### Test Case 5: Empty Activities Array
**Scenario:** activities.json has no activities for selected mood

**Test Data:**
```json
{
  "activities": [
    {
      "id": 99,
      "mood": "happy",
      "name": "Only Happy",
      "audio_file": "/audio/happy/only.mp3",
      "time_flags": [true, true, true, true]
    }
  ]
}
```

**Test Procedure:**
1. Load JSON with activities only for "happy" mood
2. Select "sad" token (no activities available)
3. Observe error handling

**Expected Behaviour:**
- Stage 1 (mood filter): 0 candidates
- Error handling: Transition to STATE_ERROR
- LED shows error animation
- System does not crash

**Success Criteria:**
- ✅ No crashes or watchdog resets
- ✅ Graceful transition to ERROR state
- ✅ User-friendly error indication (LED + optional audio)
- ✅ Can recover by selecting different token

### Edge Case Test Results Documentation

**Create test report in docs/edge-case-testing.md:**
```markdown
# Edge Case Test Results

## Test Date: [Date]
## Firmware Version: [Version]

| Test Case | Status | Notes |
|-----------|--------|-------|
| Single Activity Per Mood | ✅ Pass | History clearing works correctly |
| No Time-Appropriate Activities | ✅ Pass | Fallback logic activated |
| Random Seed Validation | ✅ Pass | Different patterns across boots |
| Rapid Token Taps | ✅ Pass | Debouncing effective |
| Empty Activities Array | ✅ Pass | Error state reached gracefully |

## Issues Found
[List any issues discovered during testing]

## Recommendations
[Any improvements or fixes needed]
```

**Phase 10 Addendum Complete When:**
- ✅ Watchdog timing analysis complete
- ✅ Maximum loop time measured and acceptable (<100ms)
- ✅ All 5 edge case tests pass
- ✅ Random seed properly initialised
- ✅ Edge case test report documented

---

## Architecture Deviations & Design Decisions

### Overview
This implementation plan intentionally deviates from the architecture.md v2.4 specification in specific areas. These deviations are documented here with clear rationale.

### Deviation 1: Real-Time Clock - Limited Functionality

**Architecture Specification (Section 6.2):**
- Time-of-day filtering based on current hour
- No explicit RTC hardware specified

**Implementation Approach:**
- **Simulation (Wokwi):** millis() simulation (1 minute = 1 hour)
- **Hardware (ESP32):** Internal RTC with compile-time timestamp
- **Limitation:** Time resets on power cycle

**Rationale:**
1. **No External RTC:** Avoids adding DS3231 module (~$5, extra wiring)
2. **Sufficient for v1.0:** Time persists during usage session
3. **Cost/Complexity:** External RTC + battery backup overkill for initial release

**Impact:**
- ✅ Time-of-day filtering works during usage
- ⚠️ Time resets to compile-time on boot
- ⚠️ No timezone or DST support
- ⚠️ Cannot track long-term time accurately

**Future Enhancements (v2.0):**
- WiFi + NTP synchronisation (no hardware change)
- Optional: External RTC with battery backup for offline accuracy

### Deviation 2: Activity History - RAM-Based with CSV Restoration

**Architecture Specification (Implicit):**
- Activity history prevents repeats
- Session logging to SD card

**Implementation Approach:**
- **Primary:** In-RAM history (circular buffer, 5 entries per mood)
- **Secondary:** Restore from sessions.csv on boot (Phase 8.5)
- **Limitation:** Requires parsing CSV on every boot

**Rationale:**
1. **Performance:** RAM access faster than SD card lookups
2. **Simplicity:** Circular buffer simpler than persistent storage
3. **Trade-off:** Acceptable to parse CSV once per boot

**Impact:**
- ✅ History prevents repeats during usage
- ✅ History persists across power cycles (via CSV restoration)
- ⚠️ Boot time increases by ~2-3 seconds for CSV parsing
- ⚠️ Memory overhead: 1200 bytes during boot (temporary)

**Alternative Considered:**
- Dedicated history.csv file (faster parsing, smaller file)
- Rejected: Adds complexity, sessions.csv already contains data

### Deviation 3: Audio Format - Not Enforced

**Architecture Specification (Section 5):**
- Format: MP3, 44.1kHz, 16-bit, mono, 128kbps CBR

**Implementation Approach:**
- **Phase 7:** ESP32-audioI2S library used (supports multiple formats)
- **Documentation:** FFmpeg encoding guide provided
- **Validation:** No runtime format checking

**Rationale:**
1. **Library Flexibility:** ESP32-audioI2S handles multiple formats
2. **User Responsibility:** Content creators follow encoding guide
3. **No Enforcement:** Runtime format detection complex and unnecessary

**Impact:**
- ✅ Correct format plays perfectly
- ⚠️ Incorrect format may fail silently or cause crashes
- ⚠️ No user-friendly error message for wrong format

**Mitigation:**
- Clear documentation in Phase 7
- Test files provided with correct format
- Validation during content creation workflow

### Deviation 4: Battery Thresholds - Assumed Until Calibration

**Architecture Specification (Section 2.2):**
- Low threshold: <3.4V
- Critical threshold: <3.3V

**Implementation Approach:**
- **Phase 9:** Thresholds implemented as specified
- **Phase 9.5:** Validation with actual hardware

**Rationale:**
1. **Standard Values:** 3.4V/3.3V typical for LiPo batteries
2. **Safe Defaults:** Conservative thresholds prevent damage
3. **Hardware Dependent:** Final calibration requires actual battery testing

**Impact:**
- ✅ Safe defaults implemented
- ⚠️ May trigger early/late depending on specific battery
- ⚠️ Requires empirical validation (Phase 9.5)

**Validation Required:**
- Measure actual discharge curve
- Adjust thresholds if needed based on real data
- Document calibration in battery-calibration.md

### Summary of Deviations

| Architecture Feature | Implementation Status | Rationale |
|---------------------|----------------------|-----------|
| Real-Time Clock | Limited (compile-time init) | No external hardware, sufficient for v1.0 |
| Activity History | RAM + CSV restoration | Performance + persistence trade-off |
| Audio Format Validation | Not enforced | Library flexibility, user responsibility |
| Battery Thresholds | Assumed (require validation) | Safe defaults, hardware-dependent |

**Overall Philosophy:**
This implementation plan prioritises **incremental delivery** and **testability** over complete architecture adherence. All deviations are **reversible** and can be addressed in v2.0 without major refactoring.

**Recommendation:**
Proceed with v1.0 implementation as planned. Collect field data and user feedback to inform v2.0 architecture decisions.

---

## Implementation Complete - Success Criteria

### System-Level Validation

**Hardware Integration:**
- ✅ All components initialise successfully
- ✅ No I2C/SPI/I2S bus conflicts
- ✅ Power consumption within budget (<400mA average)

**Software Architecture:**
- ✅ State machine lifecycle correct (enter/exit/update)
- ✅ No blocking delays anywhere
- ✅ Watchdog timer never triggers
- ✅ Memory usage <30% SRAM, <25% Flash

**User Experience:**
- ✅ Token tap to activity start <2 seconds
- ✅ LED feedback immediate and clear
- ✅ Audio playback smooth and uninterrupted
- ✅ Error states gentle and non-scary

**Data Integrity:**
- ✅ All sessions logged correctly
- ✅ CSV file survives power cycles
- ✅ Activity history prevents immediate repeats

**Reliability:**
- ✅ 24-hour continuous operation test passes
- ✅ 100 consecutive sessions without errors
- ✅ Recovery from all error conditions
- ✅ Deep sleep and wake functional

---

## Next Steps After Implementation

1. **Field Testing**
   - Deploy to beta families
   - Collect usage data and feedback
   - Refine activity selection algorithm

2. **Content Creation**
   - Record all 44 guided activity audio files
   - Professional voice talent for child-friendly delivery
   - Test audio quality and compression

3. **Hardware Enclosure**
   - Design child-safe enclosure
   - Large tactile NFC token holders
   - Speaker grille and LED diffuser

4. **Future Enhancements**
   - WiFi connectivity for data sync
   - OTA firmware updates
   - Parent dashboard web app

---

*Document Version: 1.1*
*Last Updated: 2026-02-10*
*Changes in v1.1:*
- *Added Phase 6.5: Real-Time Clock Integration*
- *Added Phase 7: Audio File Specifications section*
- *Added Phase 8.5: Activity History Persistence*
- *Added Phase 9.5: Battery Calibration & Validation*
- *Added Phase 10 Addendum: Watchdog Timing Analysis & Edge Cases*
- *Added Architecture Deviations & Design Decisions section*
- *Total phases: 10 core + 4 sub-phases (14 total)*
*Based on Architecture v2.4*
