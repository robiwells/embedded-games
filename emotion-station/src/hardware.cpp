#include "hardware.h"
#include "config.h"
#include "platform_hal.h"
#include "nfc_handler.h"
#include "activity_manager.h"
#include "audio_player.h"
#include "data_logger.h"
#include <esp_task_wdt.h>
#include <Arduino.h>

void hardware_init() {
    Serial.begin(115200);
    // Small delay for serial stability (Wokwi doesn't need this but doesn't hurt)
    delay(50);
    Serial.println("\n\n=== Emotion Station Booting ===");

    // Initialise status LED (GPIO2 is a boot strapping pin - only configure AFTER boot completes)
    HAL_pin_mode(STATUS_LED_PIN, OUTPUT);
    HAL_digital_write(STATUS_LED_PIN, LOW);

    // Initialise watchdog timer (4 seconds)
    esp_task_wdt_init(WATCHDOG_TIMEOUT_MS / 1000, true);
    esp_task_wdt_add(NULL);

    // Initialise NFC reader (Phase 3)
    if (!nfc_init()) {
        HAL_log_println("WARNING: NFC initialisation failed");
        HAL_log_println("System will continue without NFC functionality");
    }

    // Initialise activity manager (Phase 5)
    if (!activity_manager_init()) {
        HAL_log_println("WARNING: Activity manager init failed - using fallback");
    }

    // Initialise audio (Phase 7)
    if (!audio_init()) {
        HAL_log_println("WARNING: Audio init failed");
    }

    // Initialise data logger (Phase 8)
    if (!logger_init()) {
        HAL_log_println("WARNING: Logger init failed");
    }

    HAL_log_println("Hardware initialisation complete");
}

void hardware_heartbeat() {
    static uint32_t last_beat = 0;
    if (HAL_millis() - last_beat >= 1000) {
        HAL_digital_write(STATUS_LED_PIN, !HAL_digital_read(STATUS_LED_PIN));
        last_beat = HAL_millis();
    }
}
