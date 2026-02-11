#include "game.h"
#include "led_controller.h"
#include "config.h"

// State variables
static GameState current_state = STATE_IDLE;
static unsigned long state_entry_time = 0;

// Test sequence states
typedef enum {
    TEST_IDLE = 0,           // Not running
    TEST_START,              // Starting test
    TEST_NFC_STEP,           // Testing NFC detection
    TEST_VALIDATING_STEP,    // Testing validation
    TEST_SELECTING_STEP,     // Testing selection
    TEST_PLAYING_STEP,       // Testing activity playback
    TEST_COMPLETE_STEP,      // Testing completion
    TEST_IDLE_RETURN_STEP,   // Testing return to idle
    TEST_ERROR_STEP,         // Testing error state
    TEST_FINISH_STEP,        // Final step
    NUM_TEST_STATES
} TestState;

// Test state machine variables
static TestState test_state = TEST_IDLE;
static unsigned long test_step_start_time = 0;
static bool test_active = false;

// State names for debug logging
static const char* state_names[] = {
    "IDLE",
    "NFC_DETECTED",
    "VALIDATING",
    "SELECTING",
    "PLAYING_ACTIVITY",
    "ACTIVITY_COMPLETE",
    "ERROR",
    "LOW_BATTERY"
};

// Forward declarations for state handlers
// IDLE state
static void idle_enter();
static void idle_update();
static void idle_exit();

// NFC_DETECTED state
static void nfc_detected_enter();
static void nfc_detected_update();
static void nfc_detected_exit();

// VALIDATING state
static void validating_enter();
static void validating_update();
static void validating_exit();

// SELECTING state
static void selecting_enter();
static void selecting_update();
static void selecting_exit();

// PLAYING_ACTIVITY state
static void playing_enter();
static void playing_update();
static void playing_exit();

// ACTIVITY_COMPLETE state
static void complete_enter();
static void complete_update();
static void complete_exit();

// ERROR state
static void error_enter();
static void error_update();
static void error_exit();

// LOW_BATTERY state
static void low_battery_enter();
static void low_battery_update();
static void low_battery_exit();

// Test state machine
static void game_test_update();

// State handler table indexed by GameState enum
static const StateHandler state_handlers[NUM_STATES] = {
    {idle_enter, idle_update, idle_exit},                    // STATE_IDLE
    {nfc_detected_enter, nfc_detected_update, nfc_detected_exit},  // STATE_NFC_DETECTED
    {validating_enter, validating_update, validating_exit},  // STATE_VALIDATING
    {selecting_enter, selecting_update, selecting_exit},     // STATE_SELECTING
    {playing_enter, playing_update, playing_exit},           // STATE_PLAYING_ACTIVITY
    {complete_enter, complete_update, complete_exit},        // STATE_ACTIVITY_COMPLETE
    {error_enter, error_update, error_exit},                 // STATE_ERROR
    {low_battery_enter, low_battery_update, low_battery_exit} // STATE_LOW_BATTERY
};

// =============================================================================
// CENTRALISED TRANSITION FUNCTION
// =============================================================================

void game_transition_to(GameState new_state) {
    // Validate new state
    if (new_state >= NUM_STATES) {
        Serial.println("ERROR: Invalid state transition requested");
        return;
    }

    // Log state change
    Serial.print("State: ");
    Serial.print(state_names[current_state]);
    Serial.print(" -> ");
    Serial.println(state_names[new_state]);

    // Call current state's exit function
    if (state_handlers[current_state].exit != nullptr) {
        state_handlers[current_state].exit();
    }

    // Update state and entry time
    current_state = new_state;
    state_entry_time = millis();

    // Call new state's enter function
    if (state_handlers[current_state].enter != nullptr) {
        state_handlers[current_state].enter();
    }
}

// =============================================================================
// PUBLIC API FUNCTIONS
// =============================================================================

void game_init() {
    Serial.println("Game state machine initialising");
    current_state = STATE_IDLE;
    state_entry_time = millis();

    // Call initial state's enter function
    if (state_handlers[current_state].enter != nullptr) {
        state_handlers[current_state].enter();
    }
}

void game_update() {
    // Skip state update during test sequence to prevent automatic transitions
    // from interfering with manual test control.
    // NOTE: Currently all state update functions only contain automatic transition
    // logic. If future states need non-transition logic (sensor polling, etc.),
    // it must be placed in enter functions or handled separately.
    if (!test_active && state_handlers[current_state].update != nullptr) {
        state_handlers[current_state].update();
    }

    // Update test state machine if active
    game_test_update();
}

GameState game_get_current_state() {
    return current_state;
}

// =============================================================================
// STATE IMPLEMENTATIONS: IDLE
// =============================================================================

static void idle_enter() {
    Serial.println("[IDLE] Enter: LED pulsing white (eco mode)");
    led_set_animation(LED_IDLE);
    led_set_brightness(POWER_MODE_ECO);
}

static void idle_update() {
    // Auto-transition after 5 seconds (testing only)
    if (millis() - state_entry_time > 5000) {
        game_transition_to(STATE_NFC_DETECTED);
    }
}

static void idle_exit() {
    Serial.println("[IDLE] Exit");
}

// =============================================================================
// STATE IMPLEMENTATIONS: NFC_DETECTED
// =============================================================================

static void nfc_detected_enter() {
    Serial.println("[NFC_DETECTED] Enter: Green flash");
    led_set_animation(LED_DETECTED);
    led_set_brightness(POWER_MODE_NORMAL);
}

static void nfc_detected_update() {
    // Transition after 1 second
    if (millis() - state_entry_time > 1000) {
        game_transition_to(STATE_VALIDATING);
    }
}

static void nfc_detected_exit() {
    Serial.println("[NFC_DETECTED] Exit");
}

// =============================================================================
// STATE IMPLEMENTATIONS: VALIDATING
// =============================================================================

static void validating_enter() {
    Serial.println("[VALIDATING] Enter: Validating UID");
}

static void validating_update() {
    // Transition after 200ms
    if (millis() - state_entry_time > 200) {
        game_transition_to(STATE_SELECTING);
    }
}

static void validating_exit() {
    Serial.println("[VALIDATING] Exit");
}

// =============================================================================
// STATE IMPLEMENTATIONS: SELECTING
// =============================================================================

static void selecting_enter() {
    Serial.println("[SELECTING] Enter: Choosing activity");
}

static void selecting_update() {
    // Transition after 500ms
    if (millis() - state_entry_time > 500) {
        game_transition_to(STATE_PLAYING_ACTIVITY);
    }
}

static void selecting_exit() {
    Serial.println("[SELECTING] Exit");
}

// =============================================================================
// STATE IMPLEMENTATIONS: PLAYING_ACTIVITY
// =============================================================================

static void playing_enter() {
    Serial.println("[PLAYING] Enter: Breathing animation, audio playing");
    led_set_animation(LED_BREATHING);
}

static void playing_update() {
    // Transition after 5 seconds
    if (millis() - state_entry_time > 5000) {
        game_transition_to(STATE_ACTIVITY_COMPLETE);
    }
}

static void playing_exit() {
    Serial.println("[PLAYING] Exit");
}

// =============================================================================
// STATE IMPLEMENTATIONS: ACTIVITY_COMPLETE
// =============================================================================

static void complete_enter() {
    Serial.println("[COMPLETE] Enter: Sparkle animation");
    led_set_animation(LED_SPARKLE);
}

static void complete_update() {
    // Transition after 2 seconds back to IDLE
    if (millis() - state_entry_time > 2000) {
        game_transition_to(STATE_IDLE);
    }
}

static void complete_exit() {
    Serial.println("[COMPLETE] Exit");
}

// =============================================================================
// STATE IMPLEMENTATIONS: ERROR
// =============================================================================

static void error_enter() {
    Serial.println("[ERROR] Enter: Red pulsing (eco mode)");
    led_set_animation(LED_ERROR);
    led_set_brightness(POWER_MODE_ECO);
}

static void error_update() {
    // Transition after 5 seconds back to IDLE
    if (millis() - state_entry_time > 5000) {
        game_transition_to(STATE_IDLE);
    }
}

static void error_exit() {
    Serial.println("[ERROR] Exit");
}

// =============================================================================
// STATE IMPLEMENTATIONS: LOW_BATTERY
// =============================================================================

static void low_battery_enter() {
    Serial.println("[LOW_BATTERY] Enter: Red pulsing (critical brightness)");
    led_set_animation(LED_ERROR);
    led_set_brightness(POWER_MODE_CRITICAL);
}

static void low_battery_update() {
    // Stay in state (placeholder - battery recovery logic goes here)
}

static void low_battery_exit() {
    Serial.println("[LOW_BATTERY] Exit");
}

// =============================================================================
// TEST FUNCTION
// =============================================================================

void game_test_transitions() {
    if (test_active) {
        Serial.println("Test already running, please wait...");
        return;
    }

    Serial.println("\n========== STATE MACHINE TEST START ==========");
    test_active = true;
    test_state = TEST_START;
    test_step_start_time = millis();
    Serial.println("\n1. Starting from IDLE");
}

static void game_test_update() {
    if (!test_active) {
        return;
    }

    unsigned long elapsed = millis() - test_step_start_time;

    switch (test_state) {
        case TEST_START:
            if (elapsed >= 2000) {
                Serial.println("\n2. Simulating NFC detection");
                game_transition_to(STATE_NFC_DETECTED);
                test_state = TEST_NFC_STEP;
                test_step_start_time = millis();
            }
            break;

        case TEST_NFC_STEP:
            if (elapsed >= 2000) {
                Serial.println("\n3. Validating UID");
                game_transition_to(STATE_VALIDATING);
                test_state = TEST_VALIDATING_STEP;
                test_step_start_time = millis();
            }
            break;

        case TEST_VALIDATING_STEP:
            if (elapsed >= 1000) {
                Serial.println("\n4. Selecting activity");
                game_transition_to(STATE_SELECTING);
                test_state = TEST_SELECTING_STEP;
                test_step_start_time = millis();
            }
            break;

        case TEST_SELECTING_STEP:
            if (elapsed >= 1000) {
                Serial.println("\n5. Playing activity");
                game_transition_to(STATE_PLAYING_ACTIVITY);
                test_state = TEST_PLAYING_STEP;
                test_step_start_time = millis();
            }
            break;

        case TEST_PLAYING_STEP:
            if (elapsed >= 2000) {
                Serial.println("\n6. Activity complete");
                game_transition_to(STATE_ACTIVITY_COMPLETE);
                test_state = TEST_COMPLETE_STEP;
                test_step_start_time = millis();
            }
            break;

        case TEST_COMPLETE_STEP:
            if (elapsed >= 2000) {
                Serial.println("\n7. Back to idle");
                game_transition_to(STATE_IDLE);
                test_state = TEST_IDLE_RETURN_STEP;
                test_step_start_time = millis();
            }
            break;

        case TEST_IDLE_RETURN_STEP:
            if (elapsed >= 2000) {
                Serial.println("\n8. Testing error state");
                game_transition_to(STATE_ERROR);
                test_state = TEST_ERROR_STEP;
                test_step_start_time = millis();
            }
            break;

        case TEST_ERROR_STEP:
            if (elapsed >= 2000) {
                Serial.println("\n9. Returning to idle");
                game_transition_to(STATE_IDLE);
                test_state = TEST_FINISH_STEP;
                test_step_start_time = millis();
            }
            break;

        case TEST_FINISH_STEP:
            if (elapsed >= 1000) {
                Serial.println("\n========== STATE MACHINE TEST COMPLETE ==========");
                Serial.println("Press 't' to run test again\n");
                test_active = false;
                test_state = TEST_IDLE;
            }
            break;

        default:
            // Safety: reset test if in invalid state
            test_active = false;
            test_state = TEST_IDLE;
            break;
    }
}
