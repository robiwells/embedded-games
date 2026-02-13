/**
 * @file nfc_handler.cpp
 * @brief NFC reader implementation with PN532 I2C integration
 *
 * Phase 3: NFC Reading with Retry Logic
 *
 * Two implementations:
 * - Real hardware: Uses Adafruit_PN532 library for ESP32
 * - Wokwi simulation: Mock NFC controlled by serial commands
 */

#include "nfc_handler.h"
#include "config.h"
#include "platform_hal.h"

#ifdef WOKWI_SIMULATION
// ============================================================================
// WOKWI SIMULATION MODE (Mock NFC for testing without hardware)
// ============================================================================

static bool mock_token_present = false;
static bool nfc_ready = false;
static uint8_t mock_uid[7] = {0x04, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6};

bool nfc_init() {
    HAL_log_println("NFC: MOCK MODE (Wokwi simulation)");
    HAL_log_println("Use serial commands:");
    HAL_log_println("  'p' - Present token");
    HAL_log_println("  'r' - Remove token");
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

    // Copy mock UID
    for (uint8_t i = 0; i < 7; i++) {
        uid[i] = mock_uid[i];
    }

    // Log mock UID
    HAL_log_print("NFC: MOCK UID: ");
    char hex_buf[24];
    snprintf(hex_buf, sizeof(hex_buf), "%02X %02X %02X %02X %02X %02X %02X",
             uid[0], uid[1], uid[2], uid[3], uid[4], uid[5], uid[6]);
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
    if (cmd == 'p') {
        mock_token_present = true;
        HAL_log_println("NFC: MOCK token present");
    } else if (cmd == 'r') {
        mock_token_present = false;
        HAL_log_println("NFC: MOCK token removed");
    }
}

#else
// ============================================================================
// REAL HARDWARE MODE (PN532 NFC Reader via I2C)
// ============================================================================

#include <Wire.h>
#include <Adafruit_PN532.h>

// PN532 instance (I2C mode using SDA/SCL pins)
static Adafruit_PN532 nfc(I2C_SDA_PIN, I2C_SCL_PIN);
static bool nfc_ready = false;

bool nfc_init() {
    HAL_log_println("NFC: Initialising PN532...");

    // Begin I2C communication
    nfc.begin();

    // Verify PN532 is present and get firmware version
    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata) {
        HAL_log_println("NFC: ERROR - PN532 not found!");
        HAL_log_println("NFC: Check I2C wiring (SDA=21, SCL=22)");
        return false;
    }

    // Log firmware version
    char log_buf[60];
    snprintf(log_buf, sizeof(log_buf), "NFC: Found chip PN5%02X", (versiondata >> 24) & 0xFF);
    HAL_log_println(log_buf);

    snprintf(log_buf, sizeof(log_buf), "NFC: Firmware ver. %d.%d",
             (versiondata >> 16) & 0xFF, (versiondata >> 8) & 0xFF);
    HAL_log_println(log_buf);

    // Configure for ISO14443A cards (NTAG213/215/216)
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

    // Non-blocking detection (0ms timeout)
    uint8_t uid[7];
    uint8_t uidLength;
    bool detected = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 0);

    return detected;
}

bool nfc_read_uid(uint8_t uid[7]) {
    if (!nfc_ready) {
        HAL_log_println("NFC: ERROR - Reader not initialised");
        return false;
    }

    uint8_t uidLength;

    // Blocking read with 1000ms timeout (safe for 4000ms watchdog)
    bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, NFC_READ_TIMEOUT_MS);

    if (success && uidLength == 7) {
        // Log successful UID read
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
    // This function exists for API completeness
}

void nfc_test() {
    HAL_log_println("\n=== NFC TEST ===");
    HAL_log_println("Tap an NFC tag to test reading...");
    HAL_log_println("Test window: 10 seconds");

    uint32_t start = HAL_millis();
    bool token_logged = false;

    while (HAL_millis() - start < 10000) {
        // Feed watchdog during test
        HAL_watchdog_reset();

        if (nfc_token_detected()) {
            if (!token_logged) {
                HAL_log_println("Token detected! Reading UID...");
                token_logged = true;
            }

            uint8_t uid[7];
            if (nfc_read_uid(uid)) {
                HAL_log_println("SUCCESS: UID read successfully");
                HAL_log_println("NFC test PASSED");
                return;
            } else {
                HAL_log_println("FAILED: Could not read UID");
            }
        } else {
            if (token_logged) {
                HAL_log_println("Token removed");
                token_logged = false;
            }
        }

        // Small delay to prevent tight loop
        HAL_delay(100);
    }

    HAL_log_println("Test timeout - no tag detected");
    HAL_log_println("NFC test INCOMPLETE");
}

#endif // WOKWI_SIMULATION
