# Tutorial 6: Simple State Machine

## What You'll Learn
- State machine fundamentals (states, transitions, events)
- Enum types for state representation
- Conditional state dispatch with if/else
- Game flow control (attract → playing → game over → attract)
- Static local variables for state-specific persistence

## Prerequisites
- Tutorial 5: Button Debounce (understanding button handling, scoring)

## Hardware Components
- Arduino Uno
- 8× LEDs (pins 2-9)
- 8× 220Ω resistors
- 1× Pushbutton (pin 10)

## Code Overview
This tutorial transforms our simple game into a proper state machine with three game modes:
1. **ATTRACT**: Waiting screen, press button to start
2. **PLAYING**: Active gameplay, scoring points
3. **GAME_OVER**: 2-second pause, then return to attract

**State flow:**
```
ATTRACT ──(button press)──> PLAYING ──(hit)──> PLAYING (continue)
   ↑                                    │
   │                                    │(miss)
   │                                    ↓
   └────(2 sec timer)──── GAME_OVER <──┘
```

## File Structure
```
06-simple-state-machine/
├── diagram.json          # Wokwi circuit
├── platformio.ini        # Build configuration
├── wokwi.toml           # Wokwi simulator settings
├── include/
│   └── config.h         # Constants + GameState enum
└── src/
    └── main.cpp         # State machine logic
```

## Key Concepts

### What Is a State Machine?

A **state machine** is a system that is always in exactly one state. It transitions between states based on events.

**Real-world example: Traffic Light**
```
States: GREEN, YELLOW, RED
Events: Timer expires

GREEN ──(timer)──> YELLOW ──(timer)──> RED ──(timer)──> GREEN
```

**Our game example:**
```
States: ATTRACT, PLAYING, GAME_OVER
Events: Button press, hit, miss, timer

ATTRACT ──(button)──> PLAYING
PLAYING ──(hit)────> PLAYING
PLAYING ──(miss)───> GAME_OVER
GAME_OVER ──(timer)──> ATTRACT
```

### Why Use State Machines?

**Without state machine (messy):**
```cpp
bool playing = false;
bool game_over = false;
uint32_t game_over_start = 0;

void loop() {
    if (!playing && !game_over) {
        // Attract mode logic
        if (button) playing = true;
    }
    if (playing && !game_over) {
        // Playing logic
        if (miss) { playing = false; game_over = true; /* ... */ }
    }
    if (game_over) {
        // Game over logic
        if (timer) { game_over = false; /* ... */ }
    }
}
```

**Problems:**
- Easy to get into invalid states (playing && game_over both true?)
- Hard to understand current behaviour
- Difficult to add new states
- Logic scattered, repeated conditions

**With state machine (clean):**
```cpp
enum GameState { ATTRACT, PLAYING, GAME_OVER };
GameState current_state = ATTRACT;

void loop() {
    if (current_state == ATTRACT) {
        // Attract logic only
    } else if (current_state == PLAYING) {
        // Playing logic only
    } else if (current_state == GAME_OVER) {
        // Game over logic only
    }
}
```

**Benefits:**
- ✅ Always in exactly one valid state
- ✅ Clear behaviour based on current state
- ✅ Easy to add states (add to enum, add to if/else)
- ✅ Each state's logic is isolated
- ✅ State transitions are explicit and trackable

### Enum: Defining Named States

```cpp
enum GameState {
    STATE_ATTRACT,
    STATE_PLAYING,
    STATE_GAME_OVER
};
```

**What is an enum?**
An enumeration defines a set of named constants. Behind the scenes, they're just integers:

```cpp
STATE_ATTRACT = 0
STATE_PLAYING = 1
STATE_GAME_OVER = 2
```

**Why use enum instead of #define or raw integers?**

**Bad (magic numbers):**
```cpp
int state = 0;  // What does 0 mean?
if (state == 2) { /* ... */ }  // What's state 2?
```

**Bad (#define):**
```cpp
#define ATTRACT 0
#define PLAYING 1
int state = ATTRACT;
```
Problem: `state = 99;` is valid but meaningless (no type safety)

**Good (enum):**
```cpp
enum GameState { STATE_ATTRACT, STATE_PLAYING, STATE_GAME_OVER };
GameState state = STATE_ATTRACT;  // Type-safe
// state = 99; // Compiler warning (wrong type)
```

**Benefits:**
- Self-documenting (STATE_ATTRACT vs 0)
- Type safety (can't assign random integers)
- Autocomplete in IDE (shows all valid states)
- Easier to maintain (add/remove states, compiler catches errors)

### State Dispatching with if/else

Our current approach: **conditional dispatch**

```cpp
if (current_state == STATE_ATTRACT) {
    // Attract mode behaviour
} else if (current_state == STATE_PLAYING) {
    // Playing behaviour
} else if (current_state == STATE_GAME_OVER) {
    // Game over behaviour
}
```

**Execution:**
1. Check if current_state == STATE_ATTRACT
2. If true: run attract code, skip others
3. If false: check next condition
4. Only ONE block executes per loop

**Alternative: switch statement** (equivalent):
```cpp
switch (current_state) {
    case STATE_ATTRACT:
        // Attract logic
        break;
    case STATE_PLAYING:
        // Playing logic
        break;
    case STATE_GAME_OVER:
        // Game over logic
        break;
}
```

Both approaches work. We use if/else here for simplicity. Later tutorials will use more advanced patterns.

### State Transitions

**Changing states:**
```cpp
current_state = STATE_PLAYING;  // Transition to PLAYING state
```

**When to transition:**
- **Event occurs** (button press, hit, miss)
- **Timer expires** (game over delay)
- **Condition met** (score threshold, etc.)

**Transition examples:**
```cpp
// In ATTRACT state:
if (button_just_pressed()) {
    score = 0;                        // Reset score for new game
    current_state = STATE_PLAYING;    // Start game
}

// In PLAYING state:
if (miss) {
    current_state = STATE_GAME_OVER;  // End game
}

// In GAME_OVER state:
if (timer_expired) {
    current_state = STATE_ATTRACT;    // Return to attract
}
```

### Static Local Variables

**Problem:** GAME_OVER state needs to track when it started, but only while in that state.

**Bad (global variable):**
```cpp
uint32_t game_over_time = 0;  // Global, always exists

void loop() {
    if (current_state == GAME_OVER) {
        if (millis() - game_over_time >= 2000) { /* ... */ }
    }
}
```
Wastes RAM (variable exists even when not in GAME_OVER state)

**Good (static local variable):**
```cpp
void loop() {
    if (current_state == GAME_OVER) {
        static uint32_t game_over_time = 0;  // Only visible in this block
        static bool started = false;          // But persists between calls

        if (!started) {
            game_over_time = millis();  // Record entry time
            started = true;
        }

        if (millis() - game_over_time >= 2000) {
            started = false;  // Reset for next time
            current_state = STATE_ATTRACT;
        }
    }
}
```

**static keyword effects:**
- **Inside function:** Variable persists between calls (not on stack)
- **Visibility:** Only accessible within that scope
- **Initialization:** Only runs once (on first entry)

**Benefits:**
- Encapsulation (variable only accessible where needed)
- Persistence (retains value between loop() calls)
- Clear ownership (belongs to that state)

## Code Walkthrough

### New State Variable

```cpp
GameState current_state = STATE_ATTRACT;  // Start in attract mode
```

### State Logic: ATTRACT

```cpp
if (current_state == STATE_ATTRACT) {
    if (button_just_pressed()) {
        score = 0;                        // Reset score
        current_state = STATE_PLAYING;    // Start game
        Serial.println("Game started!");
    }
}
```

**Behaviour:**
- Chase LED bounces (shared with all states)
- Waiting for button press
- Button press → reset score, switch to PLAYING

### State Logic: PLAYING

```cpp
else if (current_state == STATE_PLAYING) {
    if (button_just_pressed()) {
        if (current_position >= TARGET_ZONE_START && current_position <= TARGET_ZONE_END) {
            // Hit!
            score += 10;
            Serial.print("Hit! Score: ");
            Serial.println(score);
            // Stay in PLAYING state (continue game)
        } else {
            // Miss!
            Serial.print("Miss! Final score: ");
            Serial.println(score);

            if (score > high_score) {
                high_score = score;
                Serial.print("New high score: ");
                Serial.println(high_score);
            }

            current_state = STATE_GAME_OVER;  // Transition to game over
        }
    }
}
```

**Behaviour:**
- Active gameplay
- Button press checks hit/miss
- Hit → add points, stay in PLAYING
- Miss → save high score, transition to GAME_OVER

### State Logic: GAME_OVER

```cpp
else if (current_state == STATE_GAME_OVER) {
    static uint32_t game_over_time = 0;
    static bool game_over_started = false;

    if (!game_over_started) {
        game_over_time = millis();  // Record when we entered this state
        game_over_started = true;
    }

    if (millis() - game_over_time >= 2000) {
        // 2 seconds passed
        game_over_started = false;           // Reset flag for next game over
        current_state = STATE_ATTRACT;       // Return to attract
        Serial.println("Press button to start!");
    }
}
```

**Behaviour:**
- 2-second pause (let player see final score)
- After 2 seconds, return to ATTRACT
- Uses static variables to track state entry

## Running the Code

### Using PlatformIO + Wokwi:
1. Open this folder in VS Code
2. Build: `pio run`
3. Upload to Wokwi simulator
4. Start simulation
5. Open Serial Monitor

### What You Should See:

**Serial output:**
```
Light Chaser - Press button to start!
[Press button]
Game started!
Hit! Score: 10
Hit! Score: 20
Hit! Score: 30
Miss! Final score: 30
New high score: 30
[2 second pause]
Press button to start!
```

**State progression:**
```
Boot → ATTRACT (waiting)
Button press → PLAYING (active gameplay)
Hit → PLAYING (continue, score increases)
Hit → PLAYING (continue, score increases)
Miss → GAME_OVER (2 second pause)
Timer → ATTRACT (ready for new game)
```

## Exercises

### Easy: Add a countdown
In GAME_OVER state, print a countdown:
```
Game Over in 2...
Game Over in 1...
Press button to start!
```

Hint: Use `millis()` to check every second.

### Medium: Add a pause after hit
Add a STATE_RESULT that pauses for 300ms after each hit, then returns to PLAYING. This gives the player a moment to see they scored before the LED continues moving.

Flow: `PLAYING ──(hit)──> RESULT ──(300ms)──> PLAYING`

### Hard: Implement lives system
Instead of instant game over on first miss:
- Start with 3 lives
- Miss → lose 1 life
- 0 lives → game over
- Display lives in serial output

You'll need a `lives` variable and modified miss logic.

## Next Steps
- **Next tutorial:** [07-state-lifecycle](../07-state-lifecycle/) - Enter/exit/update pattern
- **New concepts:** StateHandler struct, function pointers, table-driven dispatch
- **Architecture:** Split code into main.cpp, game.cpp, game.h

## Common Pitfalls

### Invalid state values
**Problem:**
```cpp
current_state = 99;  // Not a valid state!
```

**Solution:**
Use enum type (compiler may warn) and validate state:
```cpp
if (current_state >= STATE_ATTRACT && current_state <= STATE_GAME_OVER) {
    // Valid state
}
```

### Forgotten transitions
**Problem:**
```cpp
if (current_state == GAME_OVER) {
    // Wait 2 seconds...
    // Forgot to transition back to ATTRACT!
    // Stuck in GAME_OVER forever!
}
```

**Solution:**
Always plan exit conditions for every state. Draw state diagram first.

### Shared variables not reset
**Problem:**
```cpp
// Start new game
current_state = STATE_PLAYING;
// Forgot to reset score! Player starts with previous score.
```

**Solution:**
Reset all relevant variables when transitioning:
```cpp
if (button_just_pressed()) {
    score = 0;                     // ✅ Reset score
    current_state = STATE_PLAYING;
}
```

### State-specific setup not run
**Problem:**
```cpp
static uint32_t timer = millis();  // Only initializes once!
static bool started = false;

// First time in GAME_OVER: works
// Second time in GAME_OVER: started is still true! Logic skips setup
```

**Solution:**
Reset state-specific flags on exit:
```cpp
if (millis() - timer >= 2000) {
    started = false;  // ✅ Reset for next time
    current_state = STATE_ATTRACT;
}
```

## Further Reading
- [State Machines on Arduino](https://www.norwegiancreations.com/2017/03/state-machines-and-arduino-implementation/)
- [Finite State Machines (Wikipedia)](https://en.wikipedia.org/wiki/Finite-state_machine)
- [Enum Types in C++](https://www.arduino.cc/reference/en/language/variables/data-types/enum/)

## Why This Matters

State machines are **everywhere** in embedded systems:

**Consumer electronics:**
- Washing machines (fill, wash, rinse, spin states)
- Microwave ovens (idle, running, paused, complete)
- Game consoles (boot, menu, playing, paused)

**Industrial:**
- CNC machines (homing, idle, running, error)
- Robots (idle, moving, gripping, error recovery)
- Process controllers (startup, running, shutdown, emergency stop)

**Communication:**
- Network protocols (connecting, connected, disconnecting)
- UART parsers (idle, receiving, complete, error)

The pattern you learned here scales from simple games to industrial control systems. Master state machines and you can design any complex behaviour clearly and reliably.
