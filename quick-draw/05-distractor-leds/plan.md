# Tutorial 05: Distractor LEDs

## Purpose

Teach the **state validation pattern** where the ISR captures moment-in-time state data, and the main loop validates it with full context. This tutorial adds distractor LED flashes during countdown to create ambiguity that requires sophisticated validation.

## Learning Objectives

- Capture game state in ISR for later validation
- Distinguish between ISR data capture and main loop validation
- Handle timing race conditions (button pressed during distractor vs draw LED)
- Use `volatile` enum types for state sharing
- Implement parallel timing (countdown + distractor flashes)

## Key Concepts

### The State Validation Pattern

**The Problem:**

```
Timeline during STATE_READY:
  0ms: "Get Ready..." displayed
  800ms: Distractor LED 1 flashes (200ms)
  1500ms: Distractor LED 2 flashes (200ms)
  2500ms: STATE_DRAW entered, draw LED turns on
```

**Challenge:** If button pressed at 850ms, was it during distractor flash or between flashes?

**Wrong Approach (Tutorial 04):**
```cpp
void ready_update(void) {
    if (button_was_pressed()) {
        // Always false start - but what if LED was on?
        game_transition_to(STATE_FALSE_START);
    }
}
```

**Problem:** Doesn't account for state changes between button press and flag check!

### ISR State Capture

**Solution:** ISR captures the game state at the exact moment of the press:

```cpp
// ISR-accessible state variable (MUST be volatile)
volatile GameState state_at_press = STATE_ATTRACT;

void button_isr(void) {
    uint32_t current_time = micros();

    if ((current_time - last_interrupt_time_us) > DEBOUNCE_US) {
        last_interrupt_time_us = current_time;
        button_press_time_us = current_time;

        // NEW: Capture state at press moment
        state_at_press = game_get_state();

        button_pressed_flag = true;
    }
}
```

### Main Loop Validation

**Main loop checks captured state, not current state:**

```cpp
void draw_update(void) {
    if (button_was_pressed()) {
        // Validate: Was state actually DRAW when button pressed?
        if (button_get_state_at_press() == STATE_DRAW) {
            // Valid press - calculate reaction time
            uint32_t reaction_us = button_get_press_time() - draw_start_time;
            game_transition_to(STATE_RESULT);
        } else {
            // Invalid - pressed during distractor or wrong state
            game_transition_to(STATE_FALSE_START);
        }
    }

    // Check timeout...
}
```

**Key Insight:**
- ISR captures **moment-in-time snapshot** (microsecond-accurate)
- Main loop validates with **full game context** (knows about distractors)

### Distractor LED Timing

**Parallel State Machine:**

```cpp
static uint32_t next_distractor_time = 0;
static bool distractor_active = false;

void ready_update(void) {
    uint32_t current_time = millis();

    // Handle distractor timing
    if (!distractor_active && current_time >= next_distractor_time) {
        // Turn on distractor LED
        led_set_distractor(random(0, 4));  // LEDs 0-3 are distractors
        distractor_active = true;
        next_distractor_time = current_time + DISTRACTOR_DURATION;
    } else if (distractor_active && current_time >= next_distractor_time) {
        // Turn off distractor LED
        led_clear_all();
        distractor_active = false;

        // Schedule next distractor (if time remaining)
        uint32_t time_remaining = ready_duration - (current_time - state_entry_time);
        if (time_remaining > DISTRACTOR_MAX_MS) {
            next_distractor_time = current_time + random(DISTRACTOR_MIN_MS, DISTRACTOR_MAX_MS);
        }
    }

    // Check for button press (regardless of distractor state)
    if (button_was_pressed()) {
        game_transition_to(STATE_FALSE_START);  // Any press during READY is false start
    }

    // Check if countdown expired
    if (current_time - state_entry_time >= ready_duration) {
        game_transition_to(STATE_DRAW);
    }
}
```

## Implementation Checklist

### Files to Modify from Tutorial 04

- [ ] `include/config.h` - Add distractor timing constants
- [ ] `include/hardware.h` - Add `led_set_distractor()`, `button_get_state_at_press()`
- [ ] `src/hardware.cpp` - Capture state in ISR, implement distractor LED function
- [ ] `src/game.cpp` - Add distractor timing logic, validate state in draw_update

### Changes to `config.h`

```cpp
// Distractor LED timing
#define DISTRACTOR_MIN_MS 500   // Minimum time before first distractor
#define DISTRACTOR_MAX_MS 1500  // Maximum time before distractor
#define DISTRACTOR_DURATION 100 // How long distractor LED stays on (ms)
```

### Changes to `hardware.h`

```cpp
// Get game state at time of button press (for validation)
GameState button_get_state_at_press(void);

// Set distractor LED (one of the LEDs before the draw LED)
void led_set_distractor(uint8_t index);
```

### Changes to `hardware.cpp`

#### Add State Capture Variable

```cpp
// ISR-accessible variables
volatile bool button_pressed_flag = false;
volatile uint32_t button_press_time_us = 0;
volatile uint32_t last_interrupt_time_us = 0;
volatile GameState state_at_press = STATE_ATTRACT;  // NEW
```

#### Update ISR

```cpp
void button_isr(void) {
    uint32_t current_time = micros();

    if ((current_time - last_interrupt_time_us) > DEBOUNCE_US) {
        last_interrupt_time_us = current_time;
        button_press_time_us = current_time;

        // NEW: Capture state at press moment
        state_at_press = game_get_state();

        button_pressed_flag = true;
    }
}
```

#### Add Accessor Function

```cpp
GameState button_get_state_at_press(void) {
    return state_at_press;
}
```

#### Add Distractor LED Function

```cpp
void led_set_distractor(uint8_t index) {
    // Use one of the LEDs before the draw LED (middle one)
    // Draw LED is at index 4 (middle of 0-7), so use 0-3 for distractors
    if (index < 4) {
        led_set(index);
    }
}
```

#### Update Clear Function

```cpp
void button_clear_state(void) {
    button_pressed_flag = false;
    button_press_time_us = 0;
    last_interrupt_time_us = 0;
    state_at_press = game_get_state();  // NEW: Reset to current state
}
```

### Changes to `game.cpp`

#### Add Distractor Variables

```cpp
// State-specific variables
static uint32_t ready_duration = 0;
static uint32_t draw_start_time = 0;
static uint32_t next_distractor_time = 0;    // NEW
static uint8_t distractor_led_index = 0;     // NEW
static bool distractor_active = false;       // NEW
```

#### Update ready_enter

```cpp
static void ready_enter(void) {
    led_clear_all();
    display_show_ready();
    button_clear_state();

    current_round++;

    ready_duration = random(READY_MIN_MS, READY_MAX_MS);
    state_entry_time = millis();

    // NEW: Schedule first distractor
    next_distractor_time = state_entry_time + random(DISTRACTOR_MIN_MS, DISTRACTOR_MAX_MS);
    distractor_active = false;
}
```

#### Update ready_update

```cpp
static void ready_update(void) {
    uint32_t current_time = millis();

    // NEW: Handle distractor LEDs
    if (!distractor_active && current_time >= next_distractor_time) {
        distractor_led_index = random(0, 4);
        led_set_distractor(distractor_led_index);
        distractor_active = true;
        next_distractor_time = current_time + DISTRACTOR_DURATION;
    } else if (distractor_active && current_time >= next_distractor_time) {
        led_clear_all();
        distractor_active = false;

        uint32_t time_remaining = ready_duration - (current_time - state_entry_time);
        if (time_remaining > DISTRACTOR_MAX_MS) {
            next_distractor_time = current_time + random(DISTRACTOR_MIN_MS, DISTRACTOR_MAX_MS);
        }
    }

    // Check for false start (ANY press during READY is invalid)
    if (button_was_pressed()) {
        game_transition_to(STATE_FALSE_START);
        return;
    }

    // Check if ready period expired
    if (current_time - state_entry_time >= ready_duration) {
        game_transition_to(STATE_DRAW);
    }
}
```

#### Update draw_update (State Validation)

```cpp
static void draw_update(void) {
    if (button_was_pressed()) {
        // NEW: Validate that press happened during DRAW state
        if (button_get_state_at_press() == STATE_DRAW) {
            // Valid press - calculate reaction time
            uint32_t press_time = button_get_press_time();
            uint32_t reaction_us = press_time - draw_start_time;

            if (reaction_us >= REACTION_MIN_US && reaction_us <= REACTION_MAX_US) {
                uint16_t reaction_ms = reaction_us / 1000;
                round_times[current_round - 1] = reaction_ms;
                game_transition_to(STATE_RESULT);
            } else {
                round_times[current_round - 1] = REACTION_MAX_MS;
                game_transition_to(STATE_RESULT);
            }
        } else {
            // Invalid - pressed during distractor or wrong state
            game_transition_to(STATE_FALSE_START);
        }
        return;
    }

    // Check timeout...
}
```

## Expected Behavior

### Distractor Pattern

**Timeline Example:**
```
Time:   Action:
0ms     "Get Ready..." displayed
600ms   LED 1 (index 2) flashes on
700ms   LED 1 off
1200ms  LED 3 (index 0) flashes on
1300ms  LED 3 off
1900ms  LED 2 (index 1) flashes on
2000ms  LED 2 off
2800ms  STATE_DRAW → LED 4 (middle) turns on
```

### Valid Press Scenarios

1. **Press during draw LED** → Valid reaction time recorded
2. **Press between distractors** → False start (was in STATE_READY)
3. **Press during distractor flash** → False start (was in STATE_READY)

### What Students Should See

1. **Distractors are distracting!** - LEDs flash during countdown
2. **Can't cheat** - Pressing during distractor → false start
3. **Must wait for middle LED** - Only middle LED press is valid
4. **State capture works** - Even rapid state changes handled correctly

## Teaching Moments

### Why Capture State in ISR?

**Demonstration:**

```cpp
// BAD: Check current state (race condition!)
void draw_update(void) {
    if (button_was_pressed()) {
        if (game_get_state() == STATE_DRAW) {  // WRONG!
            // What if state changed between press and this check?
        }
    }
}

// GOOD: Check captured state (accurate!)
void draw_update(void) {
    if (button_was_pressed()) {
        if (button_get_state_at_press() == STATE_DRAW) {  // RIGHT!
            // State captured at press moment by ISR
        }
    }
}
```

**Race Condition Example:**
```
Time:    Event:
1000µs:  Button pressed (ISR captures STATE_DRAW)
1001µs:  Reaction time calculated
1002µs:  game_transition_to(STATE_RESULT) called
1003µs:  draw_update() checks game_get_state() → Returns STATE_RESULT!
```

**Solution:** ISR captures state at microsecond 1000, saved in `volatile` variable.

### ISR Responsibilities vs Main Loop Responsibilities

**ISR (Moment-in-Time Capture):**
- Timestamp: `micros()`
- State: `game_get_state()`
- Flag: Set `button_pressed_flag`

**Main Loop (Contextual Validation):**
- Was this a valid state for the button press?
- Was this within valid timing bounds?
- What should happen based on current game logic?

**Analogy:**
"The ISR is a witness who records exactly what they saw at a specific moment. The main loop is the judge who decides what that evidence means."

### `volatile` with Enums

**Question:** "Why is `state_at_press` declared `volatile GameState`?"

**Answer:**
```cpp
volatile GameState state_at_press = STATE_ATTRACT;
```

Because it's accessed by both ISR (write) and main loop (read):
- ISR writes: `state_at_press = game_get_state();`
- Main loop reads: `if (button_get_state_at_press() == STATE_DRAW)`

**Without `volatile`:** Compiler might cache the value, missing ISR updates.

## Testing Checklist

### Basic Functionality
- [ ] Distractor LEDs flash during countdown (2-4 times)
- [ ] Distractors are random LEDs (index 0-3)
- [ ] Distractors flash for ~100ms each
- [ ] Draw LED (middle, index 4) is different from distractors

### State Validation
- [ ] Press during distractor flash → False start
- [ ] Press between distractors → False start
- [ ] Press during draw LED → Valid reaction time
- [ ] Press right as LED transitions (edge case) → Correct state captured

### Edge Cases
- [ ] Very fast reaction (<100ms) → Check state validation still works
- [ ] State transitions during ISR → No race conditions
- [ ] Multiple distractors in quick succession → Timing doesn't break

## Debugging Tips

**Add temporary debug output in ISR:**
```cpp
void button_isr(void) {
    // ... debounce check ...

    state_at_press = game_get_state();

    // TEMPORARY: Blink LED to show state captured
    // (Remove for production - this violates ISR constraints!)
    // digitalWrite(DEBUG_LED, state_at_press == STATE_DRAW ? HIGH : LOW);

    button_pressed_flag = true;
}
```

**Serial debugging in main loop:**
```cpp
if (button_was_pressed()) {
    Serial.print("Press at state: ");
    Serial.println(button_get_state_at_press());
}
```

## Key Differences from Tutorial 04

| Aspect | Tutorial 04 | Tutorial 05 |
|--------|-------------|-------------|
| **Distractors** | None | 2-4 random LED flashes |
| **State Capture** | Not needed | ISR captures state |
| **Validation** | State-based (current) | Snapshot-based (captured) |
| **Complexity** | Simple timing | Parallel timing (countdown + distractors) |
| **False Starts** | Early press only | Early press OR distractor press |

## Transition to Tutorial 06

**The Hook:** "Now we have a solid interrupt-driven game with state validation. Let's add the final features: multi-round gameplay, statistics tracking, EEPROM persistence, and celebration animations!"

**Preview:** Tutorial 06 is the complete, production-quality game with:
- 5-round gameplay
- Average/best time tracking
- EEPROM persistence
- New record celebration
- STATE_ROUND_COMPLETE and STATE_NEW_RECORD states

## Key Takeaways

✅ ISR captures moment-in-time state for later validation
✅ Main loop validates context using ISR-captured data
✅ Prevents race conditions (state changes between press and check)
✅ `volatile` required for enum types shared between ISR and main
✅ Parallel timing in state machines (countdown + distractor scheduler)
✅ Defensive programming: Validate assumptions with captured data

## Advanced Concepts Introduced

### Two-Level State Machines

1. **Main game state machine** (ATTRACT → READY → DRAW → RESULT)
2. **Distractor sub-state machine** (active/inactive within READY state)

### Event Timestamping Pattern

Common in embedded systems:
```
Hardware event → ISR captures timestamp + context → Main loop processes with full context
```

Examples:
- Button press (this game)
- Sensor trigger (alarm systems)
- Communication packet arrival (protocols)
- Timer overflow (precision timing)

## File Status

- ✅ `platformio.ini` - No changes from Tutorial 04
- ✅ `include/config.h` - Modified (add distractor timing)
- ✅ `include/hardware.h` - Modified (add state accessor, distractor LED)
- ✅ `include/game.h` - No changes from Tutorial 04
- ✅ `src/hardware.cpp` - Modified (capture state in ISR, distractor LED)
- ✅ `src/game.cpp` - Modified (distractor timing, state validation)
- ✅ `src/main.cpp` - No changes from Tutorial 04

**Ready to build and master the state validation pattern!**
