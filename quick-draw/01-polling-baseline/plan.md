# Tutorial 01: Polling Baseline

## Purpose

Establish a baseline reaction-time game using **polling** to demonstrate its limitations. This provides a clear "before" state that makes the advantages of interrupts (Tutorial 02+) obvious.

## Learning Objectives

- Understand button polling with edge detection
- See the ~20ms precision limitation of polling
- Experience timing jitter in measurements
- Recognize when polling is "good enough" vs when interrupts are needed

## Key Concepts

### Button Polling Pattern

```cpp
static bool last_button_state = !BUTTON_ACTIVE;

bool button_just_pressed(void) {
    bool current_state = (digitalRead(BUTTON_PIN) == BUTTON_ACTIVE);
    bool just_pressed = (current_state && !last_button_state);
    last_button_state = current_state;
    return just_pressed;
}
```

**How it works:**
1. Read current button state
2. Compare with previous state (edge detection)
3. Return `true` only on transition from released → pressed
4. Update state for next call

### Timing with `millis()`

```cpp
draw_start_time = millis();  // Record when LED turns on
// ... wait for button press ...
reaction_time = millis() - draw_start_time;  // Calculate reaction (ms precision)
```

**Limitation:** `millis()` has 1ms resolution, but polling happens ~every 20ms depending on loop speed.

## Hardware Configuration

- **Button on pin 10** (NO interrupt capability - this is intentional!)
- **7 LEDs** on pins 3-9 (pin 10 used for button)
- **Buzzer** on pin 11
- **I2C LCD** on A4/A5

## Game States

Simple 4-state machine:

```
STATE_ATTRACT  → Wait for button press to start
STATE_READY    → Random countdown (2-4 seconds)
STATE_DRAW     → LED on, waiting for button press
STATE_RESULT   → Show reaction time
```

## Implementation Checklist

### Files to Create

- [ ] `platformio.ini` - Arduino Uno configuration
- [ ] `include/config.h` - Pin definitions (BUTTON_PIN = 10)
- [ ] `include/hardware.h` - Polling API: `button_just_pressed()`
- [ ] `include/game.h` - State machine interface
- [ ] `src/hardware.cpp` - Polling implementation
- [ ] `src/game.cpp` - 4 state handlers (attract/ready/draw/result)
- [ ] `src/main.cpp` - Entry point with watchdog

### Key Code Sections

**In `hardware.cpp`:**
```cpp
// Polling-based edge detection
static bool last_button_state = !BUTTON_ACTIVE;

bool button_just_pressed(void) {
    bool current = (digitalRead(BUTTON_PIN) == BUTTON_ACTIVE);
    bool just_pressed = (current && !last_button_state);
    last_button_state = current;
    return just_pressed;
}
```

**In `game.cpp` (draw_update):**
```cpp
static void draw_update(void) {
    // Poll button every loop iteration (~20ms)
    if (button_just_pressed()) {
        button_press_time = millis();
        game_transition_to(STATE_RESULT);
        return;
    }

    // Timeout check
    if (millis() - draw_start_time >= REACTION_MAX_MS) {
        game_transition_to(STATE_RESULT);
    }
}
```

## Expected Behavior

### What Students Should See

1. **Game works fine** - Polling is fast enough for human reaction times
2. **Visible jitter** - Reaction times show ~20ms variance (237ms, 241ms, 219ms from same reaction)
3. **Millisecond precision** - Times displayed as whole milliseconds (no decimal places)

### Teaching Moment

**Question to ask:** "The game works, but is 20ms precision good enough for all applications?"

**Examples where polling fails:**
- Rotary encoder (misses pulses at high speed)
- Communication protocols (UART/SPI/I2C timing critical)
- Precise event timestamping (scientific measurements)

**Examples where polling works:**
- UI buttons (humans can't press faster than 100ms)
- Slow sensors (temperature, light level)
- Simple games (like this one!)

## Testing

1. Upload to Arduino Uno
2. Play multiple rounds
3. Note reaction times vary by ~20ms for similar reactions
4. Observe that times are whole milliseconds only
5. Game feels responsive enough (polling isn't "broken")

## Transition to Tutorial 02

**The Hook:** "Polling works for this game, but what if we needed microsecond precision? What if we had other tasks to do in the loop and couldn't check the button every 20ms? Let's try hardware interrupts..."

**Preview:** Next tutorial moves button to pin 2 (INT0), uses `micros()` for timing, and implements a simple ISR. Students will immediately see microsecond precision and then encounter the bounce problem that Tutorial 03 solves.

## Key Takeaways

✅ Polling is simple and works for many applications
✅ Precision is limited by loop speed (~20ms for simple loops)
✅ CPU must actively check - can't do other long-running tasks
✅ Good baseline for comparing interrupt-based approach

## File Status

- ✅ `platformio.ini` - Created
- ✅ `include/config.h` - Created
- ✅ `include/hardware.h` - Created
- ✅ `include/game.h` - Created
- ✅ `src/hardware.cpp` - Created
- ✅ `src/game.cpp` - Created
- ✅ `src/main.cpp` - Created

**Ready to build and upload!**
