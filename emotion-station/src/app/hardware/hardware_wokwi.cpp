/**
 * @file hardware_wokwi.cpp
 * @brief Wokwi simulation hardware — replaces hardware.cpp in wokwi env
 */

#include "hardware.h"
#include "config.h"
#include "platform_hal.h"
#include "event_bus.h"
#include "nfc_handler.h"
#include "activity_manager.h"
#include "audio_player.h"
#include "data_logger.h"
#include <esp_task_wdt.h>
#include <Arduino.h>

static float last_battery_voltage = 4.2f;

float battery_get_voltage() { return last_battery_voltage; }
bool  battery_is_low()      { return last_battery_voltage < BATTERY_LOW_THRESHOLD; }
bool  battery_is_critical() { return last_battery_voltage < BATTERY_CRITICAL_THRESHOLD; }

void battery_set_mock_voltage(float voltage) {
    last_battery_voltage = voltage;
    HAL_log_print("[BATTERY] MOCK voltage set to ");
    Serial.print(voltage);  // float — Serial only
    Serial.println("V");
}

HardwareInitResult hardware_init() {
    Serial.begin(115200);
    delay(50);
    Serial.println("\n\n=== Emotion Station Booting (Wokwi) ===");

    HardwareInitResult result = {true, true, true};

    HAL_pin_mode(STATUS_LED_PIN, OUTPUT);
    HAL_digital_write(STATUS_LED_PIN, LOW);

    // Watchdog timer (4 seconds)
    esp_task_wdt_init(WATCHDOG_TIMEOUT_MS / 1000, true);
    esp_task_wdt_add(NULL);

    // Initialise NFC reader
    if (!nfc_init()) {
        result.nfc_ok = false;
    }

    // Initialise activity manager
    if (!activity_manager_init()) {
        result.sd_ok = false;
    }

    // Initialise audio
    if (!audio_init()) {
        result.audio_ok = false;
    }

    // Initialise data logger — non-critical
    if (!logger_init()) {
        HAL_log_println("WARNING: Logger init failed (non-critical)");
    }

    // Battery voltage reporting (mock — no ADC in Wokwi)
    HAL_log_print("[BATTERY] Initial voltage: ");
    Serial.print(last_battery_voltage);  // float — Serial only
    Serial.println("V");

    HAL_log_println("Hardware initialisation complete");
    return result;
}

void hardware_enter_deep_sleep() {
    HAL_log_println("[DEEP_SLEEP] Simulated deep sleep (Wokwi)");
}

void battery_manager_update() {
    static uint32_t last_check = 0;
    if (HAL_millis() - last_check < BATTERY_CHECK_INTERVAL_MS) return;
    last_check = HAL_millis();

    static bool was_low = false;
    bool low_now = battery_is_low();

    if (!was_low && low_now) {
        was_low = true;
        HAL_log_println("[BATTERY] Voltage low — publishing BATTERY_LOW event");
        event_bus_publish(BATTERY_LOW, PRIORITY_CRITICAL, NULL, 0);
    } else if (was_low && !low_now) {
        was_low = false;
    }

    if (battery_is_critical()) {
        HAL_log_println("[BATTERY] CRITICAL — entering deep sleep");
        hardware_enter_deep_sleep();
    }
}

void hardware_heartbeat() {
    static uint32_t last_beat = 0;
    if (HAL_millis() - last_beat >= 1000) {
        HAL_digital_write(STATUS_LED_PIN, !HAL_digital_read(STATUS_LED_PIN));
        last_beat = HAL_millis();
    }
}
