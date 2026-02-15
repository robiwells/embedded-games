#include <Arduino.h>
#include "config.h"
#include "platform_hal.h"
#include "hardware.h"
#include "led_controller.h"
#include "game.h"
#include "event_bus.h"
#include "nfc_handler.h"
#include "activity_manager.h"
#include "audio_player.h"
#include <esp_task_wdt.h>
#include <esp_system.h>

void setup() {
    // Initialise HAL first (must be done before any other modules)
    platform_hal = &platform_real;

#ifndef WOKWI_SIMULATION
    esp_reset_reason_t reset_reason = esp_reset_reason();
#endif

    hardware_init();
    led_init();

    // Initialise event bus (Phase 2.5)
    event_bus_init();

    game_init();

#ifndef WOKWI_SIMULATION
    if (reset_reason == ESP_RST_WDT || reset_reason == ESP_RST_TASK_WDT) {
        Serial.println("WARNING: Watchdog reset detected!");
        handle_error(ERROR_WATCHDOG_RESET);
    }
#endif

    // Publish boot complete event
    event_bus_publish(BOOT_COMPLETE, PRIORITY_LOW, NULL, 0);

    Serial.println("\nCommands:");
    Serial.println("  t - Test state machine");
    Serial.println("  n - Test NFC reader");
#ifdef WOKWI_SIMULATION
    Serial.println("  p - Present mock NFC token");
    Serial.println("  r - Remove mock NFC token");
    Serial.println("  0 - Set token mood: Happy");
    Serial.println("  1 - Set token mood: Sad");
    Serial.println("  2 - Set token mood: Calm");
    Serial.println("  3 - Set token mood: Energetic");
    Serial.println("  4 - Set token mood: Anxious");
    Serial.println("  5 - Set token mood: Angry");
    Serial.println("  M - Jump to Morning (06:00)");
    Serial.println("  A - Jump to Afternoon (12:00)");
    Serial.println("  E - Jump to Evening (17:00)");
    Serial.println("  B - Jump to Bedtime (21:00)");
    Serial.println("  b - Cycle battery voltage (4.2 → 3.5 → 3.4 → 3.3 → ...)");
#endif
}

void loop() {
    uint32_t loop_start = micros();

    HAL_watchdog_reset();

    // Process events FIRST (before state machine update) - Phase 2.5
    event_bus_process();

    hardware_heartbeat();
    led_update();
    audio_loop();
    game_update();

    // Test triggers from serial input
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 't') {
            game_test_transitions();
        } else if (cmd == 'n') {
            nfc_test();
        } else if (cmd == 'a') {
            activity_test_load();
        }
#ifdef WOKWI_SIMULATION
        else if (cmd == 'p' || cmd == 'r' || (cmd >= '0' && cmd <= '5')) {
            nfc_handle_mock_command(cmd);
        } else if (cmd == 'M') { activity_set_sim_time(6);  Serial.println("Time: Morning");   }
        else if (cmd == 'A') { activity_set_sim_time(12); Serial.println("Time: Afternoon"); }
        else if (cmd == 'E') { activity_set_sim_time(17); Serial.println("Time: Evening");   }
        else if (cmd == 'B') { activity_set_sim_time(21); Serial.println("Time: Bedtime");   }
        else if (cmd == 'b') {
            static uint8_t b_step = 0;
            float voltages[] = {4.2f, 3.5f, 3.4f, 3.3f};
            battery_set_mock_voltage(voltages[b_step % 4]);
            b_step++;
        }
#endif
    }

    // Watchdog timing analysis
    uint32_t loop_duration = micros() - loop_start;
    static uint32_t max_loop_time = 0;
    static uint32_t total_loops = 0;
    static uint64_t total_time = 0;
    total_loops++;
    total_time += loop_duration;
    if (loop_duration > max_loop_time) {
        max_loop_time = loop_duration;
        Serial.print("[TIMING] New max loop time: ");
        Serial.print(max_loop_time);
        Serial.println(" us");
    }
    static uint32_t last_timing_report = 0;
    if (millis() - last_timing_report >= 10000) {
        last_timing_report = millis();
        Serial.println("\n=== LOOP TIMING ===");
        Serial.print("Avg: "); Serial.print((uint32_t)(total_time / total_loops)); Serial.println(" us");
        Serial.print("Max: "); Serial.print(max_loop_time); Serial.println(" us");
        Serial.print("WDT margin: "); Serial.print(4000.0f - (max_loop_time / 1000.0f)); Serial.println(" ms");
        Serial.println("===================\n");
    }
}
