/******************************************************************************
 * GAME.CPP - State Machine Implementation
 *
 * TUTORIAL: Complete State Machine with Enter/Update/Exit Pattern
 *
 * This file implements the full game logic using a table-driven state machine.
 * It demonstrates:
 * - Enter/update/exit lifecycle pattern for each state
 * - Centralised state transitions through game_transition_to()
 * - Non-blocking timing using millis() timestamps
 * - Static memory allocation (no malloc/new)
 * - Clean separation between states
 *
 * ARCHITECTURE OVERVIEW:
 *
 * 5 game states × 3 lifecycle functions = 15 state handler functions
 * + 2 helper functions (update_chase_position, calculate_score)
 * + 3 public interface functions (game_init, game_update, game_transition_to)
 * = 20 functions total
 *
 * READING GUIDE:
 * 1. Read static variable section to understand game data
 * 2. Read state handler table to see the big picture
 * 3. Read game_transition_to() to understand how states change
 * 4. Read each state's enter/update/exit functions in sequence
 * 5. Read helper functions to see non-blocking timing patterns
 *
 * STATE FLOW SUMMARY:
 *
 * ATTRACT (demo)
 *    ↓ Button press
 * PLAYING (active game)
 *    ↓ Button press + hit target
 * RESULT (300ms pause)
 *    ↓ Timer expires
 * PLAYING (continue game)
 *    ↓ Button press + miss target
 * CELEBRATION or GAME_OVER (depending on high score)
 *    ↓ Animation complete
 * ATTRACT (loop)
 *
 * Related files:
 * - game.h: StateHandler typedef and public interface
 * - hardware.cpp: All hardware operations called from this file
 * - config.h: All constants (BULLSEYE_SCORE, chase speeds, etc.)
 ******************************************************************************/

#include "game.h"
#include "hardware.h"
#include "config.h"

/******************************************************************************
 * STATIC VARIABLES - Game State Data
 *
 * EMBEDDED MEMORY CONCEPT: Static Memory Allocation
 *
 * All these variables use the "static" keyword. On embedded systems, this is
 * CRITICAL for memory safety:
 *
 * STATIC ALLOCATION (what we do):
 *   static uint16_t score = 0;  // Allocated at compile time
 *   // Memory location determined by linker
 *   // Lives in .bss or .data section
 *   // Never freed, exists forever
 *
 * DYNAMIC ALLOCATION (what to AVOID on small embedded):
 *   uint16_t *score = (uint16_t*)malloc(sizeof(uint16_t));  // ❌ DON'T!
 *   // Runtime allocation from heap
 *   // Heap fragmentation over time
 *   // malloc() can fail (out of memory)
 *   // Must remember to free() (memory leaks)
 *
 * WHY AVOID MALLOC ON ARDUINO?
 *
 * 1. Only 2KB RAM total - heap fragmentation is deadly
 * 2. No memory protection - overflow corrupts other data
 * 3. No virtual memory - can't recover from out-of-memory
 * 4. malloc() adds overhead (~150 bytes of Flash code)
 * 5. Failure modes are catastrophic (crashes, not errors)
 *
 * STATIC BENEFITS:
 * - Predictable memory layout (known at compile time)
 * - Zero runtime overhead (no allocation/deallocation)
 * - Linker validates total memory usage (fails at build if too large)
 * - No memory leaks possible
 * - No fragmentation
 *
 * FILE SCOPE vs FUNCTION SCOPE:
 *
 * "static" at file scope (here):
 *   static int x;  // Private to this .cpp file only
 *   // Other files can't access x (good encapsulation)
 *
 * "static" inside function:
 *   void func() {
 *       static int x = 0;  // Retains value between calls
 *       x++;  // Increments every call
 *   }
 *
 * MEMORY LAYOUT (Arduino Uno with this code):
 *
 * FLASH (Program memory, 32 KB):
 *   - Program code: ~8 KB
 *   - String constants: ~200 bytes
 *   - Remaining: ~24 KB free
 *
 * RAM (Data memory, 2 KB):
 *   - Stack: ~200 bytes (grows downward)
 *   - Static/global variables: ~420 bytes
 *   - Remaining: ~1400 bytes free (no heap used!)
 *
 * See memory.md for full memory usage analysis.
 ******************************************************************************/

// Current state (using enum from config.h)
// Made static so external code MUST use game_transition_to() to change state
static GameState current_state = STATE_ATTRACT;

// Chase LED position and movement
static uint8_t current_position = 0;        // LED index (0-7)
static int8_t chase_direction = 1;          // Movement direction: +1 = right, -1 = left
static uint16_t chase_speed = INITIAL_CHASE_SPEED;  // ms between LED movements (decreases as game progresses)
static uint32_t last_chase_update = 0;      // Timestamp of last LED movement (for non-blocking timing)

// Score tracking
static uint16_t current_score = 0;          // Score for current game (reset on new game)
static uint16_t high_score = 0;             // All-time high score (loaded from EEPROM)
static bool is_new_high_score = false;      // Flag: did we beat the high score this game?

// Generic state timing
// Consolidated timing variable used by multiple states (RESULT, CELEBRATION)
// Replaces previous scattered timing variables (result_state_start, celebration_start_time)
static uint32_t state_entry_time = 0;       // Timestamp when we entered current state (millis())

/******************************************************************************
 * FORWARD DECLARATIONS
 *
 * C requires functions to be declared before use. We declare all state
 * handler functions here so they can be referenced in the state_handlers[]
 * table below (which comes before their implementations).
 *
 * "static" means these functions are private to this file - game.h only
 * exposes the public interface (game_init, game_update, game_transition_to).
 ******************************************************************************/

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

// Helper functions (private to this file)
static void update_chase_position(void);
static uint8_t calculate_score(uint8_t position);

/******************************************************************************
 * STATE HANDLER TABLE - Heart of the State Machine
 *
 * TABLE-DRIVEN PROGRAMMING PATTERN:
 *
 * This array maps each GameState enum to its three lifecycle functions.
 * Array index = GameState value (STATE_ATTRACT = 0, STATE_PLAYING = 1, etc.)
 *
 * SYNTAX BREAKDOWN:
 *
 * static const StateHandler state_handlers[5] = {
 *   └┬┘  └──┬──┘ └────┬─────┘ └─────┬──────┘ └┬┘
 *    │      │          │             │          └─ Array size (5 states)
 *    │      │          │             └──────────── Array name
 *    │      │          └────────────────────────── Type (struct from game.h)
 *    │      └───────────────────────────────────── Immutable (stored in Flash)
 *    └──────────────────────────────────────────── File-scope private
 *
 *     [STATE_ATTRACT] = {attract_enter, attract_update, attract_exit},
 *     └──────┬──────┘   └────────────────┬──────────────────────────┘
 *            │                            └─ StateHandler struct initialiser
 *            └────────────────────────────── Designated initialiser (C99 feature)
 *
 * DESIGNATED INITIALISERS (C99):
 *
 * Instead of positional initialisation:
 *   state_handlers[0] = {attract_enter, attract_update, attract_exit};
 *   state_handlers[1] = {playing_enter, playing_update, playing_exit};
 *   // Error-prone: if we change enum order, this breaks!
 *
 * We use designated initialisers (more robust):
 *   [STATE_ATTRACT] = {...};  // Explicitly maps enum to handler
 *   [STATE_PLAYING] = {...};
 *   // Safe: even if enum order changes, mapping remains correct
 *
 * HOW IT'S USED:
 *
 * To call current state's update function:
 *   state_handlers[current_state].update();
 *
 * Example: if current_state == STATE_PLAYING (value 1)
 *   → state_handlers[1].update()
 *   → playing_update()
 *
 * ADDING NEW STATES:
 *
 * To add a new state (e.g., STATE_PAUSED):
 * 1. Add to GameState enum in config.h:
 *      enum GameState {
 *          ... existing states ...
 *          STATE_PAUSED   // Add here
 *      };
 *
 * 2. Write 3 functions:
 *      static void paused_enter(void) { ... }
 *      static void paused_update(void) { ... }
 *      static void paused_exit(void) { ... }
 *
 * 3. Add entry to this table:
 *      [STATE_PAUSED] = {paused_enter, paused_update, paused_exit},
 *
 * 4. Update array size [5] → [6]
 *
 * That's it! No changes needed to game_update() or game_transition_to().
 * They automatically work with the new state.
 *
 * COMPARISON TO SWITCH STATEMENT APPROACH:
 *
 * Switch-based dispatcher (requires editing dispatcher for each new state):
 *   void game_update() {
 *       switch (current_state) {
 *           case STATE_ATTRACT:  attract_update();  break;
 *           case STATE_PLAYING:  playing_update();  break;
 *           // Must add case here for new states
 *       }
 *   }
 *
 * Table-based dispatcher (automatically supports any state in table):
 *   void game_update() {
 *       state_handlers[current_state].update();  // Done!
 *   }
 *
 * PERFORMANCE:
 * Table lookup: ~2-3 CPU cycles (array index + dereference)
 * Switch statement: ~5-10 CPU cycles (comparison + branch)
 * Table is actually FASTER, plus more scalable!
 ******************************************************************************/

static const StateHandler state_handlers[5] = {
    [STATE_ATTRACT]     = {attract_enter,     attract_update,     attract_exit},
    [STATE_PLAYING]     = {playing_enter,     playing_update,     playing_exit},
    [STATE_RESULT]      = {result_enter,      result_update,      result_exit},
    [STATE_CELEBRATION] = {celebration_enter, celebration_update, celebration_exit},
    [STATE_GAME_OVER]   = {game_over_enter,   game_over_update,   game_over_exit}
};

/******************************************************************************
 * game_transition_to - Centralised State Transition Manager
 *
 * THIS IS THE MOST IMPORTANT FUNCTION IN THE STATE MACHINE.
 *
 * ALL state changes flow through here. This guarantees that:
 * 1. Old state's exit() is always called
 * 2. State variable is changed
 * 3. New state's enter() is always called
 *
 * Without this function, you'd need to remember three steps every time:
 *   state_handlers[current_state].exit();  // Easy to forget!
 *   current_state = new_state;
 *   state_handlers[current_state].enter();  // Easy to forget!
 *
 * Now it's just one call:
 *   game_transition_to(STATE_PLAYING);  // Foolproof!
 *
 * TRANSITION SEQUENCE (example: ATTRACT → PLAYING):
 *
 * Before transition:
 *   current_state = STATE_ATTRACT
 *   Screen shows: "Press to Play!"
 *   LED is bouncing in demo mode
 *
 * User calls: game_transition_to(STATE_PLAYING)
 *
 * Step 1: Call attract_exit()
 *   - current_score = 0 (reset for new game)
 *   - button_clear_state() (forget old button presses)
 *
 * Step 2: current_state = STATE_PLAYING
 *   - State variable updated
 *
 * Step 3: Call playing_enter()
 *   - display_show_game(0, high_score) (show game screen)
 *   - last_chase_update = millis() (sync LED timing)
 *
 * After transition:
 *   current_state = STATE_PLAYING
 *   Screen shows: "Score: 0 / HiScore: 100"
 *   LED continues bouncing (seamless transition)
 *   Next loop: playing_update() runs
 *
 * NULL CHECKING:
 *
 * We check if function pointers are NULL before calling. Strictly unnecessary
 * (all our handlers have all three functions), but defensive programming:
 * - Prevents crash if someone forgets to implement a function
 * - Allows "empty" states (e.g., exit does nothing → set to NULL)
 * - Costs only ~4 CPU cycles per check (negligible)
 *
 * DEBUGGING ENHANCEMENT (optional):
 * Add logging to track all transitions:
 *   void game_transition_to(GameState new_state) {
 *       Serial.print("Transition: ");
 *       Serial.print(current_state);
 *       Serial.print(" -> ");
 *       Serial.println(new_state);
 *       // ... rest of function
 *   }
 *
 * ADVANCED: State Transition Validation (optional):
 * You could add validation to prevent illegal transitions:
 *   if (current_state == STATE_RESULT && new_state == STATE_ATTRACT) {
 *       // Illegal: can't go from RESULT directly to ATTRACT
 *       return;  // Ignore transition
 *   }
 ******************************************************************************/

void game_transition_to(GameState new_state) {
    // Call current state's exit function (cleanup old state)
    if (state_handlers[current_state].exit != NULL) {
        state_handlers[current_state].exit();
    }

    // Change state variable
    current_state = new_state;

    // Call new state's enter function (initialise new state)
    if (state_handlers[current_state].enter != NULL) {
        state_handlers[current_state].enter();
    }
}

/******************************************************************************
 * game_init - One-Time Game Initialisation
 *
 * Called from main.cpp:setup() before entering main loop.
 *
 * Initialises all game variables to starting values and loads persistent
 * data from EEPROM.
 *
 * INITIALISATION ORDER MATTERS:
 *
 * 1. Initialise chase LED variables
 *    - Position 0 (leftmost LED)
 *    - Direction +1 (moving right)
 *    - Speed at initial value (200ms)
 *
 * 2. Initialise scoring
 *    - Current score 0
 *    - Load high score from EEPROM (may be 0 on first boot)
 *
 * 3. Transition to initial state
 *    - Sets current_state = STATE_ATTRACT
 *    - Calls attract_enter() which shows attract screen
 *
 * WHY SET current_state TWICE?
 *
 *   current_state = STATE_ATTRACT;  // Line 79
 *   game_transition_to(STATE_ATTRACT);  // Line 80
 *
 * Technically redundant, but defensive:
 * - Line 79: Ensures current_state is valid before transition
 * - Line 80: Properly enters state (calls attract_enter)
 * - If we only did line 80 and current_state was garbage initially,
 *   line 80 would call exit() on invalid state (potential crash)
 *
 * MEMORY NOTE:
 * Static variables are automatically initialised to 0 at boot, so explicit
 * initialisation (= 0) is redundant but improves code clarity.
 ******************************************************************************/

void game_init(void) {
    // Initialise chase LED animation
    current_position = 0;
    chase_direction = 1;
    chase_speed = INITIAL_CHASE_SPEED;
    last_chase_update = millis();

    // Load high score from EEPROM
    // eeprom_read_high_score() validates data, returns 0 if corrupted/uninitialised
    high_score = eeprom_read_high_score();
    current_score = 0;
    is_new_high_score = false;

    // Enter initial state (attract mode)
    current_state = STATE_ATTRACT;  // Set valid state first
    game_transition_to(STATE_ATTRACT);  // Properly enter state (calls attract_enter)
}

/******************************************************************************
 * game_update - Main Game Loop (Per-Frame Update)
 *
 * Called every iteration of main.cpp:loop(), typically 1000-20000 times/sec.
 *
 * RESPONSIBILITIES:
 * 1. Update all animations (non-blocking)
 * 2. Call current state's update function
 *
 * CRITICAL REQUIREMENTS:
 * - Must execute quickly (< 4 seconds for watchdog timer, ideally < 1ms)
 * - Must NOT block (no delay() calls, no while loops)
 * - Must always call animation_update() before state update
 *
 * WHY animation_update() FIRST?
 *
 * Animations run on independent timers. If we called state update first:
 * 1. State update might trigger state transition
 * 2. Transition might start new animation
 * 3. New animation wouldn't update until NEXT frame (+1-50ms delay)
 * 4. User perceives lag between action and feedback
 *
 * Correct order:
 * 1. animation_update() advances any playing animations
 * 2. State update reads animation_is_playing() to detect completion
 * 3. Smooth, responsive behaviour
 *
 * EXECUTION TIME ANALYSIS:
 *
 * Typical frame time breakdown (STATE_PLAYING):
 *   animation_update():        ~50-100 μs
 *   playing_update():          ~200-500 μs
 *     ├─ update_chase_position(): ~100 μs
 *     ├─ button_just_pressed():   ~50 μs
 *     └─ calculate_score():       ~10 μs
 *   Total:                     ~300-600 μs
 *
 * At this rate, we could run 1600-3300 frames/second! In practice, limited
 * by millis() resolution (1ms), we run ~1000 frames/second.
 *
 * COMPARISON TO DESKTOP GAMES:
 * - Desktop games: 60 FPS (16.67ms per frame)
 * - Our game: ~1000 FPS (1ms per frame)
 * - We're 16× faster! (because we have far less to compute)
 ******************************************************************************/

void game_update(void) {
    // Always update animations first (non-blocking)
    // See hardware.cpp:animation_update() for state machine implementation
    animation_update();

    // Call current state's update function
    // This dynamically dispatches to the correct function based on current_state
    // Example: if current_state == STATE_PLAYING → calls playing_update()
    if (state_handlers[current_state].update != NULL) {
        state_handlers[current_state].update();
    }
}

/******************************************************************************
 * STATE_ATTRACT - Demo Mode / Attract Screen
 *
 * PURPOSE:
 * Waiting screen between games. Shows high score and plays chase animation
 * to attract players. Waits for button press to start game.
 *
 * VISUAL:
 *   LCD:  "Press to Play!"
 *         "HiScore: 100"
 *   LEDs: Bouncing chase LED at initial speed (200ms between movements)
 *
 * TRANSITIONS:
 *   → STATE_PLAYING (button pressed)
 ******************************************************************************/

/**
 * attract_enter - Initialise attract mode
 *
 * Called when entering STATE_ATTRACT from:
 * - game_init() (first boot)
 * - STATE_CELEBRATION (after celebrating high score)
 * - STATE_GAME_OVER (after game over animation)
 *
 * RESPONSIBILITIES:
 * - Reset chase speed to initial value (game difficulty reset)
 * - Display attract screen with current high score
 *
 * NOTE: We don't reset current_position or chase_direction. The LED continues
 * bouncing from wherever it was, creating seamless visual continuity.
 */
static void attract_enter(void) {
    chase_speed = INITIAL_CHASE_SPEED;  // Reset to easy difficulty
    display_show_attract(high_score);   // Show "Press to Play!" screen
}

/**
 * attract_update - Per-frame attract mode logic
 *
 * Called every loop iteration while in STATE_ATTRACT.
 *
 * RESPONSIBILITIES:
 * - Animate chase LED (bouncing back and forth)
 * - Wait for button press to start game
 *
 * LEARNING: Minimal State
 * This is one of the simplest update functions. Just animation + input check.
 * No scoring, no timers, no complex logic. Demonstrates that not all states
 * need to be complex.
 */
static void attract_update(void) {
    // Update chase LED position (non-blocking)
    // See update_chase_position() below for timing implementation
    update_chase_position();

    // Check for button press (edge detection, not level)
    // See hardware.cpp:button_just_pressed() for debouncing implementation
    if (button_just_pressed()) {
        game_transition_to(STATE_PLAYING);  // Start game!
    }
}

/**
 * attract_exit - Clean up attract mode
 *
 * Called when leaving STATE_ATTRACT (always transitioning to STATE_PLAYING).
 *
 * RESPONSIBILITIES:
 * - Reset current_score to 0 (starting fresh game)
 * - Clear is_new_high_score flag
 * - Clear button state to prevent stale edge detections
 *
 * WHY RESET SCORE HERE (not in playing_enter)?
 *
 * Score is reset in attract_exit because:
 * - Leaving attract always means starting new game
 * - playing_enter is ALSO called when returning from STATE_RESULT
 * - If we reset score in playing_enter, score would reset mid-game!
 *
 * Correct flow:
 *   ATTRACT → PLAYING (attract_exit resets score to 0)
 *   PLAYING → RESULT → PLAYING (score preserved, no reset)
 */
static void attract_exit(void) {
    current_score = 0;           // New game starts with score = 0
    is_new_high_score = false;   // Haven't beaten high score yet
    button_clear_state();        // Forget the button press that started the game
}

/******************************************************************************
 * STATE_PLAYING - Active Gameplay
 *
 * PURPOSE:
 * Main game state. Chase LED bounces, player presses button to score points.
 * Hit green LEDs (positions 3-4) = +10 points + continue playing.
 * Miss (red LEDs) = game ends.
 *
 * VISUAL:
 *   LCD:  "Score: 45"
 *         "HiScore: 100"
 *   LEDs: Bouncing chase LED (speed increases after each hit)
 *
 * TRANSITIONS:
 *   → STATE_RESULT (successful hit - any score > 0)
 *   → STATE_CELEBRATION (miss + new high score achieved)
 *   → STATE_GAME_OVER (miss + no new high score)
 ******************************************************************************/

/**
 * playing_enter - Initialise active gameplay
 *
 * Called when entering STATE_PLAYING from:
 * - STATE_ATTRACT (button press to start game)
 * - STATE_RESULT (300ms pause after hit, resume playing)
 *
 * RESPONSIBILITIES:
 * - Update display to show game screen (score + high score)
 * - Sync LED timing (reset last_chase_update to current time)
 *
 * IMPORTANT: What we DON'T do:
 * - DON'T reset score (preserved across RESULT transitions)
 * - DON'T reset LED position/direction (seamless animation)
 * - DON'T reset chase_speed (difficulty persists)
 *
 * WHY NOT RESET THESE?
 *
 * Score: Gets reset in attract_exit (new game) but NOT here.
 *   Flow: PLAYING → RESULT → PLAYING (score should persist!)
 *
 * LED position: Resetting would cause LED to "jump" back to position 0.
 *   Better: LED continues bouncing from current position (smooth)
 *
 * chase_speed: Speed increases with each hit throughout the game.
 *   Resetting here would make game easier after each hit (bad!)
 */
static void playing_enter(void) {
    // Update display to show current score and high score
    display_show_game(current_score, high_score);

    // Sync chase animation timing
    // Without this, LED might update immediately after entering state
    // (if time since last_chase_update > chase_speed)
    last_chase_update = millis();
}

/**
 * playing_update - Main gameplay loop
 *
 * Called every loop iteration while in STATE_PLAYING.
 *
 * RESPONSIBILITIES:
 * - Update chase LED position (bouncing animation)
 * - Check for button press
 * - Calculate score for button press position
 * - Handle hit (score points, increase difficulty, continue game)
 * - Handle miss (end game, check for high score)
 *
 * LEARNING: Complex State Logic
 * This is the most complex update function. It has multiple branches:
 * - Button not pressed: just update LED, return
 * - Button pressed + hit: score points, transition to RESULT
 * - Button pressed + miss + high score: transition to CELEBRATION
 * - Button pressed + miss + no high score: transition to GAME_OVER
 *
 * This demonstrates why separating states makes sense. Imagine all this
 * logic mixed with attract mode and game over handling in one giant function!
 */
static void playing_update(void) {
    // Update chase LED position every chase_speed milliseconds
    // Non-blocking: returns immediately if not enough time elapsed
    update_chase_position();

    // Check for button press (edge detection with debouncing)
    if (button_just_pressed()) {
        // Calculate score based on LED position when button pressed
        // Returns: 10 (bullseye), or 0 (miss)
        uint8_t points = calculate_score(current_position);

        if (points > 0) {
            /******************************************************************
             * SUCCESSFUL HIT - Continue Game
             ******************************************************************/

            // Add points to current score
            current_score += points;

            // Check if we've beaten the high score
            if (current_score > high_score) {
                is_new_high_score = true;  // Flag for later (used if we miss)
                high_score = current_score;  // Update high score immediately
            }

            // Update display with new score
            display_show_game(current_score, high_score);

            // Play appropriate sound effect
            if (points == BULLSEYE_SCORE) {
                // Bullseye (green LEDs): 3-note ascending melody
                animation_start_bullseye();
            } else {
                // Other hits (currently unused, but could be adjacent zones)
                buzzer_hit();
            }

            // Increase difficulty: speed up chase LED
            // Game gets 5ms faster after each hit, bottoming out at 50ms
            if (chase_speed > MIN_CHASE_SPEED) {
                chase_speed -= SPEED_DECREASE;
                if (chase_speed < MIN_CHASE_SPEED) {
                    chase_speed = MIN_CHASE_SPEED;  // Clamp to minimum
                }
            }

            // Transition to result state (brief pause, then resume)
            game_transition_to(STATE_RESULT);

        } else {
            /******************************************************************
             * MISS - Game Over
             ******************************************************************/

            // Check if we achieved a new high score during this game
            if (is_new_high_score) {
                // Celebrate new high score, then return to attract
                eeprom_write_high_score(high_score);  // Persist to EEPROM
                game_transition_to(STATE_CELEBRATION);
            } else {
                // Regular game over, no high score
                game_transition_to(STATE_GAME_OVER);
            }
        }
    }
}

/**
 * playing_exit - Clean up active gameplay
 *
 * Called when leaving STATE_PLAYING (transitioning to RESULT, CELEBRATION,
 * or GAME_OVER).
 *
 * RESPONSIBILITIES:
 * - None! (currently empty)
 *
 * WHY NO CLEANUP?
 *
 * All our cleanup happens in other states' exit functions:
 * - Score reset: attract_exit (before new game)
 * - Button clear: result_exit, celebration_exit, game_over_exit
 *
 * This function exists for symmetry (all states have enter/update/exit)
 * and future extensibility (might need cleanup later).
 *
 * POSSIBLE FUTURE USE:
 * - Stop sounds playing (if we add continuous background music)
 * - Save game state to EEPROM (for power-loss recovery)
 * - Log gameplay statistics
 */
static void playing_exit(void) {
    // No cleanup needed (all cleanup handled by destination state's exit)
}

/******************************************************************************
 * STATE_RESULT - Brief Pause After Successful Hit
 *
 * PURPOSE:
 * 300ms pause after scoring points. Gives player feedback that they hit the
 * target before LED continues bouncing. Without this pause, game feels too
 * fast and players can't process what happened.
 *
 * VISUAL:
 *   LCD:  "Score: 55" (updated)
 *         "HiScore: 100"
 *   LEDs: Chase LED frozen at hit position
 *   Audio: Bullseye melody or hit beep playing (non-blocking)
 *
 * TRANSITIONS:
 *   → STATE_PLAYING (after 300ms)
 ******************************************************************************/

/**
 * result_enter - Initialise result pause
 *
 * Called when entering STATE_RESULT from STATE_PLAYING (after successful hit).
 *
 * RESPONSIBILITIES:
 * - Record entry timestamp for timing the 300ms pause
 *
 * LEARNING: Simple Timed State
 * This demonstrates a common pattern: "wait N milliseconds, then transition".
 * We record the time we entered, then in update() we check if enough time
 * has elapsed.
 */
static void result_enter(void) {
    state_entry_time = millis();  // Record when we entered this state
}

/**
 * result_update - Wait for pause to complete
 *
 * Called every loop iteration while in STATE_RESULT.
 *
 * RESPONSIBILITIES:
 * - Check if 300ms has elapsed
 * - If yes, resume game (transition back to STATE_PLAYING)
 *
 * NON-BLOCKING TIMING PATTERN:
 *
 * Bad (blocking):
 *   delay(300);  // ❌ Freezes entire system for 300ms
 *   game_transition_to(STATE_PLAYING);
 *
 * Good (non-blocking):
 *   uint32_t now = millis();
 *   if (now - state_entry_time >= 300) {  // ✅ Check elapsed time
 *       game_transition_to(STATE_PLAYING);
 *   }
 *   // Returns immediately if time hasn't elapsed yet
 *
 * MILLIS() ROLLOVER SAFETY:
 *
 * millis() returns uint32_t (0 to 4,294,967,295). After ~49.7 days, it wraps
 * to 0. Our subtraction (now - state_entry_time) handles this correctly:
 *
 * Example (near rollover):
 *   state_entry_time = 4,294,967,200  (100ms before rollover)
 *   now = 50  (50ms after rollover, millis() wrapped)
 *   now - state_entry_time = 50 - 4,294,967,200
 *                          = -4,294,967,150  (as signed)
 *                          = 150  (as unsigned, due to wraparound)
 *   150 >= 300 → false (correct!)
 *
 * After 300ms total:
 *   now = 200  (200ms after rollover)
 *   now - state_entry_time = 200 - 4,294,967,200 = 350 (as unsigned)
 *   350 >= 300 → true (correct!)
 *
 * Unsigned arithmetic naturally handles wraparound. This is why we use
 * uint32_t for timestamps and calculate elapsed time as (now - then).
 */
static void result_update(void) {
    uint32_t now = millis();

    // Check if 300ms has elapsed since entering this state
    if (now - state_entry_time >= 300) {
        // Resume playing
        game_transition_to(STATE_PLAYING);

        // Sync chase timing to prevent immediate LED update after transition
        // Without this line, LED might move the instant we return to PLAYING
        // (if time since last_chase_update > chase_speed)
        last_chase_update = now;
    }
}

/**
 * result_exit - Clean up result pause
 *
 * Called when leaving STATE_RESULT (always transitioning to STATE_PLAYING).
 *
 * RESPONSIBILITIES:
 * - None! (currently empty)
 *
 * No cleanup needed. Button state is already cleared (from playing_update
 * consuming the button press), score is preserved, display is already correct.
 */
static void result_exit(void) {
    // No cleanup needed
}

/******************************************************************************
 * STATE_CELEBRATION - New High Score Animation
 *
 * PURPOSE:
 * Celebrate achieving a new high score! Plays exciting animation with music
 * and LED effects, then returns to attract mode.
 *
 * VISUAL:
 *   LCD:  "NEW HIGH SCORE!"
 *         "Score: 150"
 *   LEDs: Wave effect sweeping left-to-right 3 times
 *   Audio: 5-note ascending melody (C-E-G-C-E)
 *
 * TRANSITIONS:
 *   → STATE_ATTRACT (after 2 seconds)
 ******************************************************************************/

/**
 * celebration_enter - Initialise celebration
 *
 * Called when entering STATE_CELEBRATION from STATE_PLAYING (missed, but
 * achieved new high score during game).
 *
 * RESPONSIBILITIES:
 * - Display celebration message with final score
 * - Start celebration animation (parallel buzzer + LED effects)
 * - Record entry timestamp for 2-second minimum display time
 *
 * HIGH SCORE ALREADY SAVED:
 * Note that eeprom_write_high_score() was already called in playing_update
 * before transitioning here. We don't save again (avoid extra EEPROM wear).
 */
static void celebration_enter(void) {
    display_show_celebration(high_score);  // "NEW HIGH SCORE! Score: 150"
    animation_start_celebration();  // Start parallel LED wave + melody
    state_entry_time = millis();  // Record entry time for 2s minimum display
}

/**
 * celebration_update - Wait for celebration to complete
 *
 * Called every loop iteration while in STATE_CELEBRATION.
 *
 * RESPONSIBILITIES:
 * - Wait for minimum 2 seconds to elapse
 * - Then return to attract mode
 *
 * WHY 2 SECONDS?
 * - Animation takes ~1 second (5 notes × 150-300ms, 3 LED sweeps × 320ms)
 * - Extra 1 second lets player read "NEW HIGH SCORE!" message
 * - Total 2 seconds feels right (not too fast, not too slow)
 *
 * ALTERNATIVE IMPLEMENTATION:
 * Could wait for animation_is_playing() to become false instead of timer.
 * Current approach (timer) ensures message displays long enough even if
 * animation finishes early.
 */
static void celebration_update(void) {
    uint32_t now = millis();

    // After 2 seconds, return to attract mode
    if (now - state_entry_time >= 2000) {
        game_transition_to(STATE_ATTRACT);
    }
}

/**
 * celebration_exit - Clean up celebration
 *
 * Called when leaving STATE_CELEBRATION (always transitioning to STATE_ATTRACT).
 *
 * RESPONSIBILITIES:
 * - Clear button state to prevent stale edge detections
 *
 * WHY CLEAR BUTTON STATE?
 * If player pressed button during celebration (eager to play again), we don't
 * want that press to immediately start a new game when we enter attract mode.
 * Force them to press button again after seeing attract screen.
 */
static void celebration_exit(void) {
    button_clear_state();  // Forget any button presses during celebration
}

/******************************************************************************
 * STATE_GAME_OVER - Game Over Animation
 *
 * PURPOSE:
 * Game ended (missed target, no high score). Play "sad" animation, then
 * return to attract mode.
 *
 * VISUAL:
 *   LCD:  (Previous screen remains - "Score: X / HiScore: Y")
 *   LEDs: All flash on/off 5 times
 *   Audio: 3-note descending "sad trombone" (400→300→200 Hz)
 *
 * TRANSITIONS:
 *   → STATE_ATTRACT (after animation completes)
 ******************************************************************************/

/**
 * game_over_enter - Initialise game over animation
 *
 * Called when entering STATE_GAME_OVER from STATE_PLAYING (missed, no high score).
 *
 * RESPONSIBILITIES:
 * - Start game over animation (parallel buzzer + LED flash)
 * - Clear chase LED (stop bouncing before flash animation starts)
 *
 * NOTE: We don't update the display. The playing screen (showing final score
 * and high score) remains visible during animation. This lets player see
 * their final score.
 */
static void game_over_enter(void) {
    animation_start_game_over();  // Start parallel descending tones + LED flash
    led_clear_all();  // Turn off chase LED before flash animation starts
}

/**
 * game_over_update - Wait for animation to complete
 *
 * Called every loop iteration while in STATE_GAME_OVER.
 *
 * RESPONSIBILITIES:
 * - Check if animation finished
 * - If yes, return to attract mode
 *
 * ANIMATION COMPLETION DETECTION:
 * We use animation_is_playing() instead of a timer. This is more robust:
 * - If animation duration changes, no code update needed here
 * - Animation naturally controls its own timing
 * - State machine just waits for "done" signal
 *
 * Current animation duration:
 * - Buzzer: 3 notes × 200ms = 600ms
 * - LED: 5 flashes × (150ms on + 150ms off) = 1500ms
 * - Total: 1500ms (whichever finishes last)
 */
static void game_over_update(void) {
    // Wait for animation to complete
    if (!animation_is_playing()) {
        game_transition_to(STATE_ATTRACT);  // Return to attract mode
    }
}

/**
 * game_over_exit - Clean up game over
 *
 * Called when leaving STATE_GAME_OVER (always transitioning to STATE_ATTRACT).
 *
 * RESPONSIBILITIES:
 * - Reset score to 0 (ready for next game)
 * - Clear button state to prevent stale edge detections
 *
 * WHY RESET SCORE HERE?
 * attract_exit also resets score, but we reset here too for consistency.
 * Ensures score is 0 when entering attract, regardless of path taken.
 * (Redundant but harmless, improves robustness.)
 */
static void game_over_exit(void) {
    current_score = 0;      // Reset score for next game
    button_clear_state();   // Forget any button presses during game over
}

/******************************************************************************
 * HELPER FUNCTION: update_chase_position
 *
 * NON-BLOCKING LED ANIMATION PATTERN
 *
 * This function demonstrates the fundamental non-blocking timing pattern used
 * throughout embedded systems. Study this carefully - you'll use this pattern
 * constantly in embedded development!
 ******************************************************************************/

/**
 * update_chase_position - Update bouncing chase LED (non-blocking)
 *
 * Called every frame from attract_update() and playing_update().
 *
 * BEHAVIOUR:
 * - LED bounces left-to-right, reversing at edges
 * - Movement speed controlled by chase_speed (200ms initial, 50ms minimum)
 * - Plays tick sound on each movement
 *
 * NON-BLOCKING TIMING PATTERN:
 *
 * PROBLEM: How do we move LED every 200ms without using delay(200)?
 *
 * SOLUTION: Timestamp-based state machine
 *
 * PATTERN:
 *   static uint32_t last_update = 0;  // Timestamp of last action
 *   uint32_t now = millis();          // Current time
 *
 *   if (now - last_update >= interval) {
 *       // Enough time has elapsed, do the action
 *       do_action();
 *       last_update = now;  // Reset timer for next interval
 *   }
 *   // Return immediately (non-blocking!)
 *
 * BREAKDOWN:
 *
 * 1. last_chase_update: Static variable (retains value between calls)
 *    - Stores timestamp (millis()) of last LED movement
 *    - Initialised in game_init() and synced in state enter functions
 *
 * 2. now = millis(): Current time in milliseconds since boot
 *    - millis() increments every 1ms
 *    - Runs on hardware timer (independent of our code)
 *
 * 3. now - last_update: Elapsed time since last movement
 *    - Example: now = 1500, last = 1000 → elapsed = 500ms
 *
 * 4. >= chase_speed: Check if enough time passed
 *    - chase_speed = 200 initially, decreases to 50
 *    - If elapsed >= 200, time to move LED
 *
 * 5. Do the work: Update position, set LED, play sound
 *
 * 6. last_update = now: Reset timer
 *    - Next check will be relative to this timestamp
 *
 * WHY THIS PATTERN?
 *
 * Blocking (bad):
 *   while (1) {
 *       led_set(pos, true);
 *       delay(200);  // ❌ System frozen for 200ms
 *       // Can't check button, can't update display, watchdog starves!
 *   }
 *
 * Non-blocking (good):
 *   void update_chase_position() {
 *       if (time_elapsed) {
 *           move_led();  // ✅ Move LED if ready
 *       }
 *       return;  // Always return quickly
 *   }
 *   // Main loop continues: check button, update display, reset watchdog
 *
 * BOUNCING LOGIC:
 *
 * LED bounces at array boundaries (positions 0 and 7):
 *
 *   Position:  0   1   2   3   4   5   6   7
 *   Direction: → → → → → → → ← (hit right edge, reverse)
 *              → → → → → → → ←
 *   Direction: ← ← ← ← ← ← ← → (hit left edge, reverse)
 *
 * Implementation:
 *   - Start: pos=0, dir=+1
 *   - Move: pos += dir (0→1, 1→2, ..., 6→7)
 *   - Hit edge (pos==7): dir = -1
 *   - Move: pos += dir (7→6, 6→5, ..., 1→0)
 *   - Hit edge (pos==0): dir = +1
 *   - Repeat forever
 *
 * EDGE DETECTION:
 *   if (current_position == 0) → must be moving left, reverse to right
 *   if (current_position == NUM_LEDS - 1) → must be moving right, reverse left
 *
 * TICK SOUND:
 * We play tick sound on EVERY movement. This provides audio feedback for
 * LED speed (faster ticks = game getting harder). Helps players judge timing.
 */
static void update_chase_position(void) {
    // Get current time
    uint32_t now = millis();

    // Check if enough time has elapsed for next movement
    if (now - last_chase_update >= chase_speed) {
        // Time to move LED!

        // Reset timer for next movement
        // Using 'now' (not 'millis()') ensures consistent intervals
        // even if this code takes time to execute
        last_chase_update = now;

        // Turn off current LED
        led_clear_all();

        // Update position (move in current direction)
        current_position += chase_direction;

        // Check boundaries and reverse direction if needed
        if (current_position == 0) {
            // Hit left edge, start moving right
            chase_direction = 1;
        } else if (current_position == NUM_LEDS - 1) {
            // Hit right edge, start moving left
            chase_direction = -1;
        }

        // Turn on LED at new position
        led_set(current_position, true);

        // Play tick sound (audio feedback for movement)
        buzzer_tick();
    }
    // If not enough time elapsed, return immediately (non-blocking!)
}

/******************************************************************************
 * HELPER FUNCTION: calculate_score
 *
 * SCORING LOGIC
 ******************************************************************************/

/**
 * calculate_score - Determine points for hitting LED at given position
 * @param position: LED index (0-7) where button was pressed
 * @return: Points earned (10 for bullseye, 0 for miss)
 *
 * CURRENT SCORING SYSTEM:
 *
 * Position:  0   1   2   3   4   5   6   7
 * Colour:    R   R   R   G   G   R   R   R
 * Score:     0   0   0   10  10  0   0   0
 *
 * Green LEDs (positions 3-4): 10 points (bullseye zone)
 * Red LEDs (all others): 0 points (miss, game over)
 *
 * FUTURE EXPANSION:
 * Could implement graduated scoring:
 *   Position 3-4: 10 points (bullseye)
 *   Position 2, 5: 5 points (adjacent, see ADJACENT_SCORE in config.h)
 *   Position 0-1, 6-7: 1 point (outer, see OUTER_SCORE in config.h)
 *
 * DATA-DRIVEN ALTERNATIVE:
 * Instead of if/else logic, could use lookup table:
 *   static const uint8_t score_table[8] = {0, 0, 0, 10, 10, 0, 0, 0};
 *   return score_table[position];
 *
 * Benefits: Easier to adjust, clearer, potentially faster
 * Drawback: Uses 8 bytes of Flash (negligible on our system)
 */
static uint8_t calculate_score(uint8_t position) {
    // Check if position is in bullseye zone (green LEDs)
    if (position >= TARGET_ZONE_START && position <= TARGET_ZONE_END) {
        return BULLSEYE_SCORE;  // 10 points
    }

    // All other positions are misses
    return 0;  // Game over
}
