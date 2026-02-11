# Tutorial 03: Debounced Interrupt

## Purpose

Fix the button bounce problem from Tutorial 02 by adding **software debouncing** to the ISR. This teaches a fundamental interrupt technique: using timing logic to filter hardware imperfections.

## Learning Objectives

- Understand button bounce at microsecond timescales
- Implement software debouncing in an ISR
- Learn the 50ms lockout pattern
- Recognize when to handle problems in ISR vs main loop
- Document ISR constraints with comprehensive comments

## Key Concepts

### Button Bounce Explained

**Physical Reality:**
```
Button press attempt:
Time (µs):  0     5000   10000  15000  20000  25000  30000
State:      HIGH  LOW    HIGH   LOW    HIGH   LOW    LOW (stable)
            └─┘   └──┘   └──┘   └──┘   └──┘   └──────────
            Contact bounces ~5-10 times in first 25ms
```

**Without debouncing:**
- Each bounce triggers an interrupt
- ISR runs 5-10 times from one press
- Game registers multiple button presses

**With debouncing:**
- First bounce triggers ISR, captures timestamp
- Subsequent bounces ignored for 50ms
- Game registers exactly one button press

### The 50ms Lockout Pattern

```cpp
volatile uint32_t last_interrupt_time_us = 0;
const uint32_t DEBOUNCE_US = 50000;  // 50ms = 50,000µs

void button_isr(void) {
    uint32_t current_time = micros();

    // Reject interrupts within 50ms of last one
    if ((current_time - last_interrupt_time_us) > DEBOUNCE_US) {
        last_interrupt_time_us = current_time;

        // Process interrupt (capture data, set flag)
        button_press_time_us = current_time;
        button_pressed_flag = true;
    }
    // Bounces are silently ignored
}
```

**Why 50ms?**
- Most buttons stop bouncing within 5-20ms
- 50ms provides safety margin
- Short enough humans can't press twice in 50ms
- Long enough to filter all mechanical bounce

### ISR Constraint Documentation

This tutorial adds extensive comments explaining ISR constraints:

```cpp
// ============================================================================
// Button Interrupt Service Routine (ISR)
// ============================================================================
// CRITICAL ISR CONSTRAINTS:
// 1. Keep execution time < 10µs (ideally < 5µs)
// 2. NO delay(), Serial.print(), lcd.print(), or any blocking calls
// 3. NO complex logic - just capture data and set flags
// 4. Only access volatile variables
// 5. Return immediately
//
// ISR PATTERN:
// - ISR captures moment-in-time data (timestamp, flags)
// - Sets flag to signal main loop
// - Main loop processes the event with full context
//
// DEBOUNCING:
// - Physical buttons bounce (make/break contact) for 5-50ms
// - At microsecond precision, each bounce triggers an interrupt
// - Software debouncing: Ignore interrupts within 50ms of last one
// ============================================================================

void button_isr(void) {
    uint32_t current_time = micros();

    // Software debouncing: Ignore bounces within 50ms
    if ((current_time - last_interrupt_time_us) > DEBOUNCE_US) {
        last_interrupt_time_us = current_time;
        button_press_time_us = current_time;
        button_pressed_flag = true;
    }
    // Bounces are silently ignored
}
```

## Implementation Checklist

### Files to Modify from Tutorial 02

- [ ] `include/config.h` - Add `DEBOUNCE_US` constant
- [ ] `src/hardware.cpp` - Add debounce logic to ISR, add extensive comments

### Changes to `config.h`

```cpp
// Add debounce timing constant
#define DEBOUNCE_US 50000  // 50ms debounce in microseconds
```

### Changes to `hardware.cpp`

#### Add Debounce Variable

```cpp
// ISR-accessible variables (MUST be volatile)
volatile bool button_pressed_flag = false;
volatile uint32_t button_press_time_us = 0;
volatile uint32_t last_interrupt_time_us = 0;  // NEW: For debouncing
```

#### Update ISR with Debouncing

```cpp
void button_isr(void) {
    uint32_t current_time = micros();

    // Software debouncing: Ignore interrupts within 50ms of last one
    if ((current_time - last_interrupt_time_us) > DEBOUNCE_US) {
        last_interrupt_time_us = current_time;
        button_press_time_us = current_time;
        button_pressed_flag = true;
    }
    // Bounces silently ignored
}
```

#### Update Clear Function

```cpp
void button_clear_state(void) {
    button_pressed_flag = false;
    button_press_time_us = 0;
    last_interrupt_time_us = 0;  // NEW: Reset debounce timer
}
```

## Expected Behavior

### What Students Should See

1. **Single press = single detection** - Bounce problem solved!
2. **Microsecond precision maintained** - Still shows 237.423ms accuracy
3. **Reliable gameplay** - Game progresses one round per button press
4. **Fast response** - No perceptible delay (50ms is imperceptible to humans)

### Before/After Comparison

**Tutorial 02 (No Debouncing):**
```
Press button once:
  Round 1: 234ms
  Round 2: 0ms      ← Bounce
  Round 3: 0ms      ← Bounce
  Round 4: 1ms      ← Bounce
  Round 5: 0ms      ← Bounce
  Game Over!
```

**Tutorial 03 (With Debouncing):**
```
Press button once:
  Round 1: 234ms
  (Ready for round 2...)

Press button again:
  Round 2: 287ms
  (Ready for round 3...)
```

## Teaching Moments

### Understanding Debounce Timing

**Question:** "Why 50ms? Why not 5ms or 500ms?"

**Answer:**
- **Too short (5ms):** Might not catch all bounces (some buttons bounce for 20ms)
- **Too long (500ms):** Humans could press twice and get ignored
- **50ms:** Sweet spot - catches all bounces, humans can't press that fast

### ISR vs Main Loop Debouncing

**Question:** "Could we debounce in the main loop instead of the ISR?"

**Answer:**
```cpp
// Alternative: Debounce in main loop (NOT RECOMMENDED for interrupts)
void draw_update(void) {
    if (button_was_pressed()) {
        uint32_t current_time = micros();
        if ((current_time - last_press_time) > DEBOUNCE_US) {
            last_press_time = current_time;
            // Process press...
        }
    }
}
```

**Problem:** ISR still fires 10 times, filling queue with events. Better to reject at source.

**Conclusion:** For interrupts, debounce in ISR to prevent wasted processing.

### Demonstrating the Fix

**Live Demo:**
1. Load Tutorial 02 (no debouncing)
2. Press button once → observe multiple rounds
3. Load Tutorial 03 (with debouncing)
4. Press button once → observe single round
5. Hold button down → only one press registered (lockout working)

## Debounce Timing Tuning

Students can experiment with `DEBOUNCE_US`:

```cpp
// Too aggressive (5ms) - May still see bounces
#define DEBOUNCE_US 5000

// Conservative (100ms) - Feels sluggish for rapid presses
#define DEBOUNCE_US 100000

// Goldilocks (50ms) - Just right for most buttons
#define DEBOUNCE_US 50000
```

**Exercise:** Change to 5000µs, observe occasional bounce. Change to 100000µs, observe sluggish response to rapid presses.

## Testing Checklist

- [ ] Upload to Arduino Uno
- [ ] Single button press → single round (no bounces)
- [ ] Rapid button presses → each press detected (no over-filtering)
- [ ] Hold button down → only one press registered
- [ ] Microsecond precision still working
- [ ] Game plays smoothly through all 5 rounds

## Key Differences from Tutorial 02

| Aspect | Tutorial 02 | Tutorial 03 |
|--------|------------|-------------|
| **Bounce Handling** | None (bug!) | 50ms software debounce |
| **ISR Complexity** | 2 lines | 6 lines (still < 10µs) |
| **Reliability** | 5-10 triggers per press | 1 trigger per press |
| **Comments** | Minimal | Extensive ISR docs |

## Transition to Tutorial 04

**The Hook:** "Great! Our interrupt is now reliable. But what if the button is pressed at the wrong time? Let's add false start detection..."

**Preview:** Tutorial 04 keeps the interrupt enabled during the countdown phase (STATE_READY) and detects early presses as false starts.

## Key Takeaways

✅ Software debouncing is essential for reliable interrupt-based button input
✅ 50ms lockout pattern filters mechanical bounce
✅ Debounce in ISR to reject spurious events at the source
✅ ISR can contain simple timing logic and still execute in <10µs
✅ `volatile uint32_t` for timestamp variables accessed by ISR
✅ Comprehensive comments in ISR explain constraints and patterns

## Common Student Questions

**Q: Why not use hardware debouncing (capacitor)?**
A: You could! 100nF capacitor between button and ground helps. But software debouncing is free, reliable, and works with any button.

**Q: What if `micros()` overflows (wraps around every 70 minutes)?**
A: The subtraction `(current - last) > DEBOUNCE_US` still works correctly due to unsigned integer overflow behaviour. Try it: `(5 - 4294967295) > 50000` evaluates correctly even after overflow.

**Q: Is 50ms fast enough for games?**
A: Absolutely! Human reaction time is ~200-300ms. A 50ms debounce delay is imperceptible.

## File Status

- ✅ `platformio.ini` - No changes from Tutorial 02
- ✅ `include/config.h` - Modified (add DEBOUNCE_US)
- ✅ `include/hardware.h` - No changes from Tutorial 02
- ✅ `include/game.h` - No changes from Tutorial 02
- ✅ `src/hardware.cpp` - Modified (debounce logic, comments)
- ✅ `src/game.cpp` - No changes from Tutorial 02
- ✅ `src/main.cpp` - No changes from Tutorial 02

**Ready to build and enjoy reliable interrupt-based input!**
