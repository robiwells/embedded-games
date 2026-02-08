/******************************************************************************
 * GAME.H - Game State Machine Interface
 *
 * This file introduces the "enter/update/exit" lifecycle pattern for state
 * machines.
 *
 * WHAT IS A STATE MACHINE?
 *
 * A state machine is a system that's always in exactly one state. Each state
 * has different behaviour. The system transitions between states based on
 * events or conditions.
 *
 * EXAMPLE: Traffic Light
 * States: GREEN, YELLOW, RED
 * Transitions:
 *   GREEN → (timer expires) → YELLOW
 *   YELLOW → (timer expires) → RED
 *   RED → (timer expires) → GREEN
 *
 * WHY STATE MACHINES FOR EMBEDDED SYSTEMS?
 *
 * 1. **Clarity**: Current behaviour obvious from current state
 * 2. **Maintainability**: Each state's code is isolated
 * 3. **Debuggability**: Can log state transitions
 * 4. **Safety**: Prevents invalid operations (e.g., can't score in ATTRACT)
 * 5. **Testability**: Can test each state independently
 *
 * OUR APPROACH (enter/update/exit pattern):
 *   - Each state has 3 functions: enter, update, exit
 *   - Table-driven state machine (see game.cpp:state_handlers[])
 *   - Centralised transitions handle enter/exit automatically
 *
 * ENTER/UPDATE/EXIT LIFECYCLE PATTERN:
 *
 * This pattern comes from game engines (Unity, Unreal) and embedded RTOS
 * (Real-Time Operating Systems). It provides clear lifecycle hooks:
 *
 * ENTER function:
 *   - Called ONCE when entering state
 *   - Responsibilities: initialisation, setup, reset timers
 *   - Example: display_show_attract(), reset score to 0
 *
 * UPDATE function:
 *   - Called EVERY FRAME while in state
 *   - Responsibilities: per-frame logic, check conditions, trigger transitions
 *   - Example: update LED position, check for button press
 *   - MUST execute quickly (no blocking!)
 *
 * EXIT function:
 *   - Called ONCE when leaving state
 *   - Responsibilities: cleanup, stop sounds, save data
 *   - Example: button_clear_state(), eeprom_write_high_score()
 *
 * LIFECYCLE EXAMPLE (STATE_ATTRACT -> STATE_PLAYING):
 *
 *   1. User presses button in STATE_ATTRACT
 *   2. Code calls: game_transition_to(STATE_PLAYING)
 *   3. game_transition_to() does:
 *      a) Call attract_exit()     // Clean up attract state
 *      b) current_state = STATE_PLAYING  // Change state variable
 *      c) Call playing_enter()    // Initialise playing state
 *   4. Next frame: playing_update() runs (and every frame after)
 *
 * COMPARISON TO OTHER PATTERNS:
 *
 * Game Engines (Unity):
 *   void OnEnable()  { ... }   // Like our enter()
 *   void Update()    { ... }   // Like our update()
 *   void OnDisable() { ... }   // Like our exit()
 *
 * Embedded RTOS (FreeRTOS tasks):
 *   void task_init()   { ... } // Like our enter()
 *   void task_run()    { ... } // Like our update()
 *   void task_cleanup(){ ... } // Like our exit()
 *
 ******************************************************************************/

#ifndef GAME_H
#define GAME_H

#include <Arduino.h>
#include "config.h"

/******************************************************************************
 * STATE HANDLER STRUCTURE - Function Pointer Pattern
 *
 * This struct holds three function pointers, one for each lifecycle phase.
 * It's the heart of our table-driven state machine pattern.
 *
 * WHAT ARE FUNCTION POINTERS?
 *
 * Function pointers are variables that store addresses of functions, allowing
 * dynamic function calls. They enable polymorphism in C (like virtual functions
 * in C++).
 *
 * DECLARATION SYNTAX:
 *   void (*function_name)(void);
 *   └─┬┘ └┬┘└────┬─────┘ └─┬┘
 *     │   │      │         └─ Parameter list (none)
 *     │   │      └─────────── Pointer name
 *     │   └────────────────── Dereference operator
 *     └────────────────────── Return type
 *
 * USAGE EXAMPLE:
 *   void hello(void) { Serial.println("Hello"); }
 *   void (*func_ptr)(void) = hello;  // Store function address
 *   func_ptr();  // Call function through pointer (prints "Hello")
 *
 * WHY USE FUNCTION POINTERS HERE?
 *
 * Alternative without function pointers (rigid):
 *   if (state == STATE_ATTRACT) {
 *       attract_update();  // ← Hardcoded function calls
 *   } else if (state == STATE_PLAYING) {
 *       playing_update();
 *   }
 *   // Adding new state requires editing this code
 *
 * With function pointers (flexible):
 *   state_handlers[state].update();  // ← Dynamic dispatch
 *   // Adding new state: just add entry to table
 *
 * STRUCT DEFINITION:
 *
 * typedef struct {
 *     void (*enter)(void);   // Initialisation function
 *     void (*update)(void);  // Per-frame logic function
 *     void (*exit)(void);    // Cleanup function
 * } StateHandler;
 *
 * Each GameState has one StateHandler containing its three lifecycle functions.
 *
 * LIFECYCLE RESPONSIBILITIES:
 *
 * enter() - Called once when ENTERING this state
 *   - Set up initial conditions for this state
 *   - Update display for this state (display_show_*)
 *   - Record entry timestamp (state_entry_time = millis())
 *   - Reset state-specific variables
 *   - Start animations if needed
 *   Example (attract_enter):
 *     chase_speed = INITIAL_CHASE_SPEED;
 *     display_show_attract(high_score);
 *
 * update() - Called EVERY FRAME while IN this state
 *   - Check for conditions that trigger state transitions
 *   - Update animations, positions, timers
 *   - Process input (button presses)
 *   - Must execute quickly (< 50ms, ideally < 1ms)
 *   - Must NOT block (no delay() calls!)
 *   Example (playing_update):
 *     update_chase_position();  // Non-blocking LED animation
 *     if (button_just_pressed()) {
 *         // Handle hit/miss logic, transition to next state
 *     }
 *
 * exit() - Called once when LEAVING this state
 *   - Clean up resources allocated in enter()
 *   - Stop sounds/animations
 *   - Clear button state (prevent stale edge detections)
 *   - Save persistent data (EEPROM)
 *   - Reset temporary variables
 *   Example (attract_exit):
 *     current_score = 0;  // Starting new game
 *     button_clear_state();  // Forget old button presses
 *
 * BENEFITS OF THIS PATTERN:
 *
 * 1. **Clear lifecycle**: No missed initialisation or cleanup
 *    - Without exit(): might forget to clear button state → bugs
 *    - With exit(): cleanup happens automatically on every transition
 *
 * 2. **Consistent structure**: All states follow same pattern
 *    - Easy to add new states (copy template, fill in 3 functions)
 *
 * 3. **Separation of concerns**:
 *    - Initialisation separate from per-frame logic
 *    - Cleanup separate from main logic
 *
 * 4. **Debugging support**:
 *    - Can add logging in enter/exit to track transitions
 *    - Can add validation (e.g., assert state is valid)
 *
 * TABLE-DRIVEN IMPLEMENTATION:
 *
 * See game.cpp:state_handlers[] for the actual table:
 *
 *   static const StateHandler state_handlers[5] = {
 *       [STATE_ATTRACT]     = {attract_enter,     attract_update,     attract_exit},
 *       [STATE_PLAYING]     = {playing_enter,     playing_update,     playing_exit},
 *       [STATE_RESULT]      = {result_enter,      result_update,      result_exit},
 *       [STATE_CELEBRATION] = {celebration_enter, celebration_update, celebration_exit},
 *       [STATE_GAME_OVER]   = {game_over_enter,   game_over_update,   game_over_exit}
 *   };
 *
 * Array index = GameState enum value. To call current state's update:
 *   state_handlers[current_state].update();
 *
 * This is called "table-driven" programming - behaviour controlled by data
 * (the table) rather than code (if/else chains).
 *
 * COMPARISON TO OBJECT-ORIENTED APPROACH:
 *
 * In C++, you might use virtual functions:
 *   class State {
 *       virtual void enter() = 0;
 *       virtual void update() = 0;
 *       virtual void exit() = 0;
 *   };
 *   class AttractState : public State { ... };
 *
 * Our C approach achieves the same polymorphism using function pointers,
 * avoiding the overhead of C++ virtual function tables and inheritance.
 * Perfect for memory-constrained embedded systems.
 ******************************************************************************/

typedef struct {
    void (*enter)(void);   // Called once when entering state
    void (*update)(void);  // Called every frame while in state
    void (*exit)(void);    // Called once when leaving state
} StateHandler;

/******************************************************************************
 * PUBLIC GAME INTERFACE
 *
 * These three functions are called from main.cpp and form the public API
 * of the game logic system.
 ******************************************************************************/

/**
 * game_init - Initialise game state machine
 *
 * Called once from main.cpp:setup() before entering main loop.
 *
 * Responsibilities:
 * - Initialise game variables (score, position, speed)
 * - Load high score from EEPROM
 * - Transition to initial state (STATE_ATTRACT)
 *
 * After calling game_init(), the game is ready to run. Call game_update()
 * every loop() iteration to execute the state machine.
 */
void game_init(void);

/**
 * game_update - Execute one frame of game logic
 *
 * Called every iteration of main.cpp:loop() (typically 1000-20000 times/sec
 * depending on current game state).
 *
 * Responsibilities:
 * - Call animation_update() to advance any playing animations
 * - Call current state's update() function
 * - Return quickly (must not block - watchdog timer requirement)
 *
 * CRITICAL: This function MUST execute in < 4 seconds (watchdog timeout).
 * In practice, executes in 0.1-2ms depending on state.
 *
 * EXECUTION FLOW:
 *   main.cpp:loop() calls game_update()
 *   └─> animation_update()  // Advance animations (non-blocking)
 *   └─> state_handlers[current_state].update()  // Current state logic
 *       └─> May call game_transition_to() to change state
 *   Returns to main.cpp:loop()
 *   main.cpp:loop() calls wdt_reset()
 *   Repeat
 */
void game_update(void);

/**
 * game_transition_to - Centralised state transition function
 * @param new_state: GameState to transition to
 *
 * THIS IS CRITICAL: ALL state changes MUST go through this function.
 * Never directly assign current_state = STATE_XXX!
 *
 * WHY CENTRALISED TRANSITIONS?
 *
 * Without centralisation:
 *   current_state = STATE_PLAYING;  // Forgot to call attract_exit()!
 *   playing_enter();  // Easy to forget, causes bugs
 *
 * With centralisation:
 *   game_transition_to(STATE_PLAYING);  // Automatic enter/exit handling
 *
 * TRANSITION SEQUENCE (implemented in game.cpp):
 *
 * 1. Call current state's exit() function
 *    - Cleanup old state (button_clear_state, save data, etc.)
 *
 * 2. Change state variable
 *    - current_state = new_state;
 *
 * 3. Call new state's enter() function
 *    - Initialise new state (display update, reset timers, etc.)
 *
 * This guarantees:
 * - Exit always called before leaving state
 * - Enter always called when entering state
 * - No state left partially initialised
 * - Consistent transition behaviour
 *
 * BENEFITS:
 *
 * 1. **Single point of control**: All transitions in one place
 *    - Want to log transitions? Add logging here (one place)
 *    - Want to validate transitions? Add validation here (one place)
 *
 * 2. **Prevents bugs**: Can't forget to call enter/exit
 *    - Compiler enforces: only way to change state is this function
 *    - Make current_state static in game.cpp → forced to use this function
 *
 * 3. **Debugging**: Can set breakpoint here to catch all transitions
 *
 * EXAMPLE USAGE (from game.cpp:attract_update):
 *   if (button_just_pressed()) {
 *       game_transition_to(STATE_PLAYING);  // Start game
 *   }
 *
 * Internally executes:
 *   attract_exit()     -> current_state = STATE_PLAYING -> playing_enter()
 *   Clear button          Change state variable          Show game screen
 *   Reset score                                          Record start time
 *
 * DESIGN PATTERN NAME:
 * This is called the "State Pattern" in object-oriented design, adapted for
 * embedded C using function pointers instead of virtual functions.
 */
void game_transition_to(GameState new_state);

#endif // GAME_H
