#include <Arduino.h>
#include "config.h"
#include "platform_hal.h"
#include "hardware.h"
#include "led_controller.h"
#include "session_manager.h"
#include "game.h"
#include "event_bus.h"
#include "nfc_handler.h"
#include "activity_manager.h"
#include "activity_repository.h"
#include "audio_player.h"
#include "time_service.h"
#include <esp_task_wdt.h>
#include <esp_system.h>

void setup() {
    // Initialise HAL first (must be done before any other modules)
    platform_hal = &platform_real;

    // Event bus must be initialised before game_init() which subscribes to events
    event_bus_init();

    // LED and game must be ready before hardware_init() may call handle_error()
    led_init();
    led_controller_init();
    session_manager_init();
    game_init();

    // Set activity repository before hardware_init() calls activity_manager_init()
#ifdef WOKWI_SIMULATION
    activity_repository = activity_repository_wokwi;
#else
    activity_repository = activity_repository_sd;
#endif

#ifndef WOKWI_SIMULATION
    esp_reset_reason_t reset_reason = esp_reset_reason();
#endif

    HardwareInitResult hw = hardware_init();
    if (!hw.nfc_ok)   handle_error(ERROR_NFC_INIT_FAILED);
    if (!hw.sd_ok)    handle_error(ERROR_SD_INIT_FAILED);
    if (!hw.audio_ok) handle_error(ERROR_AUDIO_INIT_FAILED);

    time_service_init();

#ifndef WOKWI_SIMULATION
    if (reset_reason == ESP_RST_WDT || reset_reason == ESP_RST_TASK_WDT) {
        HAL_log_println("WARNING: Watchdog reset detected!");
        handle_error(ERROR_WATCHDOG_RESET);
    }
#endif

    // Publish boot complete event
    event_bus_publish(BOOT_COMPLETE, PRIORITY_LOW, NULL, 0);

    HAL_log_println("\nCommands:");
    HAL_log_println("  t - Test state machine");
    HAL_log_println("  n - Test NFC reader");
#ifndef WOKWI_SIMULATION
    HAL_log_println("  SETTIME YYYY-MM-DD HH:MM:SS - Set RTC time");
#endif
#ifdef WOKWI_SIMULATION
    HAL_log_println("  p - Present mock NFC token");
    HAL_log_println("  r - Remove mock NFC token");
    HAL_log_println("  0 - Set token mood: Happy");
    HAL_log_println("  1 - Set token mood: Sad");
    HAL_log_println("  2 - Set token mood: Calm");
    HAL_log_println("  3 - Set token mood: Energetic");
    HAL_log_println("  4 - Set token mood: Anxious");
    HAL_log_println("  5 - Set token mood: Angry");
    HAL_log_println("  M - Jump to Morning (06:00)");
    HAL_log_println("  A - Jump to Afternoon (12:00)");
    HAL_log_println("  E - Jump to Evening (17:00)");
    HAL_log_println("  B - Jump to Bedtime (21:00)");
    HAL_log_println("  b - Cycle battery voltage (4.2 -> 3.5 -> 3.4 -> 3.3 -> ...)");
    HAL_log_println("  s - Skip current audio (advance to ACTIVITY_COMPLETE)");
#endif
}

void loop() {
    uint32_t loop_start = micros();

    HAL_watchdog_reset();

    hardware_heartbeat();
    battery_manager_update();
    led_update();
    audio_loop();
    nfc_update();
    nfc_test_update();
    game_update();
    event_bus_process(); // dispatch events published during this iteration

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
#ifndef WOKWI_SIMULATION
        else if (cmd == 'S') {
            // Read rest of line: expect "ETTIME YYYY-MM-DD HH:MM:SS"
            String rest = Serial.readStringUntil('\n');
            String full = String("S") + rest;
            full.trim();
            int year, month, day, hour, min, sec;
            if (sscanf(full.c_str(), "SETTIME %d-%d-%d %d:%d:%d",
                       &year, &month, &day, &hour, &min, &sec) == 6) {
                time_service_set(year, month, day, hour, min, sec);
            } else {
                HAL_log_println("ERROR: Format is SETTIME YYYY-MM-DD HH:MM:SS");
            }
        }
#endif
#ifdef WOKWI_SIMULATION
        else if (cmd == 'p' || cmd == 'r' || (cmd >= '0' && cmd <= '5')) {
            nfc_handle_mock_command(cmd);
        } else if (cmd == 'M') { activity_set_sim_time(6);  HAL_log_println("Time: Morning");   }
        else if (cmd == 'A') { activity_set_sim_time(12); HAL_log_println("Time: Afternoon"); }
        else if (cmd == 'E') { activity_set_sim_time(17); HAL_log_println("Time: Evening");   }
        else if (cmd == 'B') { activity_set_sim_time(21); HAL_log_println("Time: Bedtime");   }
        else if (cmd == 'b') {
            static uint8_t b_step = 0;
            float voltages[] = {4.2f, 3.5f, 3.4f, 3.3f};
            battery_set_mock_voltage(voltages[b_step % 4]);
            b_step++;
        } else if (cmd == 's') {
            audio_stop();
            HAL_log_println("Audio: MOCK skipped");
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
        char buf[48];
        snprintf(buf, sizeof(buf), "[TIMING] New max loop time: %lu us", (unsigned long)max_loop_time);
        HAL_log_println(buf);
    }
    static uint32_t last_timing_report = 0;
    if (HAL_millis() - last_timing_report >= 10000) {
        last_timing_report = HAL_millis();
        char buf[96];
        HAL_log_println("\n=== LOOP TIMING ===");
        snprintf(buf, sizeof(buf), "Avg: %lu us", (unsigned long)(total_time / total_loops));
        HAL_log_println(buf);
        snprintf(buf, sizeof(buf), "Max: %lu us", (unsigned long)max_loop_time);
        HAL_log_println(buf);
        int32_t margin_ms = (int32_t)(4000L - (long)(max_loop_time / 1000UL));
        snprintf(buf, sizeof(buf), "WDT margin: %ld ms", (long)margin_ms);
        HAL_log_println(buf);
        HAL_log_println("===================\n");
    }
}
