#ifndef GAME_H
#define GAME_H

#include <Arduino.h>
#include "config.h"

typedef struct {
    void (*enter)(void);
    void (*update)(void);
    void (*exit)(void);
} StateHandler;

void game_init(void);
void game_update(void);
void game_transition_to(GameState new_state);

#endif
