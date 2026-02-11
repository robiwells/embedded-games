#ifndef GAME_H
#define GAME_H

#include <Arduino.h>

// ============================================================================
// Game State Machine
// ============================================================================

// Game states
typedef enum {
    STATE_ATTRACT,          // Waiting for player to start
    STATE_READY,            // Countdown before LED lights (with distractors)
    STATE_DRAW,             // LED is on, waiting for button press
    STATE_RESULT,           // Showing reaction time result
    STATE_ROUND_COMPLETE,   // Brief pause between rounds
    STATE_FALSE_START,      // Player pressed too early
    STATE_NEW_RECORD,       // Celebration for beating personal best
    STATE_GAME_COMPLETE,    // Game over, showing final stats
    NUM_STATES              // Total number of states
} GameState;

// State handler function pointers (Enter/Exit/Update pattern)
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

// Get current game state (used by hardware layer for state validation)
GameState game_get_state(void);

#endif // GAME_H
