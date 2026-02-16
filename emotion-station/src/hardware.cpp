#include "hardware.h"
#include "config.h"
#include "platform_hal.h"
#include "nfc_handler.h"
#include "activity_manager.h"
#include "audio_player.h"
#include "data_logger.h"
#include <esp_task_wdt.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <Arduino.h>

// ============================================================================
// BATTERY MONITORING (real hardware ADC)
// ============================================================================

static float last_battery_voltage = -1.0f;

float battery_get_voltage() {
    int adc_value = analogRead(BATTERY_ADC_PIN);
    float voltage = (adc_value / 4095.0f) * 3.3f * BATTERY_VOLTAGE_DIVIDER_RATIO;
    if (last_battery_voltage < 0.0f) {
        last_battery_voltage = voltage;  // First reading — no smoothing
    } else {
        last_battery_voltage = (last_battery_voltage * 0.9f) + (voltage * 0.1f);
    }
    return last_battery_voltage;
}

bool battery_is_low()      { return last_battery_voltage < BATTERY_LOW_THRESHOLD; }
bool battery_is_critical() { return last_battery_voltage < BATTERY_CRITICAL_THRESHOLD; }

// ============================================================================
// HARDWARE INITIALISATION
// ============================================================================

HardwareInitResult hardware_init() {
    Serial.begin(115200);
    // Small delay for serial stability
    delay(50);
    Serial.println("\n\n=== Emotion Station Booting ===");

    HardwareInitResult result = {true, true, true};

    // Initialise status LED (GPIO2 is a boot strapping pin - only configure AFTER boot completes)
    HAL_pin_mode(STATUS_LED_PIN, OUTPUT);
    HAL_digital_write(STATUS_LED_PIN, LOW);

    // Initialise watchdog timer (4 seconds)
    esp_task_wdt_init(WATCHDOG_TIMEOUT_MS / 1000, true);
    esp_task_wdt_add(NULL);

    // Initialise NFC reader (Phase 3)
    if (!nfc_init()) {
        result.nfc_ok = false;
    }

    // Initialise activity manager (Phase 5) — mounts SD card
    if (!activity_manager_init()) {
        result.sd_ok = false;
    }

    // Initialise audio (Phase 7)
    if (!audio_init()) {
        result.audio_ok = false;
    }

    // Initialise data logger (Phase 8) — only if SD is available, non-critical
    if (result.sd_ok) {
        if (!logger_init()) {
            HAL_log_println("WARNING: Logger init failed (non-critical)");
        }
    }

    pinMode(BATTERY_ADC_PIN, INPUT);
    analogSetAttenuation(ADC_11db);
    battery_get_voltage();  // Prime the smoothing filter
    HAL_log_print("[BATTERY] Initial voltage: ");
    Serial.print(last_battery_voltage);  // float — Serial only
    Serial.println("V");

    HAL_log_println("Hardware initialisation complete");
    return result;
}

void hardware_enter_deep_sleep() {
    HAL_log_println("[DEEP_SLEEP] Entering deep sleep mode...");
    Serial.flush();
    esp_deep_sleep_start();
}

void hardware_heartbeat() {
    static uint32_t last_beat = 0;
    if (HAL_millis() - last_beat >= 1000) {
        HAL_digital_write(STATUS_LED_PIN, !HAL_digital_read(STATUS_LED_PIN));
        last_beat = HAL_millis();
    }
}
