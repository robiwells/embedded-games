/**
 * @file test_main.cpp
 * @brief Unit tests for game timing logic (state timeouts and durations)
 *
 * Tests verify:
 * - State auto-transitions occur at correct times
 * - Timing calculations work correctly
 * - States that should stay don't auto-transition
 */

#include <unity.h>

// Include mocks BEFORE production code
#include "../../test/mocks/platform_hal_fake.cpp"

// Include production code directly (test-only pattern)
#include "../../src/game.cpp"
#include "../../src/led_controller.cpp"

// Test utilities
void setUp(void) {
    fake_reset();
    platform_hal = &platform_fake;
    game_init();
    fake_set_millis(0);
}

void tearDown(void) {
    // Cleanup after each test
}

// =============================================================================
// State Timeout Tests
// =============================================================================

void test_idle_auto_transitions_after_5_seconds(void) {
    fake_set_millis(0);
    game_transition_to(STATE_IDLE);

    // Advance time by 5000ms - should NOT transition yet (> 5000 needed)
    fake_advance_time(5000);
    game_update();
    TEST_ASSERT_EQUAL(STATE_IDLE, game_get_current_state());

    // Advance 1 more ms (total 5001ms) - should transition
    fake_advance_time(1);
    game_update();
    TEST_ASSERT_EQUAL(STATE_NFC_DETECTED, game_get_current_state());
}

void test_nfc_detected_transitions_after_1_second(void) {
    fake_set_millis(0);
    game_transition_to(STATE_NFC_DETECTED);

    // Not yet (> 1000 needed)
    fake_advance_time(1000);
    game_update();
    TEST_ASSERT_EQUAL(STATE_NFC_DETECTED, game_get_current_state());

    // Now
    fake_advance_time(1);
    game_update();
    TEST_ASSERT_EQUAL(STATE_VALIDATING, game_get_current_state());
}

void test_validating_transitions_after_200ms(void) {
    fake_set_millis(0);
    game_transition_to(STATE_VALIDATING);

    // Not yet (> 200 needed)
    fake_advance_time(200);
    game_update();
    TEST_ASSERT_EQUAL(STATE_VALIDATING, game_get_current_state());

    // Now
    fake_advance_time(1);
    game_update();
    TEST_ASSERT_EQUAL(STATE_SELECTING, game_get_current_state());
}

void test_selecting_transitions_after_500ms(void) {
    fake_set_millis(0);
    game_transition_to(STATE_SELECTING);

    fake_advance_time(500);
    game_update();
    TEST_ASSERT_EQUAL(STATE_SELECTING, game_get_current_state());

    fake_advance_time(1);
    game_update();
    TEST_ASSERT_EQUAL(STATE_PLAYING_ACTIVITY, game_get_current_state());
}

void test_playing_transitions_after_5_seconds(void) {
    fake_set_millis(0);
    game_transition_to(STATE_PLAYING_ACTIVITY);

    fake_advance_time(5000);
    game_update();
    TEST_ASSERT_EQUAL(STATE_PLAYING_ACTIVITY, game_get_current_state());

    fake_advance_time(1);
    game_update();
    TEST_ASSERT_EQUAL(STATE_ACTIVITY_COMPLETE, game_get_current_state());
}

void test_activity_complete_transitions_after_2_seconds(void) {
    fake_set_millis(0);
    game_transition_to(STATE_ACTIVITY_COMPLETE);

    fake_advance_time(2000);
    game_update();
    TEST_ASSERT_EQUAL(STATE_ACTIVITY_COMPLETE, game_get_current_state());

    fake_advance_time(1);
    game_update();
    TEST_ASSERT_EQUAL(STATE_IDLE, game_get_current_state());
}

void test_error_transitions_after_5_seconds(void) {
    fake_set_millis(0);
    game_transition_to(STATE_ERROR);

    fake_advance_time(5000);
    game_update();
    TEST_ASSERT_EQUAL(STATE_ERROR, game_get_current_state());

    fake_advance_time(1);
    game_update();
    TEST_ASSERT_EQUAL(STATE_IDLE, game_get_current_state());
}

void test_low_battery_does_not_auto_transition(void) {
    fake_set_millis(0);
    game_transition_to(STATE_LOW_BATTERY);

    // Advance time significantly - should stay in state
    fake_advance_time(10000);
    game_update();
    TEST_ASSERT_EQUAL(STATE_LOW_BATTERY, game_get_current_state());
}

// =============================================================================
// Timing Edge Cases
// =============================================================================

void test_timing_survives_millis_overflow(void) {
    // Simulate millis() overflow (wraps at ~50 days)
    // Set time near overflow point
    fake_set_millis(0xFFFFFF00); // Near max uint32_t
    game_transition_to(STATE_NFC_DETECTED);

    // Advance past overflow (need > 1000ms for NFC_DETECTED timeout)
    fake_advance_time(1100); // Wraps past overflow and triggers timeout
    game_update();

    // Should still transition correctly despite overflow
    TEST_ASSERT_EQUAL(STATE_VALIDATING, game_get_current_state());
}

void test_rapid_transitions(void) {
    // Test that rapidly advancing time through multiple states works
    fake_set_millis(0);
    game_transition_to(STATE_NFC_DETECTED);

    // Advance through multiple state timeouts (need > threshold)
    fake_advance_time(1001);  // NFC -> VALIDATING
    game_update();
    TEST_ASSERT_EQUAL(STATE_VALIDATING, game_get_current_state());

    fake_advance_time(201);   // VALIDATING -> SELECTING
    game_update();
    TEST_ASSERT_EQUAL(STATE_SELECTING, game_get_current_state());

    fake_advance_time(501);   // SELECTING -> PLAYING
    game_update();
    TEST_ASSERT_EQUAL(STATE_PLAYING_ACTIVITY, game_get_current_state());
}

void test_state_entry_time_resets_on_transition(void) {
    fake_set_millis(0);
    game_transition_to(STATE_IDLE);

    // Advance time but not enough to trigger timeout
    fake_advance_time(3000);

    // Manual transition to another state
    game_transition_to(STATE_ERROR);

    // ERROR state timeout is 5 seconds from ITS entry
    // Advance 5000ms - should NOT timeout yet (> 5000 needed)
    fake_advance_time(5000);
    game_update();
    TEST_ASSERT_EQUAL(STATE_ERROR, game_get_current_state());

    // Advance 1 more ms - should timeout
    fake_advance_time(1);
    game_update();
    TEST_ASSERT_EQUAL(STATE_IDLE, game_get_current_state());
}

// =============================================================================
// Test Runner
// =============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // State timeout tests
    RUN_TEST(test_idle_auto_transitions_after_5_seconds);
    RUN_TEST(test_nfc_detected_transitions_after_1_second);
    RUN_TEST(test_validating_transitions_after_200ms);
    RUN_TEST(test_selecting_transitions_after_500ms);
    RUN_TEST(test_playing_transitions_after_5_seconds);
    RUN_TEST(test_activity_complete_transitions_after_2_seconds);
    RUN_TEST(test_error_transitions_after_5_seconds);
    RUN_TEST(test_low_battery_does_not_auto_transition);

    // Timing edge cases
    RUN_TEST(test_timing_survives_millis_overflow);
    RUN_TEST(test_rapid_transitions);
    RUN_TEST(test_state_entry_time_resets_on_transition);

    return UNITY_END();
}
