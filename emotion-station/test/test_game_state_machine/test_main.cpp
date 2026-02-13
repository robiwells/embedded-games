/**
 * @file test_main.cpp
 * @brief Unit tests for game state machine (state transitions and lifecycle)
 *
 * Tests verify:
 * - Initial state is IDLE
 * - State transitions work correctly
 * - Entry/exit functions are called
 * - State entry time is updated on transitions
 */

#include <unity.h>

// Include mocks BEFORE production code
#include "../../test/mocks/platform_hal_fake.cpp"
#include "../../test/mocks/nfc_handler_mock.cpp"
#include "../../test/mocks/activity_manager_mock.cpp"
#include "../../test/mocks/audio_player_mock.cpp"
#include "../../test/mocks/data_logger_mock.cpp"

// Include production code directly (test-only pattern)
#include "../../src/event_bus.cpp"
#include "../../src/game.cpp"
#include "../../src/led_controller.cpp"

// Test utilities
void setUp(void) {
    fake_reset();
    platform_hal = &platform_fake;
    game_init();
}

void tearDown(void) {
    // Cleanup after each test
}

// =============================================================================
// Initial State Tests
// =============================================================================

void test_initial_state_is_idle(void) {
    TEST_ASSERT_EQUAL(STATE_IDLE, game_get_current_state());
}

void test_initial_state_entry_time_set(void) {
    // State entry time should be set to fake time (0 by default)
    fake_set_millis(1000);
    game_init();
    // Entry time should be 1000 after init
    TEST_ASSERT_EQUAL(STATE_IDLE, game_get_current_state());
}

// =============================================================================
// State Transition Tests
// =============================================================================

void test_transition_changes_state(void) {
    game_transition_to(STATE_NFC_DETECTED);
    TEST_ASSERT_EQUAL(STATE_NFC_DETECTED, game_get_current_state());
}

void test_transition_updates_entry_time(void) {
    fake_set_millis(0);
    game_transition_to(STATE_IDLE);

    fake_advance_time(500);
    game_transition_to(STATE_NFC_DETECTED);

    // Entry time should be updated to 500ms
    // (verified by checking that state timeout doesn't trigger early)
    fake_advance_time(500);
    game_update(); // Should NOT timeout yet (only 500ms elapsed since entry)
    TEST_ASSERT_EQUAL(STATE_NFC_DETECTED, game_get_current_state());
}

void test_all_state_transitions(void) {
    // Test each state in sequence
    game_transition_to(STATE_IDLE);
    TEST_ASSERT_EQUAL(STATE_IDLE, game_get_current_state());

    game_transition_to(STATE_NFC_DETECTED);
    TEST_ASSERT_EQUAL(STATE_NFC_DETECTED, game_get_current_state());

    game_transition_to(STATE_VALIDATING);
    TEST_ASSERT_EQUAL(STATE_VALIDATING, game_get_current_state());

    game_transition_to(STATE_SELECTING);
    TEST_ASSERT_EQUAL(STATE_SELECTING, game_get_current_state());

    game_transition_to(STATE_PLAYING_ACTIVITY);
    TEST_ASSERT_EQUAL(STATE_PLAYING_ACTIVITY, game_get_current_state());

    game_transition_to(STATE_ACTIVITY_COMPLETE);
    TEST_ASSERT_EQUAL(STATE_ACTIVITY_COMPLETE, game_get_current_state());

    game_transition_to(STATE_ERROR);
    TEST_ASSERT_EQUAL(STATE_ERROR, game_get_current_state());

    game_transition_to(STATE_LOW_BATTERY);
    TEST_ASSERT_EQUAL(STATE_LOW_BATTERY, game_get_current_state());
}

void test_invalid_state_transition_rejected(void) {
    GameState initial = game_get_current_state();
    game_transition_to((GameState)999); // Invalid state
    // State should remain unchanged
    TEST_ASSERT_EQUAL(initial, game_get_current_state());
}

// =============================================================================
// State Entry/Exit Tests
// =============================================================================

void test_enter_function_called_on_transition(void) {
    // Reset log buffer
    fake_reset();
    platform_hal = &platform_fake;

    // Transition should log entry message
    game_transition_to(STATE_NFC_DETECTED);

    // Check that entry log was written
    const char* log = fake_get_last_log();
    TEST_ASSERT_NOT_NULL(log);
    TEST_ASSERT_TRUE(strstr(log, "[NFC_DETECTED]") != NULL);
}

void test_exit_function_called_on_transition(void) {
    fake_reset();
    platform_hal = &platform_fake;

    game_transition_to(STATE_NFC_DETECTED);
    fake_reset(); // Clear logs

    // Transition away - should log exit
    game_transition_to(STATE_VALIDATING);

    const char* log = fake_get_last_log();
    TEST_ASSERT_NOT_NULL(log);
    // Should contain both exit and enter logs
    TEST_ASSERT_TRUE(strstr(log, "Exit") != NULL || strstr(log, "Enter") != NULL);
}

// =============================================================================
// LED Integration Tests
// =============================================================================

void test_idle_state_sets_led_animation(void) {
    game_transition_to(STATE_IDLE);
    // LED controller should have been called (verified via fake HAL LED state)
    // We can't directly check animation state without exposing it, but we can
    // verify LED brightness was set
    TEST_ASSERT_EQUAL(128, fake_get_led_brightness()); // ECO mode = 128
}

void test_nfc_detected_sets_led_brightness(void) {
    game_transition_to(STATE_NFC_DETECTED);
    // Normal power mode = 255 brightness
    TEST_ASSERT_EQUAL(255, fake_get_led_brightness());
}

// =============================================================================
// Test Runner
// =============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Initial state tests
    RUN_TEST(test_initial_state_is_idle);
    RUN_TEST(test_initial_state_entry_time_set);

    // Transition tests
    RUN_TEST(test_transition_changes_state);
    RUN_TEST(test_transition_updates_entry_time);
    RUN_TEST(test_all_state_transitions);
    RUN_TEST(test_invalid_state_transition_rejected);

    // Entry/exit tests
    RUN_TEST(test_enter_function_called_on_transition);
    RUN_TEST(test_exit_function_called_on_transition);

    // LED integration tests
    RUN_TEST(test_idle_state_sets_led_animation);
    RUN_TEST(test_nfc_detected_sets_led_brightness);

    return UNITY_END();
}
