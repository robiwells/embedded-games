# Tutorial 7: State Lifecycle Pattern

## What You'll Learn
- Enter/update/exit lifecycle pattern for states
- Function pointers and struct-based dispatch
- Table-driven state machines
- Centralised state transitions
- Code organisation: splitting into multiple files (main.cpp, game.cpp/h)
- Professional embedded software architecture

## Prerequisites
- Tutorial 6: Simple State Machine (understanding states, transitions)

## Hardware Components
- Arduino Uno
- 8× LEDs (pins 2-9)
- 8× 220Ω resistors
- 1× Pushbutton (pin 10)

## Code Overview
This tutorial refactors the state machine from Tutorial 6 to use the **enter/update/exit** pattern used in professional embedded systems and game engines. Instead of scattered initialisation and cleanup code, each state has three clear functions:

- **enter()** - Called once when entering the state (initialisation)
- **update()** - Called every frame while in the state (per-frame logic)
- **exit()** - Called once when leaving the state (cleanup)

**New architecture:**
- Code split into main.cpp and game.cpp/h
- Table-driven state dispatch (function pointer array)
- Centralised transitions through `game_transition_to()`
- Clear lifecycle guarantees

## File Structure
```
07-state-lifecycle/
├── diagram.json          # Wokwi circuit
├── platformio.ini        # Build configuration
├── wokwi.toml           # Wokwi simulator settings
├── include/
│   ├── config.h         # Constants + enums
│   └── game.h           # Game logic interface
└── src/
    ├── main.cpp         # Entry point (setup/loop)
    └── game.cpp         # State machine implementation
```

## Key Concepts

### The Enter/Update/Exit Pattern

**Problem with Tutorial 6 approach:**
```cpp
if (current_state == STATE_PLAYING) {
    static bool initialised = false;
    if (!initialised) {
        // Initialisation code
        initialised = true;
    }

    // Per-frame logic

    if (transition_condition) {
        // Cleanup code
        initialised = false;
        current_state = NEW_STATE;
    }
}
```

**Issues:**
- Initialisation mixed with per-frame logic
- Easy to forget cleanup when transitioning
- Hard to see what happens when entering/leaving state
- Boilerplate `initialised` flag for every state

**Solution: Separate lifecycle phases:**
```cpp
void state_enter()  { /* Runs once on entry */ }
void state_update() { /* Runs every frame */ }
void state_exit()   { /* Runs once on exit */ }
```

**Benefits:**
- ✅ Clear separation of concerns
- ✅ Guaranteed initialisation (enter always called)
- ✅ Guaranteed cleanup (exit always called)
- ✅ Easy to understand what happens when
- ✅ No manual initialised flags needed

**Example: RESULT state (300ms pause)**

**Tutorial 6 (complex):**
```cpp
if (current_state == STATE_RESULT) {
    static uint32_t entry_time = 0;
    static bool entered = false;

    if (!entered) {
        entry_time = millis();
        entered = true;
    }

    if (millis() - entry_time >= 300) {
        entered = false;  // Must remember to reset!
        current_state = STATE_PLAYING;
    }
}
```

**Tutorial 7 (clean):**
```cpp
void result_enter() {
    state_entry_time = millis();  // Runs once
}

void result_update() {
    if (millis() - state_entry_time >= 300) {
        game_transition_to(STATE_PLAYING);
    }
}

void result_exit() {
    // Cleanup if needed
}
```

### Function Pointers

**What are function pointers?**
Variables that store addresses of functions, allowing dynamic function calls.

**Declaration:**
```cpp
void (*function_pointer)(void);  // Pointer to function taking no args, returning void
```

**Assignment:**
```cpp
void my_function(void) { /* ... */ }
function_pointer = my_function;  // Store function address
```

**Calling:**
```cpp
function_pointer();  // Calls my_function()
```

**Why use function pointers?**
Enables **polymorphism** in C - different behaviour based on runtime data.

### StateHandler Struct

```cpp
typedef struct {
    void (*enter)(void);
    void (*update)(void);
    void (*exit)(void);
} StateHandler;
```

**What it holds:**
Three function pointers representing a state's complete lifecycle.

**Example instance:**
```cpp
StateHandler playing_handler = {
    .enter = playing_enter,    // Function to call on entry
    .update = playing_update,  // Function to call each frame
    .exit = playing_exit       // Function to call on exit
};
```

**Calling:**
```cpp
playing_handler.enter();   // Calls playing_enter()
playing_handler.update();  // Calls playing_update()
playing_handler.exit();    // Calls playing_exit()
```

### Table-Driven State Machine

**The state handler table:**
```cpp
static const StateHandler state_handlers[4] = {
    [STATE_ATTRACT]   = {attract_enter,   attract_update,   attract_exit},
    [STATE_PLAYING]   = {playing_enter,   playing_update,   playing_exit},
    [STATE_RESULT]    = {result_enter,    result_update,    result_exit},
    [STATE_GAME_OVER] = {game_over_enter, game_over_update, game_over_exit}
};
```

**Designated initialisers** `[STATE_ATTRACT] = {...}` map enum values to handlers explicitly (robust against enum reordering).

**Dispatching:**
```cpp
// Instead of:
if (current_state == STATE_ATTRACT) {
    attract_update();
} else if (current_state == STATE_PLAYING) {
    playing_update();
}

// We do:
state_handlers[current_state].update();  // One line!
```

**Array indexing:**
- `current_state` is an integer (0, 1, 2, 3)
- `state_handlers[0]` = attract handler
- `state_handlers[1]` = playing handler
- `state_handlers[current_state]` = current state's handler

**Benefits:**
- Adding new state: Add to enum, add to table (no dispatcher changes)
- Compiler catches missing states (array size mismatch)
- Clean, scalable, professional pattern

### Centralised Transitions

**`game_transition_to()` - The state transition manager:**

```cpp
void game_transition_to(GameState new_state) {
    // 1. Call current state's exit function
    if (state_handlers[current_state].exit != NULL) {
        state_handlers[current_state].exit();
    }

    // 2. Change state variable
    current_state = new_state;

    // 3. Call new state's enter function
    if (state_handlers[current_state].enter != NULL) {
        state_handlers[current_state].enter();
    }
}
```

**Why centralise?**

**Without centralisation:**
```cpp
// Developer must remember 3 steps every time:
state_handlers[current_state].exit();  // 1. Easy to forget!
current_state = new_state;              // 2.
state_handlers[current_state].enter();  // 3. Easy to forget!
```

**With centralisation:**
```cpp
game_transition_to(STATE_PLAYING);  // ✅ All 3 steps automatic
```

**Guaranteed lifecycle:**
- Exit always called before leaving state
- State variable always updated
- Enter always called when entering state
- No way to forget cleanup or initialisation!

**Additional benefits:**
- Single place to add logging (`Serial.println("Transition to...")`)
- Single place to add validation (check if transition is legal)
- Single place to set breakpoints for debugging
- Consistent transition behaviour throughout application

### Code Organisation: Multiple Files

**Why split code?**

**Tutorial 6: Everything in main.cpp (monolithic)**
- ~200 lines all in one file
- Harder to navigate
- All implementation details visible
- Game logic mixed with hardware setup

**Tutorial 7: Separated concerns**
```
main.cpp:     Entry point, hardware init, calls game functions
game.h:       Public interface (what main.cpp can use)
game.cpp:     Game logic implementation (private details hidden)
config.h:     Shared constants and enums
```

**Benefits:**
- ✅ Clear interfaces (game.h shows exactly what's available)
- ✅ Implementation hiding (main.cpp doesn't see state functions)
- ✅ Easier to navigate (find code by logical module)
- ✅ Reusability (could use game.cpp in different projects)
- ✅ Testability (can compile game.cpp separately for testing)

**main.cpp responsibilities:**
- Hardware initialisation (pinMode, Serial.begin)
- Call `game_init()` in `setup()`
- Call `game_update()` in `loop()`
- That's it! (Simple, focused)

**game.cpp responsibilities:**
- State machine implementation
- All state handler functions
- Button handling
- Chase LED logic
- Score tracking

## Code Walkthrough

### game.h - Public Interface

```cpp
typedef struct {
    void (*enter)(void);
    void (*update)(void);
    void (*exit)(void);
} StateHandler;

void game_init(void);
void game_update(void);
void game_transition_to(GameState new_state);
```

**What main.cpp sees:**
Only these 3 functions. Implementation details (state handlers, button functions, etc.) are hidden in game.cpp.

### main.cpp - Hardware Setup

```cpp
void setup() {
    // Hardware initialisation
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        pinMode(LED_PIN_START + i, OUTPUT);
    }
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    Serial.begin(9600);

    game_init();  // Initialise game state machine
}

void loop() {
    game_update();  // Run one frame of game logic
}
```

**Clean and simple:**
- Sets up hardware
- Delegates to game module
- No game logic leaking into main

### game.cpp - State Implementation

**Table definition:**
```cpp
static const StateHandler state_handlers[4] = {
    [STATE_ATTRACT]   = {attract_enter,   attract_update,   attract_exit},
    [STATE_PLAYING]   = {playing_enter,   playing_update,   playing_exit},
    [STATE_RESULT]    = {result_enter,    result_update,    result_exit},
    [STATE_GAME_OVER] = {game_over_enter, game_over_update, game_over_exit}
};
```

**Example state: PLAYING**

```cpp
static void playing_enter(void) {
    Serial.println("Game started!");
    // Runs once when entering PLAYING state
}

static void playing_update(void) {
    update_chase_position();  // Animate LED

    if (button_just_pressed()) {
        if (/* hit target */) {
            current_score += BULLSEYE_SCORE;
            game_transition_to(STATE_RESULT);  // Centralised transition
        } else {
            // Handle miss
            game_transition_to(STATE_GAME_OVER);
        }
    }
    // Runs every frame while in PLAYING state
}

static void playing_exit(void) {
    // Cleanup (currently none needed)
    // Runs once when leaving PLAYING state
}
```

**Lifecycle example (successful hit):**
```
1. Button pressed in PLAYING state
2. playing_update() calls game_transition_to(STATE_RESULT)
3. game_transition_to() calls playing_exit() ← cleanup old state
4. game_transition_to() sets current_state = STATE_RESULT
5. game_transition_to() calls result_enter() ← initialise new state
6. Next frame: result_update() runs
```

## Running the Code

### Using PlatformIO + Wokwi:
1. Open this folder in VS Code
2. Build: `pio run`
3. Upload to Wokwi simulator
4. Start simulation

### What You Should See:
**Behaviour identical to Tutorial 6:**
- Press button → game starts
- Hit green zone → score increases, 300ms pause
- Miss → game over, 2 second pause, return to attract

**Internal architecture is completely different:**
- Enter/exit functions guarantee correct initialisation/cleanup
- Table-driven dispatch (no if/else chains)
- Code organised into logical modules

### Debugging State Transitions:

Add logging to `game_transition_to()`:
```cpp
void game_transition_to(GameState new_state) {
    Serial.print("Transition: ");
    Serial.print(current_state);
    Serial.print(" -> ");
    Serial.println(new_state);

    // ... rest of function
}
```

Output:
```
Transition: 0 -> 1  (ATTRACT -> PLAYING)
Hit! Score: 10
Transition: 1 -> 2  (PLAYING -> RESULT)
Transition: 2 -> 1  (RESULT -> PLAYING)
Miss! Final score: 10
Transition: 1 -> 3  (PLAYING -> GAME_OVER)
Transition: 3 -> 0  (GAME_OVER -> ATTRACT)
```

## Exercises

### Easy: Add state entry logging
In each state's `enter()` function, add:
```cpp
Serial.println("Entered STATE_NAME");
```

Watch serial output to see lifecycle events.

### Medium: Add state validation
In `game_transition_to()`, prevent invalid transitions:
```cpp
// Can't go from RESULT directly to ATTRACT (must go through PLAYING or GAME_OVER)
if (current_state == STATE_RESULT && new_state == STATE_ATTRACT) {
    Serial.println("Invalid transition!");
    return;  // Ignore transition
}
```

### Hard: Add animation time budget tracking
Measure how long each state's update function takes:
```cpp
void game_update(void) {
    uint32_t start = micros();
    state_handlers[current_state].update();
    uint32_t elapsed = micros() - start;

    Serial.print("Update time: ");
    Serial.print(elapsed);
    Serial.println(" μs");
}
```

Identify which states are slowest.

## Next Steps
- **Next tutorial:** [08-display-and-sound](../08-display-and-sound/) - Add LCD and buzzer
- **New concepts:** I2C protocol, LiquidCrystal library, tone() function, hardware abstraction layer

## Common Pitfalls

### Direct state assignment instead of transition function
**Wrong:**
```cpp
current_state = STATE_PLAYING;  // ❌ Skips exit/enter functions!
```

**Right:**
```cpp
game_transition_to(STATE_PLAYING);  // ✅ Proper lifecycle
```

### Forgetting static keyword for state functions
**Wrong:**
```cpp
void attract_enter(void) { /* ... */ }  // ❌ Global visibility
```

**Right:**
```cpp
static void attract_enter(void) { /* ... */ }  // ✅ File-local (private)
```

Static makes functions private to game.cpp. Without it, functions are globally visible (namespace pollution).

### NULL check before calling function pointers
Our code checks for NULL:
```cpp
if (state_handlers[current_state].enter != NULL) {
    state_handlers[current_state].enter();
}
```

This is defensive. All our handlers have all three functions, but NULL checks prevent crashes if someone forgets to implement a function.

### Shared timing variable pitfalls
We use `state_entry_time` for both RESULT and GAME_OVER states. This works because:
- Only one state active at a time
- Value is set in enter() function before being used
- If we needed persistent timing across states, we'd need separate variables

## Further Reading
- [Function Pointers in C](https://www.cprogramming.com/tutorial/function-pointers.html)
- [State Design Pattern](https://en.wikipedia.org/wiki/State_pattern)
- [Organizing Code Files in C](https://stackoverflow.com/questions/1945846/what-should-go-into-an-h-file)

## Why This Matters

This tutorial teaches **professional embedded architecture patterns**:

**Where you'll see this:**
- **Game engines:** Unity, Unreal Engine use enter/update/exit lifecycle
- **RTOS:** FreeRTOS tasks have init/run/cleanup phases
- **Industrial control:** PLC state machines use similar patterns
- **Automotive:** ECU software uses table-driven state machines
- **Aerospace:** Safety-critical systems use formal state machine patterns

**Pattern applications:**
- UI state machines (menu navigation, dialogs)
- Protocol handlers (network state machines)
- Motor control (homing, running, stopping states)
- Process control (startup, running, shutdown, emergency states)

The architecture you learned here is **production-grade**. Companies use this exact pattern in shipping products. Master it now, and you're ready for professional embedded development.
