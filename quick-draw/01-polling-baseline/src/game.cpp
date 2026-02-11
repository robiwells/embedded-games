#include "game.h"
#include "hardware.h"
#include "config.h"

// ============================================================================
// State Machine Variables
// ============================================================================

static GameState current_state = STATE_ATTRACT;
static uint32_t state_entry_time = 0;  // Timestamp when current state was entered

// ============================================================================
// Game Variables
// ============================================================================

static uint32_t ready_duration = 0;       // Random countdown duration
static uint32_t draw_start_time = 0;      // When LED turned on
static uint32_t button_press_time = 0;    // When button was pressed

// ============================================================================
// Forward Declarations
// ============================================================================

static void game_transition_to(GameState new_state);

// ============================================================================
// State Handler Functions
// ============================================================================

// --- ATTRACT State ---

static void attract_enter(void) {
    led_clear_all();
    display_show_attract();
    button_clear_state();
}

static void attract_update(void) {
    // Wait for button press to start game
    if (button_just_pressed()) {
        buzzer_beep();
        game_transition_to(STATE_READY);
    }
}

static void attract_exit(void) {
    // No cleanup needed
}

// --- READY State ---

static void ready_enter(void) {
    led_clear_all();
    display_show_ready();
    button_clear_state();

    // Set random countdown duration (2-4 seconds)
    ready_duration = random(READY_MIN_MS, READY_MAX_MS);
    state_entry_time = millis();
}

static void ready_update(void) {
    // Wait for countdown to expire
    if (millis() - state_entry_time >= ready_duration) {
        game_transition_to(STATE_DRAW);
    }
}

static void ready_exit(void) {
    // No cleanup needed
}

// --- DRAW State ---

static void draw_enter(void) {
    // Turn on middle LED (index 3 of 0-6)
    led_set(3);
    button_clear_state();

    // Record when LED turned on
    draw_start_time = millis();
}

static void draw_update(void) {
    // Check for button press (polling-based)
    if (button_just_pressed()) {
        button_press_time = millis();
        game_transition_to(STATE_RESULT);
        return;
    }

    // Check for timeout
    uint32_t elapsed = millis() - draw_start_time;
    if (elapsed >= REACTION_MAX_MS) {
        game_transition_to(STATE_RESULT);
    }
}

static void draw_exit(void) {
    led_clear_all();
}

// --- RESULT State ---

static void result_enter(void) {
    // Calculate reaction time
    uint32_t reaction_time = button_press_time - draw_start_time;

    // Check for timeout
    if (button_press_time == 0 || reaction_time >= REACTION_MAX_MS) {
        display_show_timeout();
        buzzer_error();
    } else {
        // Determine feedback based on reaction time
        const char* feedback;
        if (reaction_time < REACTION_LIGHTNING_MS) {
            feedback = "Lightning!";
            buzzer_success();
        } else if (reaction_time < REACTION_QUICK_MS) {
            feedback = "Quick!";
            buzzer_success();
        } else if (reaction_time < REACTION_OK_MS) {
            feedback = "OK";
            buzzer_beep();
        } else {
            feedback = "Slow";
            buzzer_beep();
        }

        display_show_result(reaction_time, feedback);
    }

    // Reset for next round
    button_press_time = 0;
    state_entry_time = millis();
    button_clear_state();
}

static void result_update(void) {
    // Wait for display duration
    if (millis() - state_entry_time >= RESULT_DISPLAY_MS) {
        game_transition_to(STATE_ATTRACT);
    }
}

static void result_exit(void) {
    // No cleanup needed
}

// ============================================================================
// State Handler Table
// ============================================================================

static const StateHandler state_handlers[NUM_STATES] = {
    [STATE_ATTRACT] = { attract_enter, attract_update, attract_exit },
    [STATE_READY]   = { ready_enter,   ready_update,   ready_exit   },
    [STATE_DRAW]    = { draw_enter,    draw_update,    draw_exit    },
    [STATE_RESULT]  = { result_enter,  result_update,  result_exit  },
};

// ============================================================================
// State Transition Function
// ============================================================================

static void game_transition_to(GameState new_state) {
    // Call exit handler for current state
    if (state_handlers[current_state].exit != NULL) {
        state_handlers[current_state].exit();
    }

    // Change state
    current_state = new_state;

    // Call enter handler for new state
    if (state_handlers[current_state].enter != NULL) {
        state_handlers[current_state].enter();
    }
}

// ============================================================================
// Public Interface
// ============================================================================

void game_init(void) {
    // Seed random number generator
    randomSeed(analogRead(A0));

    // Enter initial state
    game_transition_to(STATE_ATTRACT);
}

void game_update(void) {
    // Call update handler for current state
    if (state_handlers[current_state].update != NULL) {
        state_handlers[current_state].update();
    }
}

GameState game_get_state(void) {
    return current_state;
}
