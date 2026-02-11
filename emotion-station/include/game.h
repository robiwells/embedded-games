#ifndef GAME_H
#define GAME_H

#ifndef UNIT_TEST
#include <Arduino.h>
#endif

#include "platform_hal.h"

// Game states enum
typedef enum {
    STATE_IDLE = 0,              // Waiting for NFC tag
    STATE_NFC_DETECTED,          // NFC tag detected, reading UID
    STATE_VALIDATING,            // Validating NFC UID
    STATE_SELECTING,             // Choosing appropriate activity
    STATE_PLAYING_ACTIVITY,      // Activity in progress
    STATE_ACTIVITY_COMPLETE,     // Success celebration
    STATE_ERROR,                 // Error handling state
    STATE_LOW_BATTERY,           // Critical battery mode
    NUM_STATES                   // Array sizing constant
} GameState;

// State handler structure (enter/exit/update pattern)
typedef struct {
    void (*enter)(void);   // Called once when entering state
    void (*update)(void);  // Called every frame while in state
    void (*exit)(void);    // Called once when leaving state
} StateHandler;

// Public API
void game_init();                           // Initialise state machine
void game_update();                         // Update current state (call every loop)
void game_transition_to(GameState state);   // Centralised state transitions
GameState game_get_current_state();         // Query current state
void game_test_transitions();               // Test function for verification

#endif // GAME_H
