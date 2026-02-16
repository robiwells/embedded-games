/**
 * @file nfc_handler.h
 * @brief NFC reader interface with retry logic and debouncing
 *
 * Phase 3: NFC Reading with Retry Logic
 *
 * Features:
 * - PN532 I2C integration
 * - 3-attempt retry strategy with 200ms delays
 * - 100ms debouncing for stable token detection
 * - Non-blocking token detection
 * - Blocking UID read with 1000ms timeout
 * - Mock mode for Wokwi simulation
 */

#ifndef NFC_HANDLER_H
#define NFC_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

/**
 * @brief NFC retry state tracking
 *
 * Used internally by the NFC state machine to manage retry attempts
 * and debouncing. Reset when returning to IDLE state.
 */
typedef struct {
    uint8_t attempt_count;      ///< Current retry attempt (0-2)
    uint32_t debounce_start;    ///< Timestamp when token first detected
    uint32_t retry_timestamp;   ///< Timestamp of last read attempt
    bool token_stable;          ///< True if token present for >100ms
} NfcReadState;

/**
 * @brief Initialise NFC reader hardware
 *
 * Configures PN532 for I2C communication at 400kHz.
 * Verifies firmware version and sets up for ISO14443A cards.
 *
 * @return true if initialisation successful, false otherwise
 */
bool nfc_init();

/**
 * @brief Non-blocking update — owns debounce logic and publishes NFC_TOKEN_PRESENT
 *
 * Call once per main loop iteration (before game_update()).
 * Publishes NFC_TOKEN_PRESENT when a token has been stable for NFC_DEBOUNCE_TIME_MS.
 * Publishes NFC_REMOVED when a previously-stable token leaves.
 */
void nfc_update();

/**
 * @brief Non-blocking check for NFC token presence
 *
 * Uses 0ms timeout for non-blocking detection.
 * Call from idle_update() for responsive token detection.
 *
 * @return true if token detected, false otherwise
 */
bool nfc_token_detected();

/**
 * @brief Blocking read of 7-byte NFC UID
 *
 * Uses 1000ms timeout (safe for 4000ms watchdog).
 * Call from nfc_detected_update() after debouncing.
 *
 * @param uid Output buffer for 7-byte UID (must be pre-allocated)
 * @return true if UID read successful, false on timeout/error
 */
bool nfc_read_uid(uint8_t uid[7]);

/**
 * @brief Reset retry state to initial values
 *
 * Call when entering IDLE state or after successful UID read.
 * Clears attempt counts and timestamps.
 */
void nfc_reset_retry_state();

/**
 * @brief Start non-blocking NFC reader test
 *
 * Begins a 10-second NFC test window without blocking.
 * Call via serial command 'n'. Call nfc_test_update() every loop iteration.
 */
void nfc_test();

/**
 * @brief Update non-blocking NFC test (call every loop iteration)
 *
 * Progresses the NFC test state machine. No-op when test is not active.
 */
void nfc_test_update();

/**
 * @brief Validate a 7-byte UID against the known mood mapping table
 *
 * @param uid 7-byte UID read from NFC tag
 * @return MoodCategory if found, MOOD_UNKNOWN if not in mapping table
 */
MoodCategory nfc_validate_uid(const uint8_t uid[7]);

/**
 * @brief Get display name for a mood category
 *
 * @param mood MoodCategory to look up
 * @return Display name string, or "Unknown" if not found
 */
const char* nfc_get_mood_name(MoodCategory mood);

#ifdef WOKWI_SIMULATION
/**
 * @brief Handle mock NFC commands (Wokwi simulation only)
 *
 * Commands:
 * - 'p': Present mock token (enables detection)
 * - 'r': Remove mock token (disables detection)
 *
 * @param cmd Serial command character
 */
void nfc_handle_mock_command(char cmd);
#endif

#endif // NFC_HANDLER_H
