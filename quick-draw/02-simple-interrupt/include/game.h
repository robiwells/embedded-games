#ifndef GAME_H
#define GAME_H

#include <Arduino.h>

// ============================================================================
// Game State Machine
// ============================================================================

// Game states
typedef enum {
    STATE_ATTRACT,   // Waiting for player to start
    STATE_READY,     // Countdown before LED lights
    STATE_DRAW,      // LED is on, waiting for button press
    STATE_RESULT,    // Showing reaction time result
    NUM_STATES       // Total number of states
} GameState;

// State handler function pointers (Enter/Exit/Update pattern from Game 1)
typedef struct {
    void (*enter)(void);   // Called once when entering state
    void (*update)(void);  // Called every frame while in state
    void (*exit)(void);    // Called once when leaving state
} StateHandler;

// ============================================================================
// Game Interface
// ============================================================================

// Initialise game state machine (call once in setup())
void game_init(void);

// Update game state machine (call every loop iteration)
void game_update(void);

// Get current game state (used for debugging/status)
GameState game_get_state(void);

#endif // GAME_H
