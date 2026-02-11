#include "hardware.h"
#include "config.h"
#include <esp_task_wdt.h>

void hardware_init() {
    Serial.begin(115200);
    // Small delay for serial stability (Wokwi doesn't need this but doesn't hurt)
    delay(50);
    Serial.println("\n\n=== Emotion Station Booting ===");

    // Initialise status LED (GPIO2 is a boot strapping pin - only configure AFTER boot completes)
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);

    // Initialise watchdog timer (4 seconds)
    esp_task_wdt_init(WATCHDOG_TIMEOUT_MS / 1000, true);
    esp_task_wdt_add(NULL);

    Serial.println("Hardware initialisation complete");
}

void hardware_heartbeat() {
    static uint32_t last_beat = 0;
    if (millis() - last_beat >= 1000) {
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        last_beat = millis();
    }
}
