#include "game.h"
#include "led_controller.h"
#include "config.h"
#include "hardware.h"
#include "event_bus.h"
#include "nfc_handler.h"
#include "platform_hal.h"
#include "activity_manager.h"
#include "session_manager.h"
#include <stdio.h>

// State variables
static GameState current_state = STATE_IDLE;
static unsigned long state_entry_time = 0;

// NFC variables (Phase 3)
static uint8_t nfc_uid[7];            // Last read UID
static MoodCategory current_mood = MOOD_UNKNOWN;  // Mood mapped from UID (Phase 4)
static Activity* selected_activity = nullptr;     // Selected activity (Phase 5)
static uint8_t attempt_count = 0;     // Retry attempt counter
static uint32_t retry_timestamp = 0;  // Timestamp for retry delays
static uint32_t debounce_start = 0;   // Token detection debounce timestamp
static bool debounce_active = false;  // True when debounce timer is running

// Session logging delegated to session_manager
static bool s_audio_completed = false;  // Set true when audio finishes naturally

// Token edge detection (Phase 3.1)
static bool previous_token_present = false;  // Track previous token state for edge detection
static bool token_processed = false;         // True if current token presentation already processed

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

void handle_error(ErrorCode error) {
    HAL_log_print("[ERROR] ");
    switch (error) {
        case ERROR_SD_INIT_FAILED:        HAL_log_println("SD card initialisation failed");        break;
        case ERROR_SD_READ_FAILED:        HAL_log_println("SD card read failed");                  break;
        case ERROR_NFC_INIT_FAILED:       HAL_log_println("NFC initialisation failed");            break;
        case ERROR_NFC_READ_TIMEOUT:      HAL_log_println("NFC read timeout after 3 attempts");    break;
        case ERROR_AUDIO_INIT_FAILED:     HAL_log_println("Audio initialisation failed");          break;
        case ERROR_AUDIO_FILE_NOT_FOUND:  HAL_log_println("Audio file not found");                 break;
        case ERROR_JSON_PARSE_FAILED:     HAL_log_println("JSON parse failed");                    break;
        case ERROR_INVALID_UID:           HAL_log_println("NFC UID not recognised");               break;
        case ERROR_NO_ACTIVITIES:         HAL_log_println("No activities available for mood");     break;
        case ERROR_BATTERY_CRITICAL:      HAL_log_println("Battery critical");                     break;
        case ERROR_WATCHDOG_RESET:        HAL_log_println("Watchdog reset detected");              break;
        default:                          HAL_log_println("Unknown error");                        break;
    }
    game_transition_to(STATE_ERROR);
}

static void on_battery_low_event(const Event* event) {
    (void)event;
    if (current_state != STATE_LOW_BATTERY && current_state != STATE_ERROR) {
        HAL_log_println("[BATTERY] BATTERY_LOW event — transitioning to LOW_BATTERY");
        game_transition_to(STATE_LOW_BATTERY);
    }
}

void game_init() {
    HAL_log_println("Game state machine initialising");
    current_state = STATE_IDLE;
    state_entry_time = HAL_millis();
    selected_activity = nullptr;
    current_mood = MOOD_UNKNOWN;
    attempt_count = 0;
    debounce_active = false;
    debounce_start = 0;
    previous_token_present = false;
    token_processed = false;

    // Subscribe to battery events
    event_bus_subscribe(BATTERY_LOW, on_battery_low_event);

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
    debounce_active = false;
    debounce_start = 0;
}

static void idle_update() {
    bool current_token_present = nfc_token_detected();

    // Detect falling edge: token was present, now removed
    if (previous_token_present && !current_token_present) {
        HAL_log_println("[IDLE] Token removed, ready for next presentation");
        token_processed = false;
        debounce_active = false;
        debounce_start = 0;
    }

    // Only trigger on rising edge (absent → present) or continuing debounce
    bool token_newly_presented = !previous_token_present && current_token_present;
    bool should_process = token_newly_presented || debounce_active;

    if (current_token_present && !token_processed && should_process) {
        if (!debounce_active) {
            debounce_active = true;
            debounce_start = HAL_millis();
            HAL_log_println("[IDLE] New token detected, debouncing...");
        } else if (HAL_millis() - debounce_start >= NFC_DEBOUNCE_TIME_MS) {
            HAL_log_println("[IDLE] Token stable, transitioning to NFC_DETECTED");
            token_processed = true;
            debounce_active = false;
            debounce_start = 0;
            game_transition_to(STATE_NFC_DETECTED);
        }
    } else if (!current_token_present && debounce_active) {
        HAL_log_println("[IDLE] Token removed during debounce");
        debounce_active = false;
        debounce_start = 0;
    }

    previous_token_present = current_token_present;
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
    current_mood = MOOD_UNKNOWN;
}

static void validating_update() {
    current_mood = nfc_validate_uid(nfc_uid);
    if (current_mood == MOOD_UNKNOWN) {
        HAL_log_println("[VALIDATING] Invalid UID - transitioning to ERROR");
        game_transition_to(STATE_ERROR);
    } else {
        HAL_log_print("[VALIDATING] Valid mood: ");
        HAL_log_println(nfc_get_mood_name(current_mood));

        // Publish NFC_DETECTED with the confirmed mood
        struct {
            uint8_t uid[7];
            MoodCategory mood;
        } nfc_data;
        for (uint8_t i = 0; i < 7; i++) {
            nfc_data.uid[i] = nfc_uid[i];
        }
        nfc_data.mood = current_mood;
        event_bus_publish(NFC_DETECTED, PRIORITY_HIGH, &nfc_data, sizeof(nfc_data));

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
    selected_activity = nullptr;
}

static void selecting_update() {
    TimeOfDay current_time = activity_get_time_of_day();
    char time_buf[48];
    snprintf(time_buf, sizeof(time_buf), "[SELECTING] Time of day: %s", activity_get_time_name(current_time));
    HAL_log_println(time_buf);
    selected_activity = activity_select(current_mood, current_time);
    if (selected_activity != nullptr) {
        HAL_log_println("[SELECTING] Activity found");
        game_transition_to(STATE_PLAYING_ACTIVITY);
    } else {
        HAL_log_println("[SELECTING] ERROR: No activity found for mood");
        game_transition_to(STATE_ERROR);
    }
}

static void selecting_exit() {
    HAL_log_println("[SELECTING] Exit");
}

// =============================================================================
// STATE IMPLEMENTATIONS: PLAYING_ACTIVITY
// =============================================================================

static void playing_enter() {
    HAL_log_println("[PLAYING] Enter: Starting audio");
    led_set_animation(LED_BREATHING);
    s_audio_completed = false;
    if (selected_activity) {
        if (!HAL_audio_play(selected_activity->file_path)) {
            HAL_log_println("[PLAYING] ERROR: Audio file not found");
            handle_error(ERROR_AUDIO_FILE_NOT_FOUND);
            return;
        }
        session_begin(current_mood, selected_activity, activity_get_time_of_day());
    }
}

static void playing_update() {
    if (!HAL_audio_is_running()) {
        HAL_log_println("[PLAYING] Audio complete");
        s_audio_completed = true;
        game_transition_to(STATE_ACTIVITY_COMPLETE);
    }
}

static void playing_exit() {
    HAL_log_println("[PLAYING] Exit: Stopping audio");
    HAL_audio_stop();
    if (session_is_active()) {
        session_end(s_audio_completed);
    }
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
    if (HAL_millis() - state_entry_time > STATE_COMPLETE_TIMEOUT_MS) {
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
    if (HAL_millis() - state_entry_time > STATE_ERROR_TIMEOUT_MS) {
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
    // battery_manager_update() (called from main loop) handles ADC polling and deep sleep.
    // Here we only check for voltage recovery to return to normal operation.
    static uint32_t last_check = 0;
    if (HAL_millis() - last_check >= BATTERY_CHECK_INTERVAL_MS) {
        last_check = HAL_millis();
        float voltage = battery_get_voltage();
        char vbuf[48];
        snprintf(vbuf, sizeof(vbuf), "[LOW_BATTERY] Voltage: %d mV", (int)(voltage * 1000));
        HAL_log_println(vbuf);
        if (voltage >= BATTERY_RECOVERY_THRESHOLD) {
            HAL_log_println("[LOW_BATTERY] Voltage recovered, returning to IDLE");
            game_transition_to(STATE_IDLE);
        }
    }
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
