/**
 * @file test_main.cpp
 * @brief Unit tests for session manager
 */

#include <unity.h>
#include <string.h>

// Include mocks BEFORE production code
#include "../../test/mocks/platform_hal_fake.cpp"
#include "../../test/mocks/data_logger_mock.cpp"
#include "../../test/mocks/mood_registry_mock.cpp"

// Include production code under test
#include "../../src/event_bus/event_bus.cpp"
#include "../../src/session_manager/session_manager.cpp"

// Declare mock helpers
extern const SessionLog* mock_get_last_session();
extern bool mock_logger_was_called();
extern void mock_logger_reset();

// A minimal activity for testing
static Activity make_test_activity() {
    Activity a = {};
    a.id               = 1;
    a.mood             = MOOD_HAPPY;
    a.duration_seconds = 120;
    strncpy(a.name,      "Test Activity",   sizeof(a.name) - 1);
    strncpy(a.file_path, "/audio/test.mp3", sizeof(a.file_path) - 1);
    strncpy(a.type,      "breathing",       sizeof(a.type) - 1);
    for (int t = 0; t < 4; t++) a.time_flags[t] = true;
    return a;
}
static Activity s_test_activity;

void setUp(void) {
    fake_reset();
    platform_hal = &platform_fake;
    mock_logger_reset();
    s_test_activity = make_test_activity();
}

void tearDown(void) {}

// =============================================================================
// Tests
// =============================================================================

void test_session_begin_sets_active(void) {
    session_begin(MOOD_HAPPY, &s_test_activity, TIME_MORNING);
    TEST_ASSERT_TRUE(session_is_active());
}

void test_session_end_completed_true(void) {
    session_begin(MOOD_HAPPY, &s_test_activity, TIME_MORNING);
    fake_advance_time(5000);
    session_end(true);

    TEST_ASSERT_FALSE(session_is_active());
    TEST_ASSERT_TRUE(mock_logger_was_called());
    TEST_ASSERT_TRUE(mock_get_last_session()->completed);
}

void test_session_end_completed_false(void) {
    session_begin(MOOD_HAPPY, &s_test_activity, TIME_AFTERNOON);
    session_end(false);

    TEST_ASSERT_FALSE(session_is_active());
    TEST_ASSERT_TRUE(mock_logger_was_called());
    TEST_ASSERT_FALSE(mock_get_last_session()->completed);
}

void test_double_session_end_guard(void) {
    session_begin(MOOD_CALM, &s_test_activity, TIME_EVENING);
    session_end(true);

    // Second call should be a no-op (s_called stays true but should only log once)
    mock_logger_reset();
    session_end(true);  // Should do nothing

    TEST_ASSERT_FALSE(mock_logger_was_called());
}

void test_session_duration_calculation(void) {
    fake_set_millis(0);
    session_begin(MOOD_HAPPY, &s_test_activity, TIME_MORNING);
    fake_advance_time(10000);  // 10 seconds
    session_end(true);

    TEST_ASSERT_EQUAL(10, mock_get_last_session()->duration_seconds);
}

// =============================================================================
// Test runner
// =============================================================================

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_session_begin_sets_active);
    RUN_TEST(test_session_end_completed_true);
    RUN_TEST(test_session_end_completed_false);
    RUN_TEST(test_double_session_end_guard);
    RUN_TEST(test_session_duration_calculation);

    return UNITY_END();
}
