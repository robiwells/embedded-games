#include <Arduino.h>
#include "config.h"
#include "hardware.h"
#include "led_controller.h"
#include "game.h"
#include <esp_task_wdt.h>

void setup() {
    hardware_init();
    led_init();
    game_init();

    Serial.println("\nPress 't' to run state machine test");
}

void loop() {
    esp_task_wdt_reset();

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
