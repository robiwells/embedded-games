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
static uint32_t result_state_start = 0;
static uint32_t celebration_start_time = 0;

// High score tracking
static uint16_t high_score = 0;
static bool is_new_high_score = false;

// Forward declarations for state handlers
static void handle_attract_state(void);
static void handle_playing_state(void);
static void handle_result_state(void);
static void handle_celebration_state(void);
static void handle_game_over_state(void);
static void update_chase_position(void);
static uint8_t calculate_score(uint8_t position);

void game_init(void) {
    current_state = STATE_ATTRACT;
    current_position = 0;
    chase_direction = 1;
    chase_speed = INITIAL_CHASE_SPEED;
    current_score = 0;
    last_chase_update = millis();

    // Load and display high score
    high_score = eeprom_read_high_score();
    is_new_high_score = false;
    display_show_attract(high_score);
}

void game_update(void) {
    // Always update animations (non-blocking)
    animation_update();

    switch (current_state) {
        case STATE_ATTRACT:
            handle_attract_state();
            break;
        case STATE_PLAYING:
            handle_playing_state();
            break;
        case STATE_RESULT:
            handle_result_state();
            break;
        case STATE_CELEBRATION:
            handle_celebration_state();
            break;
        case STATE_GAME_OVER:
            handle_game_over_state();
            break;
    }
}

static void handle_attract_state(void) {
    // Chase animation
    update_chase_position();

    // Wait for button press to start game
    if (button_just_pressed()) {
        current_state = STATE_PLAYING;
        current_score = 0;
        is_new_high_score = false;
        display_show_game(0, high_score);  // Show both scores
        chase_speed = INITIAL_CHASE_SPEED;
        // Don't reset position/direction - let LED continue seamlessly
        last_chase_update = millis();

        // Clear button state to prevent immediate re-triggering
        button_clear_state();
    }
}

static void handle_playing_state(void) {
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

            // Brief result state
            current_state = STATE_RESULT;
            result_state_start = millis();
        } else {
            // Miss - game ends
            if (is_new_high_score) {
                // Celebrate and save new high score
                eeprom_write_high_score(high_score);
                display_show_celebration(high_score);
                animation_start_celebration();
                current_state = STATE_CELEBRATION;
                celebration_start_time = millis();
                return;  // Handle in celebration state
            } else {
                // Game over without high score - play animation
                animation_start_game_over();
                current_state = STATE_GAME_OVER;
                led_clear_all();  // Stop chase LED before animation starts

                // Clear button state to prevent miss button press from triggering anything
                button_clear_state();
            }
        }
    }
}

static void handle_result_state(void) {
    // Brief pause to show the hit, then continue
    uint32_t now = millis();
    if (now - result_state_start >= 300) {
        current_state = STATE_PLAYING;
        last_chase_update = now;
    }
}

static void handle_celebration_state(void) {
    uint32_t now = millis();

    // After 2 seconds, return to attract mode
    if (now - celebration_start_time >= 2000) {
        current_state = STATE_ATTRACT;
        chase_speed = INITIAL_CHASE_SPEED;
        display_show_attract(high_score);
        button_clear_state();
    }
}

static void handle_game_over_state(void) {
    // Wait for animation to complete, then return to attract mode
    if (!animation_is_playing()) {
        current_state = STATE_ATTRACT;
        current_score = 0;
        chase_speed = INITIAL_CHASE_SPEED;
        display_show_attract(high_score);
        button_clear_state();
    }
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
