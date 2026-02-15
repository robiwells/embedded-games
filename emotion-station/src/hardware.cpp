#include "hardware.h"
#include "config.h"
#include "platform_hal.h"
#include "nfc_handler.h"
#include "activity_manager.h"
#include "audio_player.h"
#include "data_logger.h"
#include <esp_task_wdt.h>
#include <Arduino.h>

static float last_battery_voltage = 4.2f;

#ifdef WOKWI_SIMULATION

float battery_get_voltage() { return last_battery_voltage; }
bool  battery_is_low()      { return last_battery_voltage < BATTERY_LOW_THRESHOLD; }
bool  battery_is_critical() { return last_battery_voltage < BATTERY_CRITICAL_THRESHOLD; }

void battery_set_mock_voltage(float voltage) {
    last_battery_voltage = voltage;
    HAL_log_print("[BATTERY] MOCK voltage set to ");
    Serial.print(voltage);
    Serial.println("V");
}

#else // Real hardware

float battery_get_voltage() {
    int adc_value = analogRead(BATTERY_ADC_PIN);
    float voltage = (adc_value / 4095.0f) * 3.3f * BATTERY_VOLTAGE_DIVIDER_RATIO;
    last_battery_voltage = (last_battery_voltage * 0.9f) + (voltage * 0.1f);
    return last_battery_voltage;
}

bool battery_is_low()      { return last_battery_voltage < BATTERY_LOW_THRESHOLD; }
bool battery_is_critical() { return last_battery_voltage < BATTERY_CRITICAL_THRESHOLD; }

#endif // WOKWI_SIMULATION

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

#ifndef WOKWI_SIMULATION
    pinMode(BATTERY_ADC_PIN, INPUT);
    analogSetAttenuation(ADC_11db);
#endif
    battery_get_voltage();  // Prime the smoothing filter
    HAL_log_print("[BATTERY] Initial voltage: ");
    Serial.print(last_battery_voltage);
    Serial.println("V");

    HAL_log_println("Hardware initialisation complete");
}

void hardware_heartbeat() {
    static uint32_t last_beat = 0;
    if (HAL_millis() - last_beat >= 1000) {
        HAL_digital_write(STATUS_LED_PIN, !HAL_digital_read(STATUS_LED_PIN));
        last_beat = HAL_millis();
    }
}
