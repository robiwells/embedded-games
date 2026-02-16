/**
 * @file test_main.cpp
 * @brief Unit tests for event bus subscribe/publish/process
 */

#include <unity.h>

// Include mocks BEFORE production code
#include "../../test/mocks/platform_hal_fake.cpp"

// Include production code under test
#include "../../src/event_bus/event_bus.cpp"

// ========= Test state =========

static int s_received_count = 0;
static EventType s_last_type = (EventType)255;

static void test_subscriber_a(const Event* e) {
    s_received_count++;
    s_last_type = e->type;
}

static int s_subscriber_b_count = 0;
static void test_subscriber_b(const Event* e) {
    (void)e;
    s_subscriber_b_count++;
}

void setUp(void) {
    fake_reset();
    platform_hal = &platform_fake;
    event_bus_init();
    s_received_count = 0;
    s_subscriber_b_count = 0;
    s_last_type = (EventType)255;
}

void tearDown(void) {}

// =============================================================================
// Tests
// =============================================================================

void test_subscribe_and_publish_dispatches_to_subscriber(void) {
    event_bus_subscribe(BOOT_COMPLETE, test_subscriber_a);
    event_bus_publish(BOOT_COMPLETE, PRIORITY_LOW, NULL, 0);
    event_bus_process();

    TEST_ASSERT_EQUAL(1, s_received_count);
    TEST_ASSERT_EQUAL(BOOT_COMPLETE, s_last_type);
}

void test_unsubscribed_event_is_silently_dropped(void) {
    // Subscribe to BOOT_COMPLETE but publish NFC_REMOVED — should not call subscriber
    event_bus_subscribe(BOOT_COMPLETE, test_subscriber_a);
    event_bus_publish(NFC_REMOVED, PRIORITY_NORMAL, NULL, 0);
    event_bus_process();

    TEST_ASSERT_EQUAL(0, s_received_count);
}

void test_queue_overflow_drops_oldest_event(void) {
    // Fill queue beyond capacity (8 events). The 9th publish should drop the oldest.
    // We subscribe to BOOT_COMPLETE only — publish 9 of them.
    event_bus_subscribe(BOOT_COMPLETE, test_subscriber_a);

    for (int i = 0; i < 9; i++) {
        event_bus_publish(BOOT_COMPLETE, PRIORITY_LOW, NULL, 0);
    }
    event_bus_process();

    // Should process 8 events (queue holds 8, 9th overwrote oldest)
    TEST_ASSERT_EQUAL(8, s_received_count);
}

void test_multiple_subscribers_all_receive_event(void) {
    event_bus_subscribe(AUDIO_COMPLETE, test_subscriber_a);
    event_bus_subscribe(AUDIO_COMPLETE, test_subscriber_b);

    event_bus_publish(AUDIO_COMPLETE, PRIORITY_NORMAL, NULL, 0);
    event_bus_process();

    TEST_ASSERT_EQUAL(1, s_received_count);
    TEST_ASSERT_EQUAL(1, s_subscriber_b_count);
}

// =============================================================================
// Test runner
// =============================================================================

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_subscribe_and_publish_dispatches_to_subscriber);
    RUN_TEST(test_unsubscribed_event_is_silently_dropped);
    RUN_TEST(test_queue_overflow_drops_oldest_event);
    RUN_TEST(test_multiple_subscribers_all_receive_event);

    return UNITY_END();
}
