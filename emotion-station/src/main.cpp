#include <Arduino.h>
#include "config.h"
#include "hardware.h"
#include "led_controller.h"
#include <esp_task_wdt.h>

void setup() {
    hardware_init();
    led_init();

    // LED test sequence disabled for Wokwi (would exceed watchdog timeout)
    // Uncomment for manual testing on hardware:
    // led_test_sequence();

    // Start idle animation
    led_set_animation(LED_IDLE);
    led_set_brightness(POWER_MODE_ECO);

    Serial.println("\n=== Phase 1 Ready ===");
    Serial.println("System running in IDLE mode with eco brightness");
    Serial.println("(LED test sequence skipped - run manually if needed)");
}

void loop() {
    esp_task_wdt_reset();

    hardware_heartbeat();
    led_update();

    // No delay() - fully non-blocking
}
