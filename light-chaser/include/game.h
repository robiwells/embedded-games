#ifndef GAME_H
#define GAME_H

#include <Arduino.h>
#include "config.h"

// State handler function pointers
typedef struct {
    void (*enter)(void);   // Called once when entering state
    void (*update)(void);  // Called every frame while in state
    void (*exit)(void);    // Called once when leaving state
} StateHandler;

// Initialise game state
void game_init(void);

// Main game loop update (call from loop())
void game_update(void);

// Centralised state transition function
void game_transition_to(GameState new_state);

#endif // GAME_H
