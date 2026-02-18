/**
 * @file event_bus.cpp
 * @brief Event bus implementation with circular queue and publish/subscribe
 *
 * Phase 2.5: Event Bus Implementation
 * Memory Budget: 406 bytes SRAM
 * - Event queue: 8 events × 22 bytes = 176 bytes
 * - Subscriber table: 11 types × 4 subscribers × 4 bytes = 176 bytes
 * - Module overhead: ~54 bytes (indices, counts)
 */

#include "event_bus.h"
#include "platform_hal.h"
#include <stdio.h>

// ========= CIRCULAR QUEUE CONFIGURATION =========

#define EVENT_QUEUE_SIZE 8  // 8 events max (architecture spec)

// ========= STATIC STORAGE =========

// Event queue storage (8 events × 22 bytes = 176 bytes)
static Event event_queue[EVENT_QUEUE_SIZE];
static uint8_t queue_head = 0;  // Write position
static uint8_t queue_tail = 0;  // Read position
static uint8_t queue_count = 0; // Number of events in queue

// Subscriber table (11 event types × 4 subscribers max = 44 pointers = 176 bytes on ESP32)
#define MAX_SUBSCRIBERS_PER_EVENT 4
static EventCallback subscribers[NUM_EVENT_TYPES][MAX_SUBSCRIBERS_PER_EVENT];
static uint8_t subscriber_counts[NUM_EVENT_TYPES];

// ========= PUBLIC API IMPLEMENTATION =========

void event_bus_init() {
    HAL_log_println("EventBus: Initialising...");

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

    HAL_log_println("EventBus: Ready (queue size: 8 events, 176 bytes)");
}

void event_bus_publish(EventType type, EventPriority priority, const void* payload_data, uint8_t payload_size) {
    if (type >= NUM_EVENT_TYPES) {
        HAL_log_println("EventBus: ERROR - Invalid event type");
        return;
    }

    // Check if queue is full (drop oldest if necessary)
    if (queue_count >= EVENT_QUEUE_SIZE) {
        HAL_log_println("EventBus: WARNING - Queue full, dropping oldest event");
        // Drop oldest event (tail)
        queue_tail = (queue_tail + 1) % EVENT_QUEUE_SIZE;
        queue_count--;
    }

    // Create new event at head position
    Event* event = &event_queue[queue_head];
    event->type = type;
    event->priority = priority;
    event->timestamp = HAL_millis();

    // Copy payload if provided (max 16 bytes)
    if (payload_data && payload_size > 0) {
        uint8_t copy_size = payload_size > 16 ? 16 : payload_size;
        memcpy(event->payload.raw, payload_data, copy_size);
    } else {
        memset(event->payload.raw, 0, 16);
    }

    // Advance head pointer (circular)
    queue_head = (queue_head + 1) % EVENT_QUEUE_SIZE;
    queue_count++;

    // Log publication (verbose for debugging)
    char log_buf[80];
    snprintf(log_buf, sizeof(log_buf), "EventBus: Published event type %d (priority %d, queue: %d/8)",
             type, priority, queue_count);
    HAL_log_verbose(log_buf);
}

void event_bus_subscribe(EventType type, EventCallback callback) {
    if (type >= NUM_EVENT_TYPES) {
        HAL_log_println("EventBus: ERROR - Invalid event type for subscription");
        return;
    }

    if (callback == NULL) {
        HAL_log_println("EventBus: ERROR - NULL callback");
        return;
    }

    // Check if subscriber limit reached
    if (subscriber_counts[type] >= MAX_SUBSCRIBERS_PER_EVENT) {
        HAL_log_println("EventBus: ERROR - Max subscribers reached for event type");
        return;
    }

    // Add subscriber
    subscribers[type][subscriber_counts[type]] = callback;
    subscriber_counts[type]++;

    char log_buf[60];
    snprintf(log_buf, sizeof(log_buf), "EventBus: Subscribed to event type %d (%d subscribers)",
             type, subscriber_counts[type]);
    HAL_log_verbose(log_buf);
}

void event_bus_process() {
    // Process all events in queue (synchronous dispatch)
    uint32_t process_start = HAL_micros();
    uint8_t events_processed = 0;

    while (queue_count > 0) {
        // Get event from tail (oldest event first)
        Event* event = &event_queue[queue_tail];

        // Dispatch to all subscribers for this event type
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
        uint32_t process_time = HAL_micros() - process_start;

        char log_buf[80];
        snprintf(log_buf, sizeof(log_buf), "EventBus: Processed %d events in %lu µs",
                 events_processed, process_time);
        HAL_log_verbose(log_buf);

        if (process_time > 200) {
            HAL_log_verbose("EventBus: WARNING - Processing time exceeded 200µs threshold!");
        }
    }
}
