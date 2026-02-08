# Tutorial 3: LED Chase Basic

## What You'll Learn
- Arrays and iteration with for loops
- Position tracking with variables
- Bounce logic using direction reversal
- Working with multiple LEDs simultaneously
- Signed vs unsigned integers (int8_t vs uint8_t)

## Prerequisites
- Tutorial 2: Single LED Blink (understanding setup/loop, pinMode, digitalWrite, delay)

## Hardware Components
- Arduino Uno
- 8× LEDs (pins 2-9)
- 8× 220Ω resistors

## Code Overview
This tutorial creates a classic "chase" animation where a single LED moves back and forth across the row. The LED "bounces" when it reaches either end, like a bouncing ball.

**Visual pattern:**
```
[*][ ][ ][ ][ ][ ][ ][ ]  → moving right
[ ][*][ ][ ][ ][ ][ ][ ]  → moving right
[ ][ ][*][ ][ ][ ][ ][ ]  → moving right
...
[ ][ ][ ][ ][ ][ ][*][ ]  → moving right
[ ][ ][ ][ ][ ][ ][ ][*]  → moving right (hit right edge, reverse!)
[ ][ ][ ][ ][ ][ ][*][ ]  ← moving left
[ ][ ][ ][ ][ ][*][ ][ ]  ← moving left
...
```

## File Structure
```
03-led-chase-basic/
├── diagram.json          # Wokwi circuit (all 8 LEDs)
├── platformio.ini        # Build configuration
├── wokwi.toml           # Wokwi simulator settings
├── include/
│   └── config.h         # Hardware constants
└── src/
    └── main.cpp         # Chase animation logic
```

## Key Concepts

### Arrays of Pins

Instead of controlling each LED with separate variables and code, we use the fact that our LEDs are on **consecutive pins** (2, 3, 4, 5, 6, 7, 8, 9).

**Array-style access pattern:**
```cpp
const uint8_t LED_PIN_START = 2;   // First LED on pin 2
const uint8_t NUM_LEDS = 8;        // 8 LEDs total

// To access LED 0: LED_PIN_START + 0 = pin 2
// To access LED 1: LED_PIN_START + 1 = pin 3
// To access LED i: LED_PIN_START + i
```

This lets us use loops instead of repetitive code:

**Without array pattern (terrible):**
```cpp
digitalWrite(2, LOW);
digitalWrite(3, LOW);
digitalWrite(4, LOW);
digitalWrite(5, LOW);
digitalWrite(6, LOW);
digitalWrite(7, LOW);
digitalWrite(8, LOW);
digitalWrite(9, LOW);
// Imagine changing this for 50 LEDs!
```

**With array pattern (elegant):**
```cpp
for (uint8_t i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LED_PIN_START + i, LOW);
}
// Works for any number of LEDs - just change NUM_LEDS!
```

### For Loops

The `for` loop executes a block of code a specific number of times.

**Syntax:**
```cpp
for (initialisation; condition; increment) {
    // Code to repeat
}
```

**Example:**
```cpp
for (uint8_t i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LED_PIN_START + i, LOW);
}
```

**Execution breakdown:**
1. **Initialisation:** `uint8_t i = 0` - Create counter variable i, set to 0
2. **Condition check:** `i < NUM_LEDS` (is `i < 8`?) - If false, exit loop
3. **Body execution:** `digitalWrite(LED_PIN_START + i, LOW)` - Run loop body
4. **Increment:** `i++` - Increase i by 1
5. **Repeat:** Go back to step 2

**Iterations:**
```
Iteration 0: i=0, writes to pin 2 (LED_PIN_START + 0)
Iteration 1: i=1, writes to pin 3 (LED_PIN_START + 1)
Iteration 2: i=2, writes to pin 4 (LED_PIN_START + 2)
...
Iteration 7: i=7, writes to pin 9 (LED_PIN_START + 7)
Iteration 8: i=8, condition fails (8 < 8 is false), loop exits
```

### Position Tracking

We track the current LED position with a variable:

```cpp
uint8_t current_position = 0;  // Current LED index (0-7)
```

Each loop iteration:
1. Turn off all LEDs
2. Turn on LED at `current_position`
3. Wait (delay)
4. Update `current_position` for next iteration

### Bounce Logic: Direction Reversal

The trick to bouncing is tracking **direction** and reversing it at boundaries.

**Direction variable:**
```cpp
int8_t direction = 1;  // +1 = moving right, -1 = moving left
```

**Why int8_t (signed) instead of uint8_t (unsigned)?**
- Direction can be **negative** (-1 for moving left)
- Signed integers support negative values
- Range: -128 to +127 (we only need -1 and +1)

**Movement update:**
```cpp
current_position += direction;
```

If `direction = 1`: `current_position` increases (move right)
If `direction = -1`: `current_position` decreases (move left)

**Boundary detection and reversal:**
```cpp
if (current_position == 0) {
    direction = 1;   // Hit left edge, start moving right
} else if (current_position == NUM_LEDS - 1) {
    direction = -1;  // Hit right edge, start moving left
}
```

**Trace through bounce:**
```
Position: 5, Direction: +1
    └─> 5 + 1 = 6 (move right)
Position: 6, Direction: +1
    └─> 6 + 1 = 7 (move right)
Position: 7, Direction: +1 (hit boundary!)
    └─> Check: position == 7 (NUM_LEDS - 1)
    └─> Set direction = -1
    └─> 7 + (-1) = 6 (move left)
Position: 6, Direction: -1
    └─> 6 + (-1) = 5 (move left)
```

### Why This Still Uses delay()

This tutorial intentionally uses `delay()` to keep the code simple and focused on chase logic. The problem with `delay()`:

**During delay(200), the Arduino:**
- ❌ Can't read button presses
- ❌ Can't update other animations
- ❌ Can't respond to any input
- ❌ Just sits idle, waiting for time to pass

This is **fine for learning**, but **bad for real applications**. We'll fix this in Tutorial 4 with non-blocking timing.

## Code Walkthrough

### setup() - Initialise All LEDs
```cpp
void setup() {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        pinMode(LED_PIN_START + i, OUTPUT);  // Configure pins 2-9 as outputs
        digitalWrite(LED_PIN_START + i, LOW);  // Start with all LEDs off
    }
}
```

Runs once at boot. Configures all 8 pins and ensures all LEDs start in the off state.

### loop() - Chase Animation
```cpp
void loop() {
    // 1. Turn off all LEDs
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        digitalWrite(LED_PIN_START + i, LOW);
    }

    // 2. Turn on LED at current position
    digitalWrite(LED_PIN_START + current_position, HIGH);

    // 3. Wait (creates visible delay between movements)
    delay(CHASE_DELAY);

    // 4. Update position for next iteration
    current_position += direction;

    // 5. Check boundaries and reverse direction if needed
    if (current_position == 0) {
        direction = 1;   // Hit left edge, go right
    } else if (current_position == NUM_LEDS - 1) {
        direction = -1;  // Hit right edge, go left
    }
}
```

Runs forever. Each iteration moves the LED one position.

## Running the Code

### Using PlatformIO + Wokwi:
1. Open this folder in VS Code
2. Build: `pio run`
3. Upload to Wokwi simulator
4. Start simulation

### What You Should See:
- Single LED moves smoothly from left to right (200ms per step)
- When it reaches the rightmost LED (position 7), it reverses
- LED then moves right to left
- When it reaches the leftmost LED (position 0), it reverses
- Pattern repeats forever (bouncing back and forth)

**Speed:** 200ms per LED = 8 LEDs × 200ms = 1.6 seconds for full sweep

## Exercises

### Easy: Change the chase speed
1. Modify `CHASE_DELAY` in config.h
2. Try: `100` (faster), `500` (slower), `50` (very fast)
3. What's the fastest speed where you can still see individual LEDs?

### Medium: Double bounce
Modify the code to make the LED "bounce twice" at each end:
- Reaches right edge → move left one step → move right one step → continue left
- Reaches left edge → move right one step → move left one step → continue right

Hint: You'll need a counter variable to track how many times you've bounced at the current edge.

### Hard: Comet tail effect
Instead of just one LED lit, light multiple LEDs with decreasing brightness (simulating a comet with a tail). You'll need:
- Track multiple positions (not just `current_position`)
- Use analogWrite() instead of digitalWrite() for brightness control
- Fade the tail LEDs over several positions

Example (position 5 moving right):
```
[ ][ ][ ][dim][med][bright][ ][ ]
```

## Next Steps
- **Next tutorial:** [04-nonblocking-chase](../04-nonblocking-chase/) - Remove delay(), use millis() timestamps
- **New concepts:** Non-blocking timing, millis(), elapsed time calculations
- **Critical skill:** This is the most important transition in embedded programming!

## Common Pitfalls

### Off-by-one errors in boundary checking
**Wrong:**
```cpp
if (current_position == NUM_LEDS) {  // ❌ Will never be true before overflow!
    direction = -1;
}
```

When position is 7 and direction is +1:
- `current_position += direction` → position becomes 8
- `digitalWrite(LED_PIN_START + 8, HIGH)` → writes to pin 10 (wrong!)
- On next iteration: position becomes 9, 10, 11... (runaway!)

**Right:**
```cpp
if (current_position == NUM_LEDS - 1) {  // ✅ Check BEFORE incrementing past boundary
    direction = -1;
}
```

Checks when position is still 7 (valid), reverses direction before next increment.

### Clearing LEDs inside movement loop
**Inefficient:**
```cpp
void loop() {
    current_position += direction;
    digitalWrite(LED_PIN_START + old_position, LOW);  // Turn off previous
    digitalWrite(LED_PIN_START + current_position, HIGH);  // Turn on current
    delay(CHASE_DELAY);
}
```

Problems:
- Need to track `old_position` separately
- Harder to modify (what if you want 2 LEDs lit?)
- More complex logic

**Better:**
```cpp
void loop() {
    // Clear ALL LEDs every frame (simple, always correct)
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        digitalWrite(LED_PIN_START + i, LOW);
    }
    // Light current position
    digitalWrite(LED_PIN_START + current_position, HIGH);
    // ...
}
```

Benefits:
- Always starts with clean slate
- Easy to modify (light multiple LEDs, patterns, etc.)
- Self-correcting (can't leave LEDs stuck on)

### Using unsigned type for direction
**Wrong:**
```cpp
uint8_t direction = -1;  // ❌ Unsigned can't represent negative numbers!
```

What actually happens:
- `-1` as unsigned 8-bit wraps to 255
- `current_position += 255` is equivalent to `current_position -= 1` due to wraparound
- Works accidentally, but relies on undefined behaviour
- Very confusing to read and debug

**Right:**
```cpp
int8_t direction = -1;  // ✅ Signed integer, -1 is valid
```

## Further Reading
- [Arduino For Loop Reference](https://www.arduino.cc/reference/en/language/structure/control-structure/for/)
- [Integer Overflow/Underflow](https://en.wikipedia.org/wiki/Integer_overflow)
- [Signed vs Unsigned Integers](https://www.arduino.cc/reference/en/language/variables/data-types/unsignedint/)

## Why This Matters
This tutorial teaches fundamental embedded patterns:
- **Iteration**: for loops are everywhere in embedded (sensor arrays, LED matrices, communication buffers)
- **Bounce logic**: Same technique used in UI scrolling, motor control limits, audio envelope generation
- **Position tracking**: Foundation for any movement/animation system

The concepts here scale to industrial applications: robotic arm position control uses the same bounce/reversal logic, just with stepper motors instead of LEDs.
