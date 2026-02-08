# Tutorial 4: Non-Blocking Chase

## What You'll Learn
- **The most critical embedded programming pattern:** Non-blocking timing
- Using millis() for timestamp-based timing
- Elapsed time calculations
- Why delay() is forbidden in production code
- The responsive program architecture

## Prerequisites
- Tutorial 3: LED Chase Basic (understanding chase logic, arrays, bounce)

## Hardware Components
- Arduino Uno
- 8× LEDs (pins 2-9)
- 8× 220Ω resistors

## Code Overview
This tutorial takes the exact same chase animation from Tutorial 3 and **removes the delay()**. The LED bounces identically, but now the program **remains responsive** during timing delays.

**The transformation:**
- Tutorial 3: Uses `delay(200)` - blocks for 200ms every loop iteration
- Tutorial 4: Uses `millis()` timestamps - checks time, returns immediately

Visual output is **identical**. Internal behaviour is **completely different**.

## File Structure
```
04-nonblocking-chase/
├── diagram.json          # Wokwi circuit (same as Tutorial 3)
├── platformio.ini        # Build configuration
├── wokwi.toml           # Wokwi simulator settings
├── include/
│   └── config.h         # Hardware constants
└── src/
    └── main.cpp         # Non-blocking chase logic
```

## The Non-Blocking Problem

### Why delay() Is Problematic

**With delay() (Tutorial 3):**
```cpp
void loop() {
    update_led_position();
    delay(200);  // ❌ STOPS HERE for 200ms
                 // Can't do ANYTHING during this time
}
```

**Timeline:**
```
0ms     200ms   400ms   600ms   800ms   1000ms
 ├───────┤───────┤───────┤───────┤────────┤
 │ LED1  │delay  │ LED2  │delay  │ LED3   │delay
 │update │200ms  │update │200ms  │update  │200ms
 └───────┘       └───────┘       └────────┘
         ↑               ↑                ↑
    BLOCKED        BLOCKED          BLOCKED
    (No button checks, no other work possible)
```

**What you CAN'T do during delay():**
- ❌ Check button presses (user input ignored)
- ❌ Update other LEDs or animations
- ❌ Read sensors
- ❌ Communicate over serial/I2C/SPI
- ❌ Reset watchdog timer (system will reset!)
- ❌ Update display
- ❌ Literally anything except wait

**With non-blocking timing (Tutorial 4):**
```cpp
void loop() {
    if (enough_time_passed()) {
        update_led_position();  // ✅ Only updates when needed
    }
    // Returns immediately to loop, can do other work
}
```

**Timeline:**
```
0ms    1ms    2ms    3ms    ...   200ms  201ms  202ms
 ├──────┼──────┼──────┼──────┼──────┼──────┼──────┼
 │loop  │loop  │loop  │loop  │loop  │loop  │loop  │loop
 │      │      │      │      │      │LED   │      │
 │      │      │      │      │      │update│      │
 └──────┴──────┴──────┴──────┴──────┴──────┴──────┴
        ↑      ↑      ↑            ↑      ↑      ↑
    RESPONSIVE - Can check buttons, sensors, update display
```

Each loop iteration takes ~0.1-1ms. The program checks "is it time to update?" 200-1000 times during that 200ms delay period. Always responsive!

## Key Concepts

### millis() - The Arduino Clock

```cpp
uint32_t millis();
```

**What it returns:**
Number of milliseconds since Arduino powered on or reset.

**Return type:** uint32_t (32-bit unsigned integer)
- Range: 0 to 4,294,967,295
- Overflows after ~49.7 days (2^32 milliseconds)
- For games running minutes/hours, overflow isn't a concern

**How it works:**
Arduino's hardware timer increments a counter every millisecond using a hardware interrupt. `millis()` just reads this counter. It's:
- **Fast** (~4 CPU cycles)
- **Non-blocking** (returns immediately)
- **Always available** (updated by hardware interrupt)
- **Accurate** (1ms resolution)

**Example values:**
```cpp
// At boot:
millis() → 0

// After 1 second:
millis() → 1000

// After 30 seconds:
millis() → 30000

// After 5 minutes:
millis() → 300000
```

### Timestamp-Based Timing Pattern

The fundamental non-blocking pattern used in embedded systems:

```cpp
static uint32_t last_update = 0;  // When we last did the action
uint32_t now = millis();           // Current time

if (now - last_update >= interval) {
    // Enough time has passed, do the action
    do_something();
    last_update = now;  // Record new timestamp
}
// Always returns quickly (no blocking!)
```

**How it works:**

1. **Record timestamp:** Store `millis()` when action occurs
2. **Check elapsed time:** Subtract old timestamp from current time
3. **Compare to interval:** If `elapsed >= interval`, time to act
4. **Update timestamp:** Record new timestamp for next interval
5. **Return immediately:** Whether action happened or not, function returns

**Example trace (200ms interval):**

```
Time  │ now   │ last_update │ elapsed │ Action?
──────┼───────┼─────────────┼─────────┼────────────────
Boot  │   0   │      0      │    0    │ Initialisation
0ms   │   0   │      0      │    0    │ First action, set last_update = 0
50ms  │  50   │      0      │   50    │ No (50 < 200)
100ms │ 100   │      0      │  100    │ No (100 < 200)
150ms │ 150   │      0      │  150    │ No (150 < 200)
200ms │ 200   │      0      │  200    │ YES! Do action, set last_update = 200
250ms │ 250   │    200      │   50    │ No (50 < 200)
300ms │ 300   │    200      │  100    │ No (100 < 200)
400ms │ 400   │    200      │  200    │ YES! Do action, set last_update = 400
```

### Elapsed Time Calculation

```cpp
uint32_t elapsed = now - last_update;
```

**Why subtraction works:**

Unsigned integer subtraction naturally handles wraparound. Even if `millis()` overflows (rare), the math still works:

**Normal case (no overflow):**
```
now = 5000
last_update = 4800
elapsed = 5000 - 4800 = 200 ✅
```

**Overflow case (millis wraps from max to 0):**
```
last_update = 4,294,967,200  (100ms before overflow)
now = 100                     (100ms after overflow, millis() wrapped)
elapsed = 100 - 4,294,967,200 = -4,294,967,100 (as signed)
                              = 200 (as unsigned, due to wraparound) ✅
```

The unsigned arithmetic handles wraparound correctly! This is why we use uint32_t for timestamps.

### Why uint32_t for Timestamps?

```cpp
uint32_t now = millis();       // ✅ Correct
uint16_t now = millis();       // ❌ Truncates! Overflows every 65 seconds
uint8_t now = millis();        // ❌ Terrible! Overflows every 0.25 seconds
```

**uint32_t** (32-bit unsigned):
- Range: 0 to 4,294,967,295
- Milliseconds: ~49.7 days before overflow
- Perfect for millis() return value

**uint16_t** (16-bit unsigned):
- Range: 0 to 65,535
- Would overflow every 65.5 seconds
- Timing would break after 1 minute!

**uint8_t** (8-bit unsigned):
- Range: 0 to 255
- Would overflow every 255 milliseconds (0.255 seconds)
- Useless for timing

## Code Walkthrough

### New Global Variable: last_update

```cpp
uint32_t last_update = 0;  // Timestamp of last LED update
```

- **Type:** uint32_t (matches millis() return type)
- **Purpose:** Remember when we last moved the LED
- **Initialisation:** 0 at boot
- **Updates:** Set to current time after each LED movement

### setup() - Initialise Timestamp

```cpp
void setup() {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        pinMode(LED_PIN_START + i, OUTPUT);
        digitalWrite(LED_PIN_START + i, LOW);
    }
    last_update = millis();  // Sync timing to current time
}
```

**Why initialise last_update?**

Without initialisation:
- `last_update = 0` (default)
- If setup() takes 50ms to run, millis() is already 50 at first loop()
- `elapsed = 50 - 0 = 50` → first action happens immediately (not wrong, but inconsistent)

With initialisation:
- `last_update = millis()` (current time)
- First loop(): `elapsed = 50 - 50 = 0` → waits full interval before first action
- Consistent timing from the start

### loop() - Non-Blocking Update

```cpp
void loop() {
    uint32_t now = millis();  // [1] Get current time

    if (now - last_update >= CHASE_SPEED) {  // [2] Check if interval elapsed
        last_update = now;  // [3] Update timestamp

        // [4] Do the work (same as Tutorial 3)
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            digitalWrite(LED_PIN_START + i, LOW);
        }
        digitalWrite(LED_PIN_START + current_position, HIGH);

        current_position += direction;

        if (current_position == 0) {
            direction = 1;
        } else if (current_position == NUM_LEDS - 1) {
            direction = -1;
        }
    }
    // [5] Always returns quickly (whether action happened or not)
}
```

**Step-by-step breakdown:**

**[1] Get current time:**
```cpp
uint32_t now = millis();
```
Reads the current time once at the start of this loop iteration. We use `now` for all comparisons in this iteration (consistency).

**[2] Check if enough time passed:**
```cpp
if (now - last_update >= CHASE_SPEED) {
```
Calculate elapsed time since last update. If it's been at least `CHASE_SPEED` (200ms), it's time to move the LED.

**[3] Update timestamp:**
```cpp
last_update = now;
```
Record the current time as the new "last update". Next check will measure from this moment.

**[4] Do the work:**
The LED update code is **identical** to Tutorial 3. We just removed `delay(CHASE_SPEED)` and wrapped everything in a time check.

**[5] Return immediately:**
Whether the `if` condition was true or false, `loop()` returns immediately (in ~0.1-1ms). No blocking!

## Running the Code

### Using PlatformIO + Wokwi:
1. Open this folder in VS Code
2. Build: `pio run`
3. Upload to Wokwi simulator
4. Start simulation

### What You Should See:
**Visually:** Identical to Tutorial 3
- LED bounces left-to-right at 200ms per step
- Smooth, continuous animation
- No visible difference from blocking version

**Internally:** Completely different
- `loop()` executes 1000+ times per second
- LED only updates every 200ms (when timing check passes)
- Program remains responsive between updates

### How to Verify It's Non-Blocking?

Add a test LED that blinks rapidly:

```cpp
void loop() {
    uint32_t now = millis();

    // Original chase (200ms interval)
    if (now - last_update >= CHASE_SPEED) {
        // ... chase logic ...
    }

    // Rapid test blink (50ms interval)
    static uint32_t last_blink = 0;
    if (now - last_blink >= 50) {
        last_blink = now;
        static bool blink_state = false;
        blink_state = !blink_state;
        digitalWrite(13, blink_state);  // Built-in LED blinks rapidly
    }
}
```

With blocking delay(200), the test LED couldn't blink independently. With non-blocking timing, both animations run smoothly in parallel!

## Exercises

### Easy: Variable speed chase
Add code to speed up the chase over time:
- Start at 200ms per step
- Every 10 seconds, decrease interval by 10ms
- Minimum speed: 50ms per step

Hint: Add a second timer that checks every 10 seconds and decreases `CHASE_SPEED` (you'll need to make it a variable, not a const).

### Medium: Dual chase
Run two independent chase animations:
- Chase 1: Bounces at 200ms intervals
- Chase 2: Bounces at 300ms intervals (different speed!)
- Use different position and timing variables for each

This is **impossible** with delay() but trivial with non-blocking timing.

### Hard: Acceleration and deceleration
Make the LED speed up as it approaches the centre and slow down at the edges:
- Edges (positions 0, 7): 400ms delay
- Positions 1, 6: 300ms delay
- Positions 2, 5: 200ms delay
- Centre (positions 3, 4): 100ms delay (fastest)

Hint: Calculate interval based on `current_position`:
```cpp
uint16_t calculate_delay(uint8_t pos) {
    uint8_t distance_from_centre = abs(pos - 3.5);
    // Map distance to delay...
}
```

## Next Steps
- **Next tutorial:** [05-button-debounce](../05-button-debounce/) - Add button input with edge detection
- **New concepts:** INPUT_PULLUP, edge detection, debouncing, button state machines

## Common Pitfalls

### Forgetting to update last_update
**Wrong:**
```cpp
if (now - last_update >= CHASE_SPEED) {
    update_led();
    // ❌ Forgot to update last_update!
}
```

**What happens:**
- First check: `elapsed = 200`, passes, updates LED
- Next loop (1ms later): `elapsed = 201`, passes again, updates LED
- Next loop (1ms later): `elapsed = 202`, passes again, updates LED
- LED updates every millisecond (1000 times faster than intended!)

**Fix:**
```cpp
if (now - last_update >= CHASE_SPEED) {
    update_led();
    last_update = now;  // ✅ Critical! Reset timer
}
```

### Using wrong data type for timestamps
**Wrong:**
```cpp
uint16_t now = millis();  // ❌ Truncates! Breaks after 65 seconds
```

**What happens:**
```
millis() returns:  100000  (100 seconds = 100,000 milliseconds)
Truncated to uint16_t:  34464  (100000 % 65536)
Timing math becomes invalid
```

**Fix:**
```cpp
uint32_t now = millis();  // ✅ Matches millis() return type
```

### Calling millis() multiple times per iteration
**Inefficient:**
```cpp
void loop() {
    if (millis() - last_update1 >= 100) { /* ... */ }
    if (millis() - last_update2 >= 200) { /* ... */ }
    if (millis() - last_update3 >= 300) { /* ... */ }
    // Called millis() 3 times!
}
```

**Better:**
```cpp
void loop() {
    uint32_t now = millis();  // Call once, reuse
    if (now - last_update1 >= 100) { /* ... */ }
    if (now - last_update2 >= 200) { /* ... */ }
    if (now - last_update3 >= 300) { /* ... */ }
}
```

**Why it matters:**
- Cleaner code (single source of truth for "current time")
- Slightly faster (one function call vs three)
- More consistent (all timers use same timestamp for this iteration)

### Comparing timestamps instead of elapsed time
**Wrong:**
```cpp
if (millis() >= last_update + CHASE_SPEED) {  // ❌ Breaks on overflow!
```

**Why it breaks:**
```
Near overflow:
last_update = 4,294,967,200
CHASE_SPEED = 200
last_update + CHASE_SPEED = 4,294,967,400 (overflows to 104)
millis() = 50
Check: 50 >= 104 → FALSE (incorrect! 200ms definitely passed)
```

**Right:**
```cpp
if (now - last_update >= CHASE_SPEED) {  // ✅ Handles overflow correctly
```

**Why it works:**
```
now = 50
last_update = 4,294,967,200
now - last_update = (unsigned wraparound) = 200
Check: 200 >= 200 → TRUE (correct!)
```

## Further Reading
- [Arduino millis() Reference](https://www.arduino.cc/reference/en/language/functions/time/millis/)
- [Blink Without Delay Tutorial](https://www.arduino.cc/en/Tutorial/BuiltInExamples/BlinkWithoutDelay)
- [Understanding millis() Overflow](https://arduino.stackexchange.com/questions/12587/how-can-i-handle-the-millis-rollover)

## Why This Matters

**This is the #1 most important embedded programming pattern you'll learn.**

Non-blocking timing enables:
- ✅ Responsive user interfaces (buttons work instantly)
- ✅ Multiple simultaneous animations/tasks
- ✅ Sensor monitoring during operations
- ✅ Communication protocols (can't miss incoming data)
- ✅ Watchdog timer compatibility (must return to main loop regularly)
- ✅ Power management (can sleep between events)

**Every professional embedded system uses this pattern.** You'll see it in:
- Industrial controllers
- Consumer electronics
- Automotive systems
- Medical devices
- Robotics
- IoT devices

Master this pattern now. Everything from this point forward builds on it.
