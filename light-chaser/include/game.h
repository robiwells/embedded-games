#ifndef GAME_H
#define GAME_H

#include <Arduino.h>

// Initialise game state
void game_init(void);

// Main game loop update (call from loop())
void game_update(void);

#endif // GAME_H
