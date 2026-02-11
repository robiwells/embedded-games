# Tutorial 02: Simple Interrupt

## Purpose

Migrate from polling to **hardware interrupts**, demonstrating microsecond-precision timing and immediate response. This tutorial intentionally **does not include debouncing** to show the bounce problem that Tutorial 03 solves.

## Learning Objectives

- Set up hardware interrupt with `attachInterrupt()`
- Write a basic ISR (Interrupt Service Routine)
- Understand the `volatile` keyword
- Use `micros()` for microsecond precision
- **Experience button bounce** (multiple triggers from single press)

## Key Concepts

### Hardware Interrupt Setup

```cpp
void button_init_interrupt(void) {
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_isr, FALLING);
}
```

**Parameters:**
- `digitalPinToInterrupt(2)` → Converts pin 2 to INT0 interrupt number
- `button_isr` → Function to call when interrupt triggers
- `FALLING` → Trigger on HIGH→LOW transition (button press)

### Basic ISR Pattern

```cpp
volatile bool button_pressed_flag = false;
volatile uint32_t button_press_time_us = 0;

void button_isr(void) {
    button_press_time_us = micros();  // Capture timestamp
    button_pressed_flag = true;        // Signal main loop
}
```

**ISR Rules:**
1. Keep execution time < 10µs
2. No `delay()`, `Serial.print()`, `lcd.print()`
3. Just capture data and set flags
4. Return immediately

### The `volatile` Keyword

**Why it's required:**

```cpp
// WITHOUT volatile - Compiler bug!
bool button_flag = false;

void loop() {
    while (!button_flag) {
        // Compiler: "button_flag never changes in this loop!"
        // Optimises to infinite loop
    }
}

// WITH volatile - Correct behavior
volatile bool button_flag = false;

void loop() {
    while (!button_flag) {
        // Compiler: "Must check every iteration, could change anytime!"
        // Correctly exits when ISR sets flag
    }
}
```

**Rule:** Any variable accessed by both ISR and main code **MUST** be `volatile`.

### Microsecond Timing

```cpp
draw_start_time = micros();      // Record start (microseconds)
// ... interrupt fires ...
reaction_us = button_press_time_us - draw_start_time;  // µs precision
reaction_ms = reaction_us / 1000;  // Convert to ms for display
```

**Precision:** `micros()` has ~4µs resolution on Arduino Uno (vs 1000µs for `millis()`)

## Hardware Configuration Changes

### Pin Migration

**Tutorial 01 (Polling):**
- Button on pin 10 (no interrupt capability)
- 7 LEDs (pins 3-9)

**Tutorial 02 (Interrupt):**
- **Button on pin 2** (INT0 interrupt capable)
- **8 LEDs** (pins 3-10, pin 10 now free)

**Why pin 2?** Arduino Uno only has 2 external interrupt pins:
- Pin 2 → INT0
- Pin 3 → INT1

## Implementation Checklist

### Files to Modify from Tutorial 01

- [ ] `include/config.h` - Change `BUTTON_PIN` from 10 to 2
- [ ] `include/hardware.h` - Replace `button_just_pressed()` with interrupt API
- [ ] `src/hardware.cpp` - Implement ISR and interrupt setup
- [ ] `src/game.cpp` - Use `button_was_pressed()` flag checking
- [ ] `src/main.cpp` - Call `button_init_interrupt()` in setup

### New Code in `hardware.h`

```cpp
// Interrupt-based API
void button_init_interrupt(void);           // Setup interrupt
bool button_was_pressed(void);              // Check flag
uint32_t button_get_press_time(void);       // Get timestamp
void button_clear_state(void);              // Reset state
```

### New Code in `hardware.cpp`

```cpp
// ISR-accessible variables (MUST be volatile)
volatile bool button_pressed_flag = false;
volatile uint32_t button_press_time_us = 0;

// ISR - Called automatically by hardware when button pressed
void button_isr(void) {
    button_press_time_us = micros();  // Capture exact moment
    button_pressed_flag = true;        // Signal main loop
}

void button_init_interrupt(void) {
    // Attach ISR to INT0 (pin 2), trigger on FALLING edge
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_isr, FALLING);
}

bool button_was_pressed(void) {
    if (button_pressed_flag) {
        button_pressed_flag = false;  // Clear flag
        return true;
    }
    return false;
}
```

### Modified Code in `game.cpp`

```cpp
static void draw_enter(void) {
    led_set(4);
    button_clear_state();
    draw_start_time = micros();  // Changed from millis()
}

static void draw_update(void) {
    // Changed from button_just_pressed() to button_was_pressed()
    if (button_was_pressed()) {
        uint32_t press_time = button_get_press_time();
        uint32_t reaction_us = press_time - draw_start_time;

        // Store reaction time (convert to ms for compatibility)
        reaction_time_ms = reaction_us / 1000;

        game_transition_to(STATE_RESULT);
        return;
    }

    // Timeout check (still using microseconds)
    if (micros() - draw_start_time >= REACTION_MAX_US) {
        game_transition_to(STATE_RESULT);
    }
}
```

### Modified Code in `main.cpp`

```cpp
void setup() {
    wdt_disable();
    hardware_init();

    // NEW: Initialise interrupt after hardware
    button_init_interrupt();

    game_init();
    wdt_enable(WDTO_4S);
}
```

## Expected Behavior

### What Students Should See

1. **Microsecond precision** - Times like "237.423ms" instead of "237ms"
2. **Immediate response** - ISR fires instantly, no waiting for next loop iteration
3. **⚠️ BOUNCE BUG** - Single button press registers 5-10 times!

### The Bounce Problem (INTENTIONAL!)

**What happens:**
```
User presses button once
Hardware bounces for ~5-50ms
ISR fires on each bounce
Result: Multiple detections from single press!
```

**Console output example:**
```
Round 1: 234ms
Round 2: 0ms      ← Bounce!
Round 3: 0ms      ← Bounce!
Round 4: 237ms
Round 5: 1ms      ← Bounce!
```

**Why this is good pedagogically:**
- Shows that interrupts introduce new challenges
- Motivates Tutorial 03 (debouncing)
- Teaches that "faster" isn't always "better" without proper handling

## Teaching Moment

### Before Bounce Discovery

**Question:** "Run the game. What do you notice about the precision?"
**Answer:** Times now show microsecond precision (237.423ms vs 237ms)

**Question:** "What happens when you press the button once?"
**Answer:** Multiple rounds complete from one press!

### After Bounce Discovery

**Explanation:**
"Physical buttons don't make clean electrical contact. When you press, the metal contacts bounce together and apart dozens of times in the first 5-50ms. At microsecond precision, we can now 'see' each bounce as a separate interrupt!"

**Solution Preview:**
"Tutorial 03 adds software debouncing in the ISR to ignore bounces within 50ms of each other."

## Testing Checklist

- [ ] Upload to Arduino Uno
- [ ] Verify button is on pin 2 (not pin 10)
- [ ] Observe microsecond precision in reaction times
- [ ] Confirm single button press triggers multiple rounds (bounce bug)
- [ ] Check ISR fires instantly (LED should turn off immediately when pressed)

## Key Differences from Tutorial 01

| Aspect | Tutorial 01 (Polling) | Tutorial 02 (Interrupt) |
|--------|----------------------|------------------------|
| **Button Pin** | Pin 10 | Pin 2 (INT0) |
| **Detection** | `button_just_pressed()` | `button_was_pressed()` |
| **Timing** | `millis()` (1ms) | `micros()` (4µs) |
| **Precision** | ~20ms jitter | <1µs jitter |
| **Response** | Next loop iteration | Immediate (hardware) |
| **Bounce** | Hidden by slow polling | Visible problem |

## Transition to Tutorial 03

**The Hook:** "Great! We have microsecond precision now. But we also have a new problem - button bounce. How do we fix this without going back to slow polling?"

**Preview:** Tutorial 03 adds a simple debounce check in the ISR: ignore interrupts within 50ms of the last one.

## Key Takeaways

✅ Interrupts provide microsecond-precision event capture
✅ ISRs must be fast and simple (just capture data, set flags)
✅ `volatile` keyword is critical for ISR-main communication
✅ `attachInterrupt()` configures hardware to call ISR automatically
⚠️ Button bounce is a real problem at microsecond timescales
⚠️ Faster isn't always better without proper handling

## File Status

- ✅ `platformio.ini` - Copied from Tutorial 01
- ✅ `include/config.h` - Modified (BUTTON_PIN = 2)
- ✅ `include/hardware.h` - Modified (interrupt API)
- ✅ `include/game.h` - No changes needed
- ✅ `src/hardware.cpp` - Modified (ISR implementation)
- ✅ `src/game.cpp` - Modified (micros(), flag checking)
- ✅ `src/main.cpp` - Modified (button_init_interrupt call)

**Ready to build and experience the bounce problem!**
