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
#endif
}

void loop() {
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
#endif
    }
}
