#include "game.h"
#include "hardware.h"
#include "config.h"

// Game state variables
static GameState current_state = STATE_ATTRACT;
static uint8_t current_position = 0;
static int8_t chase_direction = 1;
static uint16_t chase_speed = INITIAL_CHASE_SPEED;
static uint32_t last_chase_update = 0;
static uint16_t current_score = 0;
static uint32_t state_entry_time = 0;  // Generic timing for state entry

// High score tracking
static uint16_t high_score = 0;
static bool is_new_high_score = false;

// Forward declarations for enter/update/exit functions
static void attract_enter(void);
static void attract_update(void);
static void attract_exit(void);

static void playing_enter(void);
static void playing_update(void);
static void playing_exit(void);

static void result_enter(void);
static void result_update(void);
static void result_exit(void);

static void celebration_enter(void);
static void celebration_update(void);
static void celebration_exit(void);

static void game_over_enter(void);
static void game_over_update(void);
static void game_over_exit(void);

// Helper functions
static void update_chase_position(void);
static uint8_t calculate_score(uint8_t position);

// State handler table
static const StateHandler state_handlers[5] = {
    [STATE_ATTRACT]     = {attract_enter,     attract_update,     attract_exit},
    [STATE_PLAYING]     = {playing_enter,     playing_update,     playing_exit},
    [STATE_RESULT]      = {result_enter,      result_update,      result_exit},
    [STATE_CELEBRATION] = {celebration_enter, celebration_update, celebration_exit},
    [STATE_GAME_OVER]   = {game_over_enter,   game_over_update,   game_over_exit}
};

void game_transition_to(GameState new_state) {
    // Call current state's exit function
    if (state_handlers[current_state].exit != NULL) {
        state_handlers[current_state].exit();
    }

    // Change state
    current_state = new_state;

    // Call new state's enter function
    if (state_handlers[current_state].enter != NULL) {
        state_handlers[current_state].enter();
    }
}

void game_init(void) {
    current_position = 0;
    chase_direction = 1;
    chase_speed = INITIAL_CHASE_SPEED;
    current_score = 0;
    last_chase_update = millis();

    // Load high score
    high_score = eeprom_read_high_score();
    is_new_high_score = false;

    // Initialise to attract state (calls attract_enter)
    current_state = STATE_ATTRACT;
    game_transition_to(STATE_ATTRACT);
}

void game_update(void) {
    // Always update animations (non-blocking)
    animation_update();

    // Call current state's update function
    if (state_handlers[current_state].update != NULL) {
        state_handlers[current_state].update();
    }
}

// ========== ATTRACT STATE ==========
static void attract_enter(void) {
    chase_speed = INITIAL_CHASE_SPEED;
    display_show_attract(high_score);
}

static void attract_update(void) {
    // Chase animation
    update_chase_position();

    // Wait for button press to start game
    if (button_just_pressed()) {
        game_transition_to(STATE_PLAYING);
    }
}

static void attract_exit(void) {
    // Starting new game - reset score
    current_score = 0;
    is_new_high_score = false;
    button_clear_state();
}

// ========== PLAYING STATE ==========
static void playing_enter(void) {
    // Note: Score is NOT reset here - only when starting new game (attract_exit)
    // This allows score to persist when returning from RESULT state
    display_show_game(current_score, high_score);
    // Don't reset position/direction - let LED continue seamlessly
    last_chase_update = millis();
}

static void playing_update(void) {
    // Update chase position
    update_chase_position();

    // Check for button press
    if (button_just_pressed()) {
        uint8_t points = calculate_score(current_position);

        if (points > 0) {
            // Successful hit
            current_score += points;

            // Check for new high score
            if (current_score > high_score) {
                is_new_high_score = true;
                high_score = current_score;
            }

            display_show_game(current_score, high_score);

            // Play appropriate sound
            if (points == BULLSEYE_SCORE) {
                animation_start_bullseye();
            } else {
                buzzer_hit();
            }

            // Increase difficulty on every successful hit
            if (chase_speed > MIN_CHASE_SPEED) {
                chase_speed -= SPEED_DECREASE;
                if (chase_speed < MIN_CHASE_SPEED) {
                    chase_speed = MIN_CHASE_SPEED;
                }
            }

            // Transition to result state
            game_transition_to(STATE_RESULT);
        } else {
            // Miss - game ends
            if (is_new_high_score) {
                // Save and celebrate new high score
                eeprom_write_high_score(high_score);
                game_transition_to(STATE_CELEBRATION);
            } else {
                // Game over without high score
                game_transition_to(STATE_GAME_OVER);
            }
        }
    }
}

static void playing_exit(void) {
    // No cleanup needed
}

// ========== RESULT STATE ==========
static void result_enter(void) {
    state_entry_time = millis();
}

static void result_update(void) {
    // Brief pause to show the hit, then continue
    uint32_t now = millis();
    if (now - state_entry_time >= 300) {
        game_transition_to(STATE_PLAYING);
        last_chase_update = now;
    }
}

static void result_exit(void) {
    // No cleanup needed
}

// ========== CELEBRATION STATE ==========
static void celebration_enter(void) {
    display_show_celebration(high_score);
    animation_start_celebration();
    state_entry_time = millis();
}

static void celebration_update(void) {
    uint32_t now = millis();

    // After 2 seconds, return to attract mode
    if (now - state_entry_time >= 2000) {
        game_transition_to(STATE_ATTRACT);
    }
}

static void celebration_exit(void) {
    button_clear_state();
}

// ========== GAME OVER STATE ==========
static void game_over_enter(void) {
    animation_start_game_over();
    led_clear_all();  // Stop chase LED before animation starts
}

static void game_over_update(void) {
    // Wait for animation to complete, then return to attract mode
    if (!animation_is_playing()) {
        game_transition_to(STATE_ATTRACT);
    }
}

static void game_over_exit(void) {
    current_score = 0;
    button_clear_state();
}

static void update_chase_position(void) {
    uint32_t now = millis();

    if (now - last_chase_update >= chase_speed) {
        last_chase_update = now;

        // Clear current LED
        led_clear_all();

        // Update position
        current_position += chase_direction;

        // Bounce at edges
        if (current_position == 0) {
            chase_direction = 1;
        } else if (current_position == NUM_LEDS - 1) {
            chase_direction = -1;
        }

        // Set new LED
        led_set(current_position, true);

        // Play tick sound
        buzzer_tick();
    }
}

static uint8_t calculate_score(uint8_t position) {
    // Only green LEDs (bullseye zone, positions 3-4) score points
    if (position >= TARGET_ZONE_START && position <= TARGET_ZONE_END) {
        return BULLSEYE_SCORE;  // 10 points
    }

    // All red LEDs (positions 0-2, 5-7) are misses
    return 0;
}
