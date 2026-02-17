/**
 * @file audio_player.cpp
 * @brief Audio player — real hardware (ESP32-audioI2S driving MAX98357A I2S DAC)
 *
 * For Wokwi simulation, see src/mocks/audio_player_wokwi.cpp
 */

#include "audio_player.h"
#include "config.h"
#include "platform_hal.h"
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

bool audio_play(const char* file_path) {
    return audio.connecttoFS(SD, file_path);
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
