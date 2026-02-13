#include "audio_player.h"
#include "config.h"
#include "platform_hal.h"

#ifdef WOKWI_SIMULATION

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

#else

// Real hardware — ESP32-audioI2S driving MAX98357A I2S DAC
#include <SD.h>
#include "Audio.h"

static Audio audio;

bool audio_init() {
    audio.setPinout(I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DOUT_PIN);
    audio.setVolume(AUDIO_VOLUME);
    HAL_log_println("Audio: init OK");
    return true;
}

void audio_loop() {
    audio.loop();
}

void audio_play(const char* file_path) {
    audio.connecttoFS(SD, file_path);
}

void audio_stop() {
    audio.stopSong();
}

bool audio_is_running() {
    return audio.isRunning();
}

void audio_test() {
    HAL_log_println("Audio: hardware test");
}

#endif
