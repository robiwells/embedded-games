#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <stdint.h>

bool audio_init();
void audio_loop();
bool audio_play(const char* file_path);
void audio_stop();
bool audio_is_running();
void audio_test();

#endif
