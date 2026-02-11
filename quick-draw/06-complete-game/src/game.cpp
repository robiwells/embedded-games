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

// Round tracking
static uint8_t current_round = 0;
static uint16_t round_times[TOTAL_ROUNDS];  // Store each round's reaction time

// Statistics
static uint16_t best_time_overall = 0;     // Best time ever (from EEPROM)
static uint16_t best_time_session = 0;     // Best time this session
static uint16_t average_time = 0;          // Average time for current game

// State-specific variables
static uint32_t ready_duration = 0;          // Random countdown duration
static uint32_t draw_start_time = 0;         // When LED turned on (in microseconds)
static uint32_t next_distractor_time = 0;    // When to show next distractor
static uint8_t distractor_led_index = 0;     // Which distractor LED is on
static bool distractor_active = false;       // Is a distractor currently shown

// ============================================================================
// Forward Declarations
// ============================================================================

static void game_transition_to(GameState new_state);
static uint16_t calculate_average_time(void);
static const char* get_feedback_message(uint16_t reaction_ms);

// ============================================================================
// State Handler Functions
// ============================================================================

// --- ATTRACT State ---

static void attract_enter(void) {
    led_clear_all();

    // Load best time from EEPROM
    best_time_overall = eeprom_load_best_time();

    display_show_attract(best_time_overall);
    button_clear_state();
}

static void attract_update(void) {
    // Wait for button press to start game
    if (button_was_pressed()) {
        buzzer_beep();

        // Reset game state
        current_round = 0;
        best_time_session = 0;

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

    // Increment round counter
    current_round++;

    // Set random countdown duration (2-4 seconds)
    ready_duration = random(READY_MIN_MS, READY_MAX_MS);
    state_entry_time = millis();

    // Schedule first distractor
    next_distractor_time = state_entry_time + random(DISTRACTOR_MIN_MS, DISTRACTOR_MAX_MS);
    distractor_active = false;
}

static void ready_update(void) {
    uint32_t current_time = millis();

    // Check for false start (button pressed before LED)
    if (button_was_pressed()) {
        game_transition_to(STATE_FALSE_START);
        return;
    }

    // Handle distractor LEDs (flash wrong LEDs during countdown)
    if (!distractor_active && current_time >= next_distractor_time) {
        // Show a distractor LED
        distractor_led_index = random(0, 4);  // LEDs 0-3 are distractors
        led_set_distractor(distractor_led_index);
        distractor_active = true;
        next_distractor_time = current_time + DISTRACTOR_DURATION;
    } else if (distractor_active && current_time >= next_distractor_time) {
        // Clear distractor LED
        led_clear_all();
        distractor_active = false;

        // Schedule next distractor (if there's time)
        uint32_t time_remaining = ready_duration - (current_time - state_entry_time);
        if (time_remaining > DISTRACTOR_MAX_MS) {
            next_distractor_time = current_time + random(DISTRACTOR_MIN_MS, DISTRACTOR_MAX_MS);
        }
    }

    // Check if ready period expired
    if (current_time - state_entry_time >= ready_duration) {
        game_transition_to(STATE_DRAW);
    }
}

static void ready_exit(void) {
    led_clear_all();
}

// --- DRAW State ---

static void draw_enter(void) {
    // Turn on middle LED (index 4 of 0-7)
    led_set(4);
    button_clear_state();

    // Record when LED turned on (microseconds for precision)
    draw_start_time = micros();
}

static void draw_update(void) {
    // Check for button press
    if (button_was_pressed()) {
        // Validate that press happened during DRAW state
        // (ISR captures moment-in-time state for validation)
        if (button_get_state_at_press() == STATE_DRAW) {
            uint32_t press_time = button_get_press_time();
            uint32_t reaction_us = press_time - draw_start_time;

            // Check if reaction time is valid (anti-cheat for impossibly fast times)
            if (reaction_us >= REACTION_MIN_US && reaction_us <= REACTION_MAX_US) {
                // Valid reaction - store time and show result
                uint16_t reaction_ms = reaction_us / 1000;
                round_times[current_round - 1] = reaction_ms;
                game_transition_to(STATE_RESULT);
            } else {
                // Invalid reaction time - treat as timeout
                round_times[current_round - 1] = REACTION_MAX_MS;
                game_transition_to(STATE_RESULT);
            }
        } else {
            // Pressed during distractor LED - false start
            game_transition_to(STATE_FALSE_START);
        }
        return;
    }

    // Check for timeout
    uint32_t elapsed = micros() - draw_start_time;
    if (elapsed >= REACTION_MAX_US) {
        round_times[current_round - 1] = REACTION_MAX_MS;
        game_transition_to(STATE_RESULT);
    }
}

static void draw_exit(void) {
    led_clear_all();
}

// --- RESULT State ---

static void result_enter(void) {
    uint16_t reaction_ms = round_times[current_round - 1];

    // Update statistics
    average_time = calculate_average_time();

    // Update session best
    if (best_time_session == 0 || reaction_ms < best_time_session) {
        best_time_session = reaction_ms;
    }

    // Check for timeout
    if (reaction_ms >= REACTION_MAX_MS) {
        display_show_timeout();
        buzzer_error();
    } else {
        // Get feedback message
        const char* feedback = get_feedback_message(reaction_ms);

        // Show result
        display_show_result(reaction_ms * 1000UL, feedback, current_round, average_time);

        // Play appropriate sound
        if (reaction_ms < REACTION_QUICK_MS) {
            buzzer_success();
        } else {
            buzzer_beep();
        }
    }

    state_entry_time = millis();
    button_clear_state();
}

static void result_update(void) {
    // Wait for display duration
    if (millis() - state_entry_time >= RESULT_DISPLAY_MS) {
        // Check if game is complete
        if (current_round >= TOTAL_ROUNDS) {
            // Check for new overall record
            if (best_time_overall == 0 || best_time_session < best_time_overall) {
                game_transition_to(STATE_NEW_RECORD);
            } else {
                game_transition_to(STATE_GAME_COMPLETE);
            }
        } else {
            // More rounds to play
            game_transition_to(STATE_ROUND_COMPLETE);
        }
    }
}

static void result_exit(void) {
    // No cleanup needed
}

// --- ROUND_COMPLETE State ---

static void round_complete_enter(void) {
    display_show_round_complete(current_round, average_time);
    state_entry_time = millis();
    button_clear_state();
}

static void round_complete_update(void) {
    // Wait for pause duration
    if (millis() - state_entry_time >= ROUND_PAUSE_MS) {
        game_transition_to(STATE_READY);
    }
}

static void round_complete_exit(void) {
    // No cleanup needed
}

// --- FALSE_START State ---

static void false_start_enter(void) {
    led_clear_all();
    display_show_false_start();
    buzzer_error();
    state_entry_time = millis();
    button_clear_state();
}

static void false_start_update(void) {
    // Wait for display duration
    if (millis() - state_entry_time >= FALSE_START_DISPLAY_MS) {
        // Retry current round (don't increment round counter)
        current_round--;  // Will be incremented again in ready_enter
        game_transition_to(STATE_READY);
    }
}

static void false_start_exit(void) {
    // No cleanup needed
}

// --- NEW_RECORD State ---

static void new_record_enter(void) {
    display_show_new_record(best_time_session);
    buzzer_celebration();

    // Save new record to EEPROM
    eeprom_save_best_time(best_time_session);
    best_time_overall = best_time_session;

    state_entry_time = millis();
    button_clear_state();
}

static void new_record_update(void) {
    // Wait for celebration duration
    if (millis() - state_entry_time >= NEW_RECORD_DISPLAY_MS) {
        game_transition_to(STATE_GAME_COMPLETE);
    }
}

static void new_record_exit(void) {
    // No cleanup needed
}

// --- GAME_COMPLETE State ---

static void game_complete_enter(void) {
    display_show_game_complete(best_time_session, average_time);
    state_entry_time = millis();
    button_clear_state();
}

static void game_complete_update(void) {
    // Wait for button press to return to attract
    if (button_was_pressed()) {
        buzzer_beep();
        game_transition_to(STATE_ATTRACT);
    }
}

static void game_complete_exit(void) {
    // No cleanup needed
}

// ============================================================================
// State Handler Table
// ============================================================================

static const StateHandler state_handlers[NUM_STATES] = {
    [STATE_ATTRACT]         = { attract_enter,         attract_update,         attract_exit         },
    [STATE_READY]           = { ready_enter,           ready_update,           ready_exit           },
    [STATE_DRAW]            = { draw_enter,            draw_update,            draw_exit            },
    [STATE_RESULT]          = { result_enter,          result_update,          result_exit          },
    [STATE_ROUND_COMPLETE]  = { round_complete_enter,  round_complete_update,  round_complete_exit  },
    [STATE_FALSE_START]     = { false_start_enter,     false_start_update,     false_start_exit     },
    [STATE_NEW_RECORD]      = { new_record_enter,      new_record_update,      new_record_exit      },
    [STATE_GAME_COMPLETE]   = { game_complete_enter,   game_complete_update,   game_complete_exit   },
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
// Helper Functions
// ============================================================================

static uint16_t calculate_average_time(void) {
    if (current_round == 0) {
        return 0;
    }

    uint32_t sum = 0;
    for (uint8_t i = 0; i < current_round; i++) {
        sum += round_times[i];
    }

    return (uint16_t)(sum / current_round);
}

static const char* get_feedback_message(uint16_t reaction_ms) {
    if (reaction_ms < REACTION_LIGHTNING_MS) {
        return "Lightning!";
    } else if (reaction_ms < REACTION_QUICK_MS) {
        return "Quick!";
    } else if (reaction_ms < REACTION_OK_MS) {
        return "OK";
    } else {
        return "Slow";
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
