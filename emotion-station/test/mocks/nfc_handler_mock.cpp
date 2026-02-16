/**
 * @file nfc_handler_mock.cpp
 * @brief Mock NFC handler for unit testing
 *
 * Provides controllable test doubles for NFC functionality without hardware.
 */

#include "nfc_handler.h"
#include "mood_registry.h"
#include "platform_hal.h"
#include "event_bus.h"
#include <stdio.h>
#include <string.h>

// Mock state
static bool mock_token_present = false;
static bool mock_read_success = true;
static bool nfc_ready = true;  // Start ready in tests (nfc_init not called in test setUp)
static uint8_t mock_uid[7] = {0x04, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6};

bool nfc_init() {
    nfc_ready = true;
    return true;
}

bool nfc_token_detected() {
    return nfc_ready && mock_token_present;
}

bool nfc_read_uid(uint8_t uid[7]) {
    if (!nfc_ready || !mock_token_present || !mock_read_success) {
        return false;
    }

    memcpy(uid, mock_uid, 7);
    return true;
}

void nfc_reset_retry_state() {
    // No-op in mock (state managed by game.cpp)
}

void nfc_update() {
    // No-op in mock — tests use mock_nfc_publish_token_present() directly
}

void nfc_test() {
    // No-op in mock
}

MoodCategory nfc_validate_uid(const uint8_t uid[7]) {
    const MoodDefinition* all = mood_registry_all();
    for (uint8_t i = 0; i < NUM_MOODS; i++) {
        bool match = true;
        for (uint8_t j = 0; j < 7; j++) {
            if (uid[j] != all[i].test_uid[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            return all[i].category;
        }
    }
    return MOOD_UNKNOWN;
}

const char* nfc_get_mood_name(MoodCategory mood) {
    return mood_registry_name(mood);
}

// Test control functions
void mock_nfc_set_token_present(bool present) {
    mock_token_present = present;
}

void mock_nfc_set_read_success(bool success) {
    mock_read_success = success;
}

void mock_nfc_set_uid(const uint8_t uid[7]) {
    memcpy(mock_uid, uid, 7);
}

// Directly publish NFC_TOKEN_PRESENT — simulates a debounce completing
void mock_nfc_publish_token_present() {
    event_bus_publish(NFC_TOKEN_PRESENT, PRIORITY_HIGH, nullptr, 0);
}
