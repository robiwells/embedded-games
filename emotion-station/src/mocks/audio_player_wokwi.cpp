/**
 * @file audio_player_wokwi.cpp
 * @brief Wokwi simulation audio player — replaces audio_player.cpp in wokwi env
 */

#include "audio_player.h"
#include "platform_hal.h"
#include <stdio.h>

// Mock playback — simulates a 5-second audio duration
static bool mock_playing = false;
static unsigned long mock_start_time = 0;
static const unsigned long mock_duration = 5000;

bool audio_init() {
    HAL_log_println("Audio: MOCK init OK");
    return true;
}

void audio_loop() {
    if (mock_playing && HAL_millis() - mock_start_time >= mock_duration) {
        mock_playing = false;
        HAL_log_println("Audio: MOCK playback complete");
    }
}

void audio_play(const char* file_path) {
    char buf[80];
    snprintf(buf, sizeof(buf), "Audio: MOCK play: %s", file_path);
    HAL_log_println(buf);
    mock_playing = true;
    mock_start_time = HAL_millis();
}

void audio_stop() {
    mock_playing = false;
}

bool audio_is_running() {
    return mock_playing;
}

void audio_test() {
    HAL_log_println("Audio: MOCK test");
}
