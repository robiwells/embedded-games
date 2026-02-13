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
