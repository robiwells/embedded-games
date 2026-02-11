#include <Arduino.h>
#include "config.h"
#include "platform_hal.h"
#include "hardware.h"
#include "led_controller.h"
#include "game.h"
#include "event_bus.h"
#include <esp_task_wdt.h>

void setup() {
    // Initialise HAL first (must be done before any other modules)
    platform_hal = &platform_real;

    hardware_init();
    led_init();

    // Initialise event bus (Phase 2.5)
    event_bus_init();

    game_init();

    // Publish boot complete event
    event_bus_publish(BOOT_COMPLETE, PRIORITY_LOW, NULL, 0);

    Serial.println("\nPress 't' to run state machine test");
}

void loop() {
    HAL_watchdog_reset();

    // Process events FIRST (before state machine update) - Phase 2.5
    event_bus_process();

    hardware_heartbeat();
    led_update();
    game_update();

    // Test trigger from serial input
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 't') {
            game_test_transitions();
        }
    }
}
