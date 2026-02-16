/**
 * @file nfc_handler.cpp
 * @brief NFC reader implementation with PN532 I2C integration (real hardware)
 *
 * Phase 3: NFC Reading with Retry Logic
 *
 * For Wokwi simulation, see src/mocks/nfc_handler_wokwi.cpp
 */

#include "nfc_handler.h"
#include "config.h"
#include "platform_hal.h"
#include "mood_registry.h"
#include <Wire.h>
#include <Adafruit_PN532.h>

// PN532 instance (I2C mode using SDA/SCL pins)
static Adafruit_PN532 nfc(I2C_SDA_PIN, I2C_SCL_PIN);
static bool nfc_ready = false;

bool nfc_init() {
    HAL_log_println("NFC: Initialising PN532...");

    nfc.begin();

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata) {
        HAL_log_println("NFC: ERROR - PN532 not found!");
        HAL_log_println("NFC: Check I2C wiring (SDA=21, SCL=22)");
        return false;
    }

    char log_buf[60];
    snprintf(log_buf, sizeof(log_buf), "NFC: Found chip PN5%02X", (versiondata >> 24) & 0xFF);
    HAL_log_println(log_buf);

    snprintf(log_buf, sizeof(log_buf), "NFC: Firmware ver. %d.%d",
             (versiondata >> 16) & 0xFF, (versiondata >> 8) & 0xFF);
    HAL_log_println(log_buf);

    nfc.SAMConfig();

    nfc_ready = true;
    HAL_log_println("NFC: Initialisation complete");
    HAL_log_println("NFC: I2C 400kHz, address 0x24");

    return true;
}

bool nfc_token_detected() {
    if (!nfc_ready) {
        return false;
    }
    uint8_t uid[7];
    uint8_t uidLength;
    return nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 0);
}

bool nfc_read_uid(uint8_t uid[7]) {
    if (!nfc_ready) {
        HAL_log_println("NFC: ERROR - Reader not initialised");
        return false;
    }

    uint8_t uidLength;
    HAL_watchdog_reset();
    bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, NFC_READ_TIMEOUT_MS);

    if (success && uidLength == 7) {
        char log_buf[80];
        snprintf(log_buf, sizeof(log_buf), "NFC: UID Read: %02X %02X %02X %02X %02X %02X %02X",
                 uid[0], uid[1], uid[2], uid[3], uid[4], uid[5], uid[6]);
        HAL_log_println(log_buf);
        return true;
    }

    if (success && uidLength != 7) {
        char log_buf[60];
        snprintf(log_buf, sizeof(log_buf), "NFC: ERROR - Wrong UID length: %d (expected 7)", uidLength);
        HAL_log_println(log_buf);
    } else {
        HAL_log_println("NFC: ERROR - Read timeout or no card present");
    }

    return false;
}

void nfc_reset_retry_state() {
    // No internal state to reset (retry state managed by game.cpp)
}

// Non-blocking NFC test state
static bool nfc_test_active = false;
static uint32_t nfc_test_start = 0;
static bool nfc_test_token_logged = false;

void nfc_test() {
    if (nfc_test_active) {
        HAL_log_println("NFC test already running");
        return;
    }
    HAL_log_println("\n=== NFC TEST ===");
    HAL_log_println("Tap an NFC tag to test reading...");
    HAL_log_println("Test window: 10 seconds");
    nfc_test_active = true;
    nfc_test_start = HAL_millis();
    nfc_test_token_logged = false;
}

void nfc_test_update() {
    if (!nfc_test_active) return;

    if (HAL_millis() - nfc_test_start >= 10000) {
        HAL_log_println("Test timeout - no tag detected");
        HAL_log_println("NFC test INCOMPLETE");
        nfc_test_active = false;
        return;
    }

    if (nfc_token_detected()) {
        if (!nfc_test_token_logged) {
            HAL_log_println("Token detected! Reading UID...");
            nfc_test_token_logged = true;
        }
        uint8_t uid[7];
        if (nfc_read_uid(uid)) {
            HAL_log_println("SUCCESS: UID read successfully");
            HAL_log_println("NFC test PASSED");
            nfc_test_active = false;
        } else {
            HAL_log_println("FAILED: Could not read UID");
        }
    } else {
        if (nfc_test_token_logged) {
            HAL_log_println("Token removed");
            nfc_test_token_logged = false;
        }
    }
}

// ============================================================================
// UID → MOOD MAPPING (sourced from mood_registry)
// ============================================================================

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
