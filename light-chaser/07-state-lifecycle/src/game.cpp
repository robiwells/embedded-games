#include "game.h"
#include "config.h"

static GameState current_state = STATE_ATTRACT;
static uint8_t current_position = 0;
static int8_t chase_direction = 1;
static uint32_t last_chase_update = 0;
static uint16_t current_score = 0;
static uint16_t high_score = 0;
static uint32_t state_entry_time = 0;

static bool last_button_state = false;
static uint32_t last_debounce_time = 0;

static void attract_enter(void);
static void attract_update(void);
static void attract_exit(void);

static void playing_enter(void);
static void playing_update(void);
static void playing_exit(void);

static void result_enter(void);
static void result_update(void);
static void result_exit(void);

static void game_over_enter(void);
static void game_over_update(void);
static void game_over_exit(void);

static const StateHandler state_handlers[4] = {
    [STATE_ATTRACT]   = {attract_enter,   attract_update,   attract_exit},
    [STATE_PLAYING]   = {playing_enter,   playing_update,   playing_exit},
    [STATE_RESULT]    = {result_enter,    result_update,    result_exit},
    [STATE_GAME_OVER] = {game_over_enter, game_over_update, game_over_exit}
};

bool button_just_pressed(void) {
    uint32_t now = millis();
    bool current = !digitalRead(BUTTON_PIN);
    bool pressed = false;

    if (current && !last_button_state) {
        if (now - last_debounce_time >= DEBOUNCE_MS) {
            pressed = true;
            last_debounce_time = now;
        }
    }

    last_button_state = current;
    return pressed;
}

void update_chase_position(void) {
    uint32_t now = millis();

    if (now - last_chase_update >= CHASE_SPEED) {
        last_chase_update = now;

        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            digitalWrite(LED_PIN_START + i, LOW);
        }

        digitalWrite(LED_PIN_START + current_position, HIGH);
        current_position += chase_direction;

        if (current_position == 0) {
            chase_direction = 1;
        } else if (current_position == NUM_LEDS - 1) {
            chase_direction = -1;
        }
    }
}

void game_transition_to(GameState new_state) {
    if (state_handlers[current_state].exit != NULL) {
        state_handlers[current_state].exit();
    }

    current_state = new_state;

    if (state_handlers[current_state].enter != NULL) {
        state_handlers[current_state].enter();
    }
}

void game_init(void) {
    current_position = 0;
    chase_direction = 1;
    last_chase_update = millis();
    last_button_state = !digitalRead(BUTTON_PIN);
    high_score = 0;
    current_score = 0;

    current_state = STATE_ATTRACT;
    game_transition_to(STATE_ATTRACT);
}

void game_update(void) {
    if (state_handlers[current_state].update != NULL) {
        state_handlers[current_state].update();
    }
}

static void attract_enter(void) {
    Serial.println("Press button to start!");
}

static void attract_update(void) {
    update_chase_position();

    if (button_just_pressed()) {
        game_transition_to(STATE_PLAYING);
    }
}

static void attract_exit(void) {
    current_score = 0;
}

static void playing_enter(void) {
    Serial.println("Game started!");
}

static void playing_update(void) {
    update_chase_position();

    if (button_just_pressed()) {
        if (current_position >= TARGET_ZONE_START && current_position <= TARGET_ZONE_END) {
            current_score += BULLSEYE_SCORE;
            Serial.print("Hit! Score: ");
            Serial.println(current_score);
            game_transition_to(STATE_RESULT);
        } else {
            Serial.print("Miss! Final score: ");
            Serial.println(current_score);
            if (current_score > high_score) {
                high_score = current_score;
                Serial.print("New high score: ");
                Serial.println(high_score);
            }
            game_transition_to(STATE_GAME_OVER);
        }
    }
}

static void playing_exit(void) {
}

static void result_enter(void) {
    state_entry_time = millis();
}

static void result_update(void) {
    uint32_t now = millis();

    if (now - state_entry_time >= 300) {
        game_transition_to(STATE_PLAYING);
        last_chase_update = now;
    }
}

static void result_exit(void) {
}

static void game_over_enter(void) {
    state_entry_time = millis();
}

static void game_over_update(void) {
    uint32_t now = millis();

    if (now - state_entry_time >= 2000) {
        game_transition_to(STATE_ATTRACT);
    }
}

static void game_over_exit(void) {
    current_score = 0;
}
