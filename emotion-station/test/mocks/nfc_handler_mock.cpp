/**
 * @file nfc_handler_mock.cpp
 * @brief Mock NFC handler for unit testing
 *
 * Provides controllable test doubles for NFC functionality without hardware.
 */

#include "nfc_handler.h"
#include "platform_hal.h"
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

void nfc_test() {
    // No-op in mock
}

// UID → Mood mapping table (duplicated from nfc_handler.cpp for mock isolation)
static const NfcMoodMapping nfc_mappings[] = {
    {{0x04, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6}, MOOD_HAPPY,     "Happy"},
    {{0x04, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0xA1}, MOOD_SAD,       "Sad"},
    {{0x04, 0xC3, 0xD4, 0xE5, 0xF6, 0xA1, 0xB2}, MOOD_CALM,      "Calm"},
    {{0x04, 0xD4, 0xE5, 0xF6, 0xA1, 0xB2, 0xC3}, MOOD_ENERGETIC, "Energetic"},
    {{0x04, 0xE5, 0xF6, 0xA1, 0xB2, 0xC3, 0xD4}, MOOD_ANXIOUS,   "Anxious"},
    {{0x04, 0xF6, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5}, MOOD_ANGRY,     "Angry"},
};
#define NUM_MAPPINGS (sizeof(nfc_mappings) / sizeof(nfc_mappings[0]))

MoodCategory nfc_validate_uid(const uint8_t uid[7]) {
    for (uint8_t i = 0; i < NUM_MAPPINGS; i++) {
        bool match = true;
        for (uint8_t j = 0; j < 7; j++) {
            if (uid[j] != nfc_mappings[i].uid[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            return nfc_mappings[i].mood;
        }
    }
    return MOOD_UNKNOWN;
}

const char* nfc_get_mood_name(MoodCategory mood) {
    for (uint8_t i = 0; i < NUM_MAPPINGS; i++) {
        if (nfc_mappings[i].mood == mood) {
            return nfc_mappings[i].display_name;
        }
    }
    return "Unknown";
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
