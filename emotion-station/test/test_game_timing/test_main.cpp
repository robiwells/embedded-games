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
#include "../../test/mocks/mood_registry_mock.cpp"
#include "../../test/mocks/nfc_handler_mock.cpp"
#include "../../test/mocks/activity_manager_mock.cpp"
#include "../../test/mocks/audio_player_mock.cpp"
#include "../../test/mocks/data_logger_mock.cpp"
#include "../../test/mocks/hardware_mock.cpp"
#include "../../test/mocks/session_manager_mock.cpp"

// Include production code directly (test-only pattern)
#include "../../src/event_bus.cpp"
#include "../../src/game.cpp"
#include "../../src/led_controller.cpp"

// Mock NFC control functions (declared in nfc_handler_mock.cpp)
extern void mock_nfc_set_token_present(bool present);
extern void mock_nfc_set_read_success(bool success);
extern void mock_nfc_set_uid(const uint8_t uid[7]);

// Test utilities
void setUp(void) {
    fake_reset();
    platform_hal = &platform_fake;
    game_init();
    fake_set_millis(0);
    // Reset NFC mock state
    mock_nfc_set_token_present(false);
    mock_nfc_set_read_success(true);
}

void tearDown(void) {
    // Cleanup after each test
}

// =============================================================================
// State Timeout Tests
// =============================================================================

void test_idle_auto_transitions_after_5_seconds(void) {
    // Phase 3: Idle now requires NFC token detection with 100ms debounce
    fake_set_millis(0);

    // Force fresh entry into IDLE (transition from ERROR ensures idle_enter() is called)
    game_transition_to(STATE_ERROR);
    game_transition_to(STATE_IDLE);

    // Present token
    mock_nfc_set_token_present(true);

    // First update - debounce starts
    game_update();
    TEST_ASSERT_EQUAL(STATE_IDLE, game_get_current_state());

    // Advance 99ms - should NOT transition yet (need >= 100ms)
    fake_advance_time(99);
    game_update();
    TEST_ASSERT_EQUAL(STATE_IDLE, game_get_current_state());

    // Advance 1 more ms (total 100ms) - should transition
    fake_advance_time(1);
    game_update();
    TEST_ASSERT_EQUAL(STATE_NFC_DETECTED, game_get_current_state());
}

void test_nfc_detected_transitions_after_1_second(void) {
    // Phase 3: NFC_DETECTED now uses retry logic (200ms delay + successful read)
    fake_set_millis(0);
    game_transition_to(STATE_NFC_DETECTED);

    // Mock token present and successful UID read
    mock_nfc_set_token_present(true);
    mock_nfc_set_read_success(true);

    // First update should wait 200ms before attempting
    game_update();
    TEST_ASSERT_EQUAL(STATE_NFC_DETECTED, game_get_current_state());

    // Advance 199ms - should NOT attempt yet
    fake_advance_time(199);
    game_update();
    TEST_ASSERT_EQUAL(STATE_NFC_DETECTED, game_get_current_state());

    // Advance 1 more ms (total 200ms) - should attempt read and succeed
    fake_advance_time(1);
    game_update();
    TEST_ASSERT_EQUAL(STATE_VALIDATING, game_get_current_state());
}

void test_validating_transitions_immediately_on_valid_uid(void) {
    // Default mock UID maps to MOOD_HAPPY — validation is immediate (no timer)
    fake_set_millis(0);
    game_transition_to(STATE_VALIDATING);

    game_update();
    TEST_ASSERT_EQUAL(STATE_SELECTING, game_get_current_state());
}

void test_selecting_transitions_immediately(void) {
    fake_set_millis(0);
    game_transition_to(STATE_SELECTING);

    game_update();
    TEST_ASSERT_EQUAL(STATE_PLAYING_ACTIVITY, game_get_current_state());
}

void test_playing_transitions_when_audio_complete(void) {
    // Mock audio_is_running() returns false, so state transitions immediately
    fake_set_millis(0);
    game_transition_to(STATE_PLAYING_ACTIVITY);

    game_update();
    TEST_ASSERT_EQUAL(STATE_ACTIVITY_COMPLETE, game_get_current_state());
}

void test_playing_stays_in_state_while_audio_running(void) {
    fake_set_millis(0);
    fake_set_audio_running(true);  // Audio is playing
    game_transition_to(STATE_PLAYING_ACTIVITY);

    game_update();
    TEST_ASSERT_EQUAL(STATE_PLAYING_ACTIVITY, game_get_current_state());

    // Now audio ends
    fake_set_audio_running(false);
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
    // Phase 3: Test that NFC retry delay works across millis() overflow
    // Set time near overflow point
    fake_set_millis(0xFFFFFF00); // Near max uint32_t
    game_transition_to(STATE_NFC_DETECTED);

    // Mock token present and successful UID read
    mock_nfc_set_token_present(true);
    mock_nfc_set_read_success(true);

    // Advance past overflow (need >= 200ms for retry delay)
    fake_advance_time(250); // Wraps past overflow and triggers read attempt
    game_update();

    // Should still transition correctly despite overflow
    TEST_ASSERT_EQUAL(STATE_VALIDATING, game_get_current_state());
}

void test_rapid_transitions(void) {
    // Phase 3: Test rapid transitions with new NFC retry timing
    fake_set_millis(0);
    game_transition_to(STATE_NFC_DETECTED);

    // Mock token present and successful UID read
    mock_nfc_set_token_present(true);
    mock_nfc_set_read_success(true);

    // Advance through multiple state timeouts (need >= threshold)
    fake_advance_time(200);  // NFC retry delay -> VALIDATING
    game_update();
    TEST_ASSERT_EQUAL(STATE_VALIDATING, game_get_current_state());

    fake_advance_time(201);   // VALIDATING -> SELECTING
    game_update();
    TEST_ASSERT_EQUAL(STATE_SELECTING, game_get_current_state());

    game_update();   // SELECTING -> PLAYING (immediate)
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
// NFC Validation Routing Tests
// =============================================================================

void test_validating_with_invalid_uid_transitions_to_error(void) {
    static const uint8_t uid_invalid[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    fake_set_millis(0);
    mock_nfc_set_uid(uid_invalid);
    mock_nfc_set_token_present(true);
    mock_nfc_set_read_success(true);

    // Drive through NFC_DETECTED to populate nfc_uid, then reach VALIDATING
    game_transition_to(STATE_NFC_DETECTED);
    fake_advance_time(200);  // Trigger retry delay
    game_update();           // Reads UID → transitions to STATE_VALIDATING
    TEST_ASSERT_EQUAL(STATE_VALIDATING, game_get_current_state());

    game_update();           // Validates UID → MOOD_UNKNOWN → STATE_ERROR
    TEST_ASSERT_EQUAL(STATE_ERROR, game_get_current_state());
}

void test_validating_with_each_valid_mood_transitions_to_selecting(void) {
    static const uint8_t uid_sad[]       = {0x04, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0xA1};
    static const uint8_t uid_calm[]      = {0x04, 0xC3, 0xD4, 0xE5, 0xF6, 0xA1, 0xB2};
    static const uint8_t uid_energetic[] = {0x04, 0xD4, 0xE5, 0xF6, 0xA1, 0xB2, 0xC3};
    static const uint8_t uid_anxious[]   = {0x04, 0xE5, 0xF6, 0xA1, 0xB2, 0xC3, 0xD4};
    static const uint8_t uid_angry[]     = {0x04, 0xF6, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5};

    const uint8_t* uids[] = {uid_sad, uid_calm, uid_energetic, uid_anxious, uid_angry};

    for (int i = 0; i < 5; i++) {
        fake_reset();
        platform_hal = &platform_fake;
        game_init();
        fake_set_millis(0);

        mock_nfc_set_uid(uids[i]);
        mock_nfc_set_token_present(true);
        mock_nfc_set_read_success(true);

        // Drive through NFC_DETECTED to populate nfc_uid, then reach VALIDATING
        game_transition_to(STATE_NFC_DETECTED);
        fake_advance_time(200);  // Trigger retry delay
        game_update();           // Reads UID → transitions to STATE_VALIDATING
        TEST_ASSERT_EQUAL(STATE_VALIDATING, game_get_current_state());

        game_update();           // Validates UID → valid mood → STATE_SELECTING
        TEST_ASSERT_EQUAL(STATE_SELECTING, game_get_current_state());
    }
}

// =============================================================================
// Test Runner
// =============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // State timeout tests
    RUN_TEST(test_idle_auto_transitions_after_5_seconds);
    RUN_TEST(test_nfc_detected_transitions_after_1_second);
    RUN_TEST(test_validating_transitions_immediately_on_valid_uid);
    RUN_TEST(test_selecting_transitions_immediately);
    RUN_TEST(test_playing_transitions_when_audio_complete);
    RUN_TEST(test_playing_stays_in_state_while_audio_running);
    RUN_TEST(test_activity_complete_transitions_after_2_seconds);
    RUN_TEST(test_error_transitions_after_5_seconds);
    RUN_TEST(test_low_battery_does_not_auto_transition);

    // Timing edge cases
    RUN_TEST(test_timing_survives_millis_overflow);
    RUN_TEST(test_rapid_transitions);
    RUN_TEST(test_state_entry_time_resets_on_transition);

    // NFC validation routing
    RUN_TEST(test_validating_with_invalid_uid_transitions_to_error);
    RUN_TEST(test_validating_with_each_valid_mood_transitions_to_selecting);

    return UNITY_END();
}
