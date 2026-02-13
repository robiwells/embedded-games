#include "game.h"
#include "led_controller.h"
#include "config.h"
#include "event_bus.h"
#include "nfc_handler.h"
#include "platform_hal.h"
#include <stdio.h>

// State variables
static GameState current_state = STATE_IDLE;
static unsigned long state_entry_time = 0;

// NFC variables (Phase 3)
static uint8_t nfc_uid[7];            // Last read UID
static uint8_t attempt_count = 0;     // Retry attempt counter
static uint32_t retry_timestamp = 0;  // Timestamp for retry delays
static uint32_t debounce_start = 0;   // Token detection debounce timestamp

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
        HAL_log_println("ERROR: Invalid state transition requested");
        return;
    }

    // Prevent no-op transitions
    if (new_state == current_state) {
        return;
    }

    GameState old_state = current_state;

    // Log state change
    HAL_log_print("State: ");
    HAL_log_print(state_names[old_state]);
    HAL_log_print(" -> ");
    HAL_log_println(state_names[new_state]);

    // Call current state's exit function
    if (state_handlers[current_state].exit != nullptr) {
        state_handlers[current_state].exit();
    }

    // Publish STATE_EXITED event (Phase 2.5)
    struct {
        GameState old_state;
        GameState new_state;
    } exit_data = {old_state, new_state};
    event_bus_publish(STATE_EXITED, PRIORITY_HIGH, &exit_data, sizeof(exit_data));

    // Update state and entry time
    current_state = new_state;
    state_entry_time = HAL_millis();

    // Call new state's enter function
    if (state_handlers[current_state].enter != nullptr) {
        state_handlers[current_state].enter();
    }

    // Publish STATE_ENTERED event (Phase 2.5)
    struct {
        GameState old_state;
        GameState new_state;
    } enter_data = {old_state, new_state};
    event_bus_publish(STATE_ENTERED, PRIORITY_HIGH, &enter_data, sizeof(enter_data));
}

// =============================================================================
// PUBLIC API FUNCTIONS
// =============================================================================

void game_init() {
    HAL_log_println("Game state machine initialising");
    current_state = STATE_IDLE;
    state_entry_time = HAL_millis();

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
    HAL_log_println("[IDLE] Enter: LED pulsing white (eco mode)");
    led_set_animation(LED_IDLE);
    led_set_brightness(POWER_MODE_ECO);
    nfc_reset_retry_state();
    debounce_start = 0; // Reset debounce timer
}

static void idle_update() {
    // Debounce token detection (100ms stable presence required)
    if (nfc_token_detected()) {
        if (debounce_start == 0) {
            // Start debounce timer (add 1 to ensure it's never 0)
            debounce_start = HAL_millis() + 1;
            HAL_log_println("[IDLE] Token detected, debouncing...");
        } else if (HAL_millis() >= debounce_start + NFC_DEBOUNCE_TIME_MS - 1) {
            HAL_log_println("[IDLE] Token stable, transitioning to NFC_DETECTED");
            debounce_start = 0;
            game_transition_to(STATE_NFC_DETECTED);
        }
    } else {
        if (debounce_start != 0) {
            HAL_log_println("[IDLE] Token removed during debounce");
        }
        debounce_start = 0;
    }
}

static void idle_exit() {
    HAL_log_println("[IDLE] Exit");
}

// =============================================================================
// STATE IMPLEMENTATIONS: NFC_DETECTED
// =============================================================================

static void nfc_detected_enter() {
    HAL_log_println("[NFC_DETECTED] Enter: Green flash");
    led_set_animation(LED_DETECTED);
    led_set_brightness(POWER_MODE_NORMAL);
    attempt_count = 0;
    retry_timestamp = HAL_millis();
}

static void nfc_detected_update() {
    // Retry logic with 200ms delays between attempts
    if (HAL_millis() - retry_timestamp < NFC_RETRY_DELAY_MS) {
        return; // Wait between attempts
    }

    // Log attempt number
    char log_buf[60];
    snprintf(log_buf, sizeof(log_buf), "[NFC_DETECTED] Attempt %d/%d",
             attempt_count + 1, NFC_READ_ATTEMPTS);
    HAL_log_println(log_buf);

    if (nfc_read_uid(nfc_uid)) {
        HAL_log_println("[NFC_DETECTED] UID read successful");

        // Publish NFC_DETECTED event with UID and placeholder mood
        struct {
            uint8_t uid[7];
            MoodCategory mood;
        } nfc_data;

        for (uint8_t i = 0; i < 7; i++) {
            nfc_data.uid[i] = nfc_uid[i];
        }
        nfc_data.mood = MOOD_HAPPY; // Placeholder - will be mapped in Phase 4

        event_bus_publish(NFC_DETECTED, PRIORITY_HIGH, &nfc_data, sizeof(nfc_data));

        game_transition_to(STATE_VALIDATING);
    } else {
        attempt_count++;
        retry_timestamp = HAL_millis();

        if (attempt_count >= NFC_READ_ATTEMPTS) {
            HAL_log_println("[NFC_DETECTED] All attempts failed, transitioning to ERROR");
            game_transition_to(STATE_ERROR);
        }
    }
}

static void nfc_detected_exit() {
    HAL_log_println("[NFC_DETECTED] Exit");
    attempt_count = 0;
}

// =============================================================================
// STATE IMPLEMENTATIONS: VALIDATING
// =============================================================================

static void validating_enter() {
    HAL_log_println("[VALIDATING] Enter: Validating UID");
}

static void validating_update() {
    // Transition after 200ms
    if (HAL_millis() - state_entry_time > 200) {
        game_transition_to(STATE_SELECTING);
    }
}

static void validating_exit() {
    HAL_log_println("[VALIDATING] Exit");
}

// =============================================================================
// STATE IMPLEMENTATIONS: SELECTING
// =============================================================================

static void selecting_enter() {
    HAL_log_println("[SELECTING] Enter: Choosing activity");
}

static void selecting_update() {
    // Transition after 500ms
    if (HAL_millis() - state_entry_time > 500) {
        game_transition_to(STATE_PLAYING_ACTIVITY);
    }
}

static void selecting_exit() {
    HAL_log_println("[SELECTING] Exit");
}

// =============================================================================
// STATE IMPLEMENTATIONS: PLAYING_ACTIVITY
// =============================================================================

static void playing_enter() {
    HAL_log_println("[PLAYING] Enter: Breathing animation, audio playing");
    led_set_animation(LED_BREATHING);
}

static void playing_update() {
    // Transition after 5 seconds
    if (HAL_millis() - state_entry_time > 5000) {
        game_transition_to(STATE_ACTIVITY_COMPLETE);
    }
}

static void playing_exit() {
    HAL_log_println("[PLAYING] Exit");
}

// =============================================================================
// STATE IMPLEMENTATIONS: ACTIVITY_COMPLETE
// =============================================================================

static void complete_enter() {
    HAL_log_println("[COMPLETE] Enter: Sparkle animation");
    led_set_animation(LED_SPARKLE);
}

static void complete_update() {
    // Transition after 2 seconds back to IDLE
    if (HAL_millis() - state_entry_time > 2000) {
        game_transition_to(STATE_IDLE);
    }
}

static void complete_exit() {
    HAL_log_println("[COMPLETE] Exit");
}

// =============================================================================
// STATE IMPLEMENTATIONS: ERROR
// =============================================================================

static void error_enter() {
    HAL_log_println("[ERROR] Enter: Red pulsing (eco mode)");
    led_set_animation(LED_ERROR);
    led_set_brightness(POWER_MODE_ECO);
}

static void error_update() {
    // Transition after 5 seconds back to IDLE
    if (HAL_millis() - state_entry_time > 5000) {
        game_transition_to(STATE_IDLE);
    }
}

static void error_exit() {
    HAL_log_println("[ERROR] Exit");
}

// =============================================================================
// STATE IMPLEMENTATIONS: LOW_BATTERY
// =============================================================================

static void low_battery_enter() {
    HAL_log_println("[LOW_BATTERY] Enter: Red pulsing (critical brightness)");
    led_set_animation(LED_ERROR);
    led_set_brightness(POWER_MODE_CRITICAL);
}

static void low_battery_update() {
    // Stay in state (placeholder - battery recovery logic goes here)
}

static void low_battery_exit() {
    HAL_log_println("[LOW_BATTERY] Exit");
}

// =============================================================================
// TEST FUNCTION
// =============================================================================

void game_test_transitions() {
    if (test_active) {
        HAL_log_println("Test already running, please wait...");
        return;
    }

    HAL_log_println("\n========== STATE MACHINE TEST START ==========");
    test_active = true;
    test_state = TEST_START;
    test_step_start_time = HAL_millis();
    HAL_log_println("\n1. Starting from IDLE");
}

static void game_test_update() {
    if (!test_active) {
        return;
    }

    unsigned long elapsed = HAL_millis() - test_step_start_time;

    switch (test_state) {
        case TEST_START:
            if (elapsed >= 2000) {
                HAL_log_println("\n2. Simulating NFC detection");
                game_transition_to(STATE_NFC_DETECTED);
                test_state = TEST_NFC_STEP;
                test_step_start_time = HAL_millis();
            }
            break;

        case TEST_NFC_STEP:
            if (elapsed >= 2000) {
                HAL_log_println("\n3. Validating UID");
                game_transition_to(STATE_VALIDATING);
                test_state = TEST_VALIDATING_STEP;
                test_step_start_time = HAL_millis();
            }
            break;

        case TEST_VALIDATING_STEP:
            if (elapsed >= 1000) {
                HAL_log_println("\n4. Selecting activity");
                game_transition_to(STATE_SELECTING);
                test_state = TEST_SELECTING_STEP;
                test_step_start_time = HAL_millis();
            }
            break;

        case TEST_SELECTING_STEP:
            if (elapsed >= 1000) {
                HAL_log_println("\n5. Playing activity");
                game_transition_to(STATE_PLAYING_ACTIVITY);
                test_state = TEST_PLAYING_STEP;
                test_step_start_time = HAL_millis();
            }
            break;

        case TEST_PLAYING_STEP:
            if (elapsed >= 2000) {
                HAL_log_println("\n6. Activity complete");
                game_transition_to(STATE_ACTIVITY_COMPLETE);
                test_state = TEST_COMPLETE_STEP;
                test_step_start_time = HAL_millis();
            }
            break;

        case TEST_COMPLETE_STEP:
            if (elapsed >= 2000) {
                HAL_log_println("\n7. Back to idle");
                game_transition_to(STATE_IDLE);
                test_state = TEST_IDLE_RETURN_STEP;
                test_step_start_time = HAL_millis();
            }
            break;

        case TEST_IDLE_RETURN_STEP:
            if (elapsed >= 2000) {
                HAL_log_println("\n8. Testing error state");
                game_transition_to(STATE_ERROR);
                test_state = TEST_ERROR_STEP;
                test_step_start_time = HAL_millis();
            }
            break;

        case TEST_ERROR_STEP:
            if (elapsed >= 2000) {
                HAL_log_println("\n9. Returning to idle");
                game_transition_to(STATE_IDLE);
                test_state = TEST_FINISH_STEP;
                test_step_start_time = HAL_millis();
            }
            break;

        case TEST_FINISH_STEP:
            if (elapsed >= 1000) {
                HAL_log_println("\n========== STATE MACHINE TEST COMPLETE ==========");
                HAL_log_println("Press 't' to run test again\n");
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
