# Tutorial 04: False Start Detection

## Purpose

Teach **defensive interrupt usage** across multiple game states. The interrupt remains enabled during the countdown phase, allowing the game to detect and penalize early button presses (false starts).

## Learning Objectives

- Use interrupts defensively across multiple states
- Validate button presses based on game context
- Add penalty states to the state machine
- Understand that interrupts are always active (not just in one state)
- Learn to check flags in multiple state handlers

## Key Concepts

### Interrupts Are Always Active

**Important Realization:**

Unlike polling (where you choose when to check), interrupts fire **whenever the hardware event occurs**, regardless of game state.

```cpp
// Polling (Tutorial 01) - Only checked in draw state
void draw_update(void) {
    if (button_just_pressed()) {  // Only checked here
        // Handle press
    }
}

// Interrupts (Tutorial 04) - ISR fires in ANY state
void button_isr(void) {
    // Fires during attract, ready, draw, result, etc.
    button_pressed_flag = true;
}
```

**Implication:** Multiple state handlers must check the button flag and decide if the press is valid for that state.

### Defensive State Checking

**Pattern:**

```cpp
void ready_update(void) {
    // Check for INVALID press (during countdown)
    if (button_was_pressed()) {
        game_transition_to(STATE_FALSE_START);  // Penalty!
        return;
    }

    // Check if countdown expired
    if (millis() - state_entry_time >= ready_duration) {
        game_transition_to(STATE_DRAW);  // Now valid to press
    }
}

void draw_update(void) {
    // Check for VALID press (during draw)
    if (button_was_pressed()) {
        calculate_reaction_time();
        game_transition_to(STATE_RESULT);  // Success!
        return;
    }

    // Check for timeout
    if (micros() - draw_start_time >= REACTION_MAX_US) {
        game_transition_to(STATE_RESULT);
    }
}
```

**Key Insight:** Same flag (`button_was_pressed()`), different outcomes based on game state.

### False Start State

New game state added:

```cpp
typedef enum {
    STATE_ATTRACT,
    STATE_READY,
    STATE_DRAW,
    STATE_RESULT,
    STATE_FALSE_START,  // NEW: Penalty for pressing too early
    NUM_STATES
} GameState;
```

## Implementation Checklist

### Files to Modify from Tutorial 03

- [ ] `include/game.h` - Add `STATE_FALSE_START` to enum
- [ ] `include/hardware.h` - Add `display_show_false_start()`
- [ ] `src/game.cpp` - Add false start state handlers, check flag in ready_update
- [ ] `src/hardware.cpp` - Add false start display function, error sound

### Changes to `game.h`

```cpp
typedef enum {
    STATE_ATTRACT,
    STATE_READY,
    STATE_DRAW,
    STATE_RESULT,
    STATE_FALSE_START,  // NEW
    NUM_STATES
} GameState;
```

### Changes to `hardware.h` and `hardware.cpp`

```cpp
// In hardware.h
void display_show_false_start(void);
void buzzer_error(void);  // Already existed, but now used more

// In hardware.cpp
void display_show_false_start(void) {
    display_show("False Start!", "Wait for LED!");
}

void buzzer_error(void) {
    // Descending tones for false start
    tone(BUZZER_PIN, 400, 100);
    delay(120);
    tone(BUZZER_PIN, 200, 200);
}
```

### Changes to `game.cpp`

#### Add False Start State Handlers

```cpp
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
```

#### Modify ready_update to Check for Early Press

```cpp
static void ready_update(void) {
    // NEW: Check for false start (pressed before LED)
    if (button_was_pressed()) {
        game_transition_to(STATE_FALSE_START);
        return;
    }

    // Check if ready period expired
    if (millis() - state_entry_time >= ready_duration) {
        game_transition_to(STATE_DRAW);
    }
}
```

#### Update State Handler Table

```cpp
static const StateHandler state_handlers[NUM_STATES] = {
    [STATE_ATTRACT]     = { attract_enter,     attract_update,     attract_exit     },
    [STATE_READY]       = { ready_enter,       ready_update,       ready_exit       },
    [STATE_DRAW]        = { draw_enter,        draw_update,        draw_exit        },
    [STATE_RESULT]      = { result_enter,      result_update,      result_exit      },
    [STATE_FALSE_START] = { false_start_enter, false_start_update, false_start_exit },  // NEW
};
```

### Add to `config.h`

```cpp
// Display durations
#define FALSE_START_DISPLAY_MS 2000  // Show false start message for 2 seconds
```

## Expected Behavior

### Valid Gameplay Flow

```
1. STATE_ATTRACT → Press button → STATE_READY
2. STATE_READY → Wait 2-4 seconds → STATE_DRAW
3. STATE_DRAW → Press button → STATE_RESULT
4. STATE_RESULT → Automatic → STATE_READY (next round)
```

### False Start Flow

```
1. STATE_ATTRACT → Press button → STATE_READY
2. STATE_READY → Press button too early → STATE_FALSE_START
   LCD: "False Start! / Wait for LED!"
   Buzzer: Descending error tones
3. STATE_FALSE_START → Wait 2 seconds → STATE_READY (retry same round)
```

### What Students Should See

1. **Normal gameplay works** - Press during LED = valid
2. **Early press detected** - Press during countdown = false start
3. **Penalty enforced** - 2 second delay, must retry round
4. **Round doesn't count** - False start doesn't increment round counter

## Teaching Moments

### Why Check in Multiple States?

**Question:** "Why do both `ready_update()` and `draw_update()` check the button?"

**Answer:**
Because interrupts fire regardless of state. We need to:
- **STATE_READY:** Detect invalid presses → false start
- **STATE_DRAW:** Detect valid presses → calculate reaction time

**Analogy:**
"The doorbell rings (interrupt) at any time. You still need to decide: Am I expecting a guest? (valid) or is this a wrong address? (invalid)"

### Interrupt State Management

**Question:** "Should we disable the interrupt during STATE_READY?"

**Bad Approach (Don't Do This):**
```cpp
void ready_enter(void) {
    detachInterrupt(digitalPinToInterrupt(BUTTON_PIN));  // Disable
}

void draw_enter(void) {
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_isr, FALLING);  // Re-enable
}
```

**Why it's bad:**
- Complex (easy to forget to re-enable)
- Misses learning opportunity (defensive checking)
- Not flexible (can't detect false starts)

**Good Approach (This Tutorial):**
```cpp
void ready_update(void) {
    if (button_was_pressed()) {  // Interrupt stays enabled, we just check
        game_transition_to(STATE_FALSE_START);
    }
}
```

**Why it's good:**
- Simple (interrupt always enabled)
- Flexible (can handle press differently per state)
- Defensive (catches unexpected behavior)

### State Validation Pattern Preview

This tutorial introduces state-based validation:

```
ISR fires → Sets flag → Main loop checks flag in current state → Decides action
```

Tutorial 05 extends this to **capture state in ISR** for even more robust validation.

## Testing Checklist

- [ ] Normal gameplay: Press during LED → valid reaction time recorded
- [ ] False start: Press during countdown → "False Start!" message
- [ ] Penalty: False start shows for 2 seconds
- [ ] Retry: After false start, same round retries (counter doesn't increment)
- [ ] Sound: False start plays descending error tones
- [ ] Multiple false starts: Can trigger multiple times if player keeps pressing early

### Edge Cases to Test

- [ ] Press immediately after "Get Ready..." appears → False start
- [ ] Press 1 second into countdown → False start
- [ ] Press right before LED turns on (timing attack) → False start
- [ ] Press right after LED turns on → Valid (no false start)

## State Diagram

```
        ┌─────────────┐
        │   ATTRACT   │
        └──────┬──────┘
               │ Press button
               ↓
        ┌─────────────┐
    ┌──→│    READY    │◄──┐
    │   └──────┬──────┘   │
    │          │ Timer    │ Retry
    │   Press  │ expires  │ same
    │   early  ↓          │ round
    │   ┌─────────────┐   │
    └───│FALSE_START  │───┘
        └─────────────┘

        ┌─────────────┐
        │    DRAW     │
        └──────┬──────┘
               │ Press button
               ↓
        ┌─────────────┐
        │   RESULT    │
        └──────┬──────┘
               │ Next round
               ↓
```

## Key Differences from Tutorial 03

| Aspect | Tutorial 03 | Tutorial 04 |
|--------|-------------|-------------|
| **States** | 4 states | 5 states (+ FALSE_START) |
| **ready_update** | Just waits for timer | Also checks for early press |
| **Interrupt Usage** | Only meaningful in DRAW | Meaningful in READY and DRAW |
| **Penalty** | None | 2 second delay + retry |

## Transition to Tutorial 05

**The Hook:** "What if we add distractor LEDs during the countdown? How do we know if the player pressed during a distractor flash vs the real LED?"

**Preview:** Tutorial 05 adds distractor LED flashes during STATE_READY. The ISR will capture the game state at the moment of the button press, allowing the main loop to validate: "Were we in STATE_DRAW or somewhere else?"

## Key Takeaways

✅ Interrupts fire in ANY state - design defensively
✅ Check button flags in multiple state handlers based on context
✅ Add penalty states to enforce game rules
✅ Interrupt remains enabled - disable/re-enable is complex and fragile
✅ State-based validation: Same event, different outcomes per state
✅ Retry logic: Decrement round counter to replay round after penalty

## Common Student Questions

**Q: Why not just disable the interrupt during STATE_READY?**
A: Works, but fragile. Easier to keep interrupt always on and check context. Also, we want to detect false starts!

**Q: What if someone presses during STATE_RESULT?**
A: Currently ignored (result_update doesn't check flag). Could add "too eager" detection if desired.

**Q: Can you false-start multiple times in a row?**
A: Yes! If you keep pressing during countdown, you'll keep getting penalties. Round won't advance until you wait for the LED.

## File Status

- ✅ `platformio.ini` - No changes from Tutorial 03
- ✅ `include/config.h` - Modified (add FALSE_START_DISPLAY_MS)
- ✅ `include/hardware.h` - Modified (add display_show_false_start)
- ✅ `include/game.h` - Modified (add STATE_FALSE_START)
- ✅ `src/hardware.cpp` - Modified (implement false start display)
- ✅ `src/game.cpp` - Modified (false start state, ready_update check)
- ✅ `src/main.cpp` - No changes from Tutorial 03

**Ready to build and enforce game rules with interrupts!**
