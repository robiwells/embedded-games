/**
 * @file nfc_handler_wokwi.cpp
 * @brief Wokwi simulation NFC handler — replaces nfc_handler.cpp in wokwi env
 */

#include "nfc_handler.h"
#include "config.h"
#include "mood_registry.h"
#include "platform_hal.h"

static bool mock_token_present = false;
static bool nfc_ready = false;
static uint8_t mock_uid_index = 0;

static const uint8_t invalid_test_uid[7] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

bool nfc_init() {
    HAL_log_println("NFC: MOCK MODE (Wokwi simulation)");
    HAL_log_println("Use serial commands:");
    HAL_log_println("  '0'-'5' - Select mood UID (0=Happy,1=Sad,2=Calm,3=Energetic,4=Anxious,5=Angry)");
    HAL_log_println("  '6'     - Select invalid UID (triggers ERROR)");
    HAL_log_println("  'p'     - Present token");
    HAL_log_println("  'r'     - Remove token");
    nfc_ready = true;
    return true;
}

bool nfc_token_detected() {
    return nfc_ready && mock_token_present;
}

bool nfc_read_uid(uint8_t uid[7]) {
    if (!nfc_ready || !mock_token_present) {
        return false;
    }

    const uint8_t* src;
    if (mock_uid_index < NUM_MOODS) {
        src = mood_registry_get((MoodCategory)mock_uid_index)->test_uid;
    } else {
        src = invalid_test_uid;
    }
    for (uint8_t i = 0; i < 7; i++) {
        uid[i] = src[i];
    }

    char hex_buf[24];
    snprintf(hex_buf, sizeof(hex_buf), "%02X %02X %02X %02X %02X %02X %02X",
             uid[0], uid[1], uid[2], uid[3], uid[4], uid[5], uid[6]);
    HAL_log_print("NFC: MOCK UID: ");
    HAL_log_println(hex_buf);

    return true;
}

void nfc_reset_retry_state() {
    // No-op in mock mode (state tracking done by game logic)
}

void nfc_test() {
    HAL_log_println("\n=== NFC TEST (MOCK MODE) ===");
    HAL_log_println("Commands:");
    HAL_log_println("  'p' - Present token");
    HAL_log_println("  'r' - Remove token");
    HAL_log_println("\nTest token by pressing 'p'");
}

void nfc_handle_mock_command(char cmd) {
    if (cmd >= '0' && cmd <= '6') {
        mock_uid_index = (uint8_t)(cmd - '0');
        const char* mood_names[] = {"Happy", "Sad", "Calm", "Energetic", "Anxious", "Angry", "Invalid"};
        HAL_log_print("NFC: UID set to index ");
        HAL_log_println(mood_names[mock_uid_index]);
    } else if (cmd == 'p') {
        mock_token_present = true;
        HAL_log_println("NFC: MOCK token present");
    } else if (cmd == 'r') {
        mock_token_present = false;
        HAL_log_println("NFC: MOCK token removed");
    }
}

// ============================================================================
// SHARED: UID → MOOD MAPPING TABLE
// ============================================================================

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
    return mood_registry_name(mood);
}
