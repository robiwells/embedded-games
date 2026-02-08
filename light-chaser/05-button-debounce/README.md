# Tutorial 5: Button Input with Debouncing

## What You'll Learn
- INPUT_PULLUP pin mode for buttons
- Edge detection (detecting button presses, not button state)
- Hardware debouncing problem and software solution
- Serial communication for debugging
- Active-low vs active-high logic
- Combining multiple non-blocking timers

## Prerequisites
- Tutorial 4: Non-Blocking Chase (understanding millis() timing pattern)

## Hardware Components
- Arduino Uno
- 8× LEDs (pins 2-9)
- 8× 220Ω resistors
- 1× Pushbutton (pin 10)

## Code Overview
This tutorial adds interactive gameplay: press the button when the LED is in the green zone (positions 3-4) to score points. Press when it's in the red zone and you lose!

**New functionality:**
- Button input with proper debouncing
- Edge detection (one action per press, not continuous)
- Scoring system with hit/miss detection
- Serial output for score display

## File Structure
```
05-button-debounce/
├── diagram.json          # Wokwi circuit (now includes button)
├── platformio.ini        # Build configuration
├── wokwi.toml           # Wokwi simulator settings
├── include/
│   └── config.h         # Hardware + game constants
└── src/
    └── main.cpp         # Chase + button handling
```

## Key Concepts

### INPUT_PULLUP: Button Wiring Made Easy

**Traditional button wiring (external pull-up resistor):**
```
     5V
      │
      ┝ 10kΩ resistor (external component needed!)
      │
Pin ──┤
      │
    [Button]
      │
     GND
```

**With INPUT_PULLUP (no external resistor needed):**
```
     5V (inside Arduino)
      │
      ┝ 20kΩ internal pull-up resistor (built-in!)
      │
Pin ──┤
      │
    [Button]
      │
     GND
```

**How INPUT_PULLUP works:**
```cpp
pinMode(BUTTON_PIN, INPUT_PULLUP);
```

1. Configures pin as input (reads voltage)
2. Enables internal 20kΩ pull-up resistor
3. When button not pressed: pin pulled to 5V → reads HIGH
4. When button pressed: pin connected to GND → reads LOW

**Active-low logic:**
- Button not pressed → pin reads HIGH (counterintuitive!)
- Button pressed → pin reads LOW
- We invert the reading in software to get natural logic

```cpp
bool button_pressed = !digitalRead(BUTTON_PIN);
// Inverts: LOW becomes true, HIGH becomes false
```

### Edge Detection vs Level Detection

**The Problem:**

```cpp
// ❌ WRONG: Level detection
if (digitalRead(BUTTON_PIN) == LOW) {
    score++;  // BUG! Increments every loop iteration while held
}
```

If loop() runs 1000 times per second and user holds button for 1 second:
- Score increments 1000 times from one press!
- Not what we want (one press should score once)

**The Solution: Edge Detection**

Detect **transitions** (button press events), not states:

```
Button State:  HIGH ────┐         ┌──── HIGH
                        │         │
                        └─────────┘ LOW
                        ↑         ↑
                   Press event  Release event
                   (detect this) (ignore)
```

**Implementation:**
```cpp
bool last_button_state = false;   // Previous state
bool current_state = !digitalRead(BUTTON_PIN);  // Current state (inverted)

if (current_state && !last_button_state) {
    // Rising edge detected! (unpressed → pressed transition)
    score++;  // Scores once per press
}

last_button_state = current_state;  // Remember for next check
```

**State table:**
```
Last │ Current │ Transition      │ Action
─────┼─────────┼─────────────────┼─────────────────
  0  │    0    │ Still released  │ Nothing
  0  │    1    │ Just pressed    │ DETECT (rising edge)
  1  │    0    │ Just released   │ Nothing (falling edge)
  1  │    1    │ Still pressed   │ Nothing
```

### Debouncing: The Mechanical Switch Problem

**What is bounce?**

When you press a physical button, the metal contacts don't make solid connection immediately. They bounce apart and together rapidly for 5-20ms:

```
Voltage (as seen by Arduino pin):
 5V ┤ ──┐   ┌──┐ ┌─┐ ┌────────  (Released)
    │   └───┘  └─┘ └─┘          (Pressed)
 0V ┤
    └────────────────────────► Time
        └────┬────┘
         5-20ms bounce period
```

**Without debouncing:**
- Edge detector sees 4-5 separate presses in 20ms
- One physical press = multiple detected presses
- Game becomes unplayable (button registers 3× per press)

**Debouncing solution:**

After detecting an edge, ignore all transitions for 50ms:

```cpp
const uint16_t DEBOUNCE_MS = 50;   // Lockout period
uint32_t last_debounce_time = 0;   // Timestamp of last press

if (current_state && !last_button_state) {
    // Edge detected, but is it valid?
    if (now - last_debounce_time >= DEBOUNCE_MS) {
        // Yes! Enough time passed since last press
        pressed = true;
        last_debounce_time = now;  // Lock out for 50ms
    }
}
```

**Timeline:**
```
Time  │ Event           │ Action
──────┼─────────────────┼────────────────────────────
0ms   │ Button pressed  │ Detect, set last_debounce_time = 0
5ms   │ Bounce          │ Ignore (5 - 0 = 5ms < 50ms)
10ms  │ Bounce          │ Ignore (10 - 0 = 10ms < 50ms)
15ms  │ Bounce          │ Ignore (15 - 0 = 15ms < 50ms)
100ms │ Press again     │ Detect! (100 - 0 = 100ms > 50ms)
```

**Why 50ms?**
- Too short (10ms): Might not filter all bounces
- Too long (200ms): User can't press rapidly (feels sluggish)
- 50ms: Filters bounces, feels responsive
- Tuned experimentally based on typical button characteristics

### Serial Communication for Debugging

**Setup:**
```cpp
Serial.begin(9600);  // 9600 baud (bits per second)
```

Initialises UART (Universal Asynchronous Receiver-Transmitter) for serial communication with computer.

**Sending data:**
```cpp
Serial.print("Score: ");    // Print text (no newline)
Serial.println(score);       // Print number + newline
```

**Where it goes:**
- In simulator: Wokwi serial monitor
- On real hardware: USB-to-serial converter → computer
- View in: Arduino IDE Serial Monitor, PlatformIO Serial Monitor, etc.

**Why use serial for debugging:**
- Can't use printf/cout on Arduino (no operating system)
- Serial is the embedded equivalent of console.log/print
- Works on real hardware (not just simulator)
- Minimal overhead (~100 bytes of code)

### Combining Multiple Timers

This tutorial has **two independent timers**:
1. Chase LED update (200ms interval)
2. Button debounce (50ms lockout)

**Both use the same millis() pattern:**

```cpp
void loop() {
    uint32_t now = millis();  // Single timestamp for this iteration

    // Timer 1: Chase LED
    if (now - last_chase_update >= CHASE_SPEED) {
        last_chase_update = now;
        update_chase();
    }

    // Timer 2: Button debounce (inside button_just_pressed())
    if (now - last_debounce_time >= DEBOUNCE_MS) {
        // ... debounce check ...
    }

    // Both timers run independently!
}
```

**This scales to any number of timers:**
- 3 animations with different speeds
- Sensor polling at different rates
- Communication timeouts
- All running in parallel, non-blocking

## Code Walkthrough

### Button State Variables

```cpp
bool last_button_state = false;      // Previous button state
uint32_t last_debounce_time = 0;     // Timestamp of last validated press
```

**Initialisation in setup():**
```cpp
last_button_state = !digitalRead(BUTTON_PIN);
```

Sync to current physical state. If button happens to be pressed at boot, we don't detect it as a new press.

### button_just_pressed() Function

```cpp
bool button_just_pressed() {
    uint32_t now = millis();
    bool current_state = !digitalRead(BUTTON_PIN);  // Inverted (active-low)
    bool pressed = false;

    if (current_state && !last_button_state) {
        // Edge detected!
        if (now - last_debounce_time >= DEBOUNCE_MS) {
            // Valid press (debounce timeout passed)
            pressed = true;
            last_debounce_time = now;
        }
    }

    last_button_state = current_state;  // Remember for next call
    return pressed;
}
```

**Returns:**
- `true` if button was just pressed (validated rising edge)
- `false` otherwise (no press, or bounce/too soon)

**Call every loop iteration** for proper edge tracking.

### Hit Detection

```cpp
if (button_just_pressed()) {
    if (current_position >= TARGET_ZONE_START && current_position <= TARGET_ZONE_END) {
        // Hit green zone!
        score += 10;
        Serial.print("Hit! Score: ");
        Serial.println(score);
    } else {
        // Missed (red zone)
        Serial.print("Miss! Final score: ");
        Serial.println(score);
        score = 0;  // Reset score on miss
    }
}
```

**Target zone:** Positions 3-4 (green LEDs in middle)
**Scoring:** +10 points for hit, reset to 0 for miss

## Running the Code

### Using PlatformIO + Wokwi:
1. Open this folder in VS Code
2. Build: `pio run`
3. Upload to Wokwi simulator
4. Start simulation
5. Open Serial Monitor (click serial monitor icon in Wokwi)

### What You Should See:
- LED chases back and forth (same as Tutorial 4)
- When you press button:
  - **Green zone (positions 3-4):** Serial prints "Hit! Score: 10" (or 20, 30...)
  - **Red zone (other positions):** Serial prints "Miss! Final score: X", score resets

### Playing the Game:
1. Watch the LED bounce
2. Time your button press for when LED is in green zone (positions 3-4)
3. Try to score as high as possible before missing!

## Exercises

### Easy: Change target zone
1. Modify `TARGET_ZONE_START` and `TARGET_ZONE_END` in config.h
2. Try: Single LED target (both = 3), larger zone (0-2), moving target

### Medium: Add difficulty progression
Make the chase speed increase after each hit:
```cpp
if (hit) {
    score += 10;
    if (CHASE_SPEED > 50) {
        CHASE_SPEED -= 10;  // Speed up (make it a variable, not const)
    }
}
```

Game gets harder as you score more!

### Hard: Multi-level scoring
Instead of binary hit/miss, implement graduated scoring:
- Positions 3-4 (dead centre): 10 points
- Positions 2, 5 (adjacent): 5 points
- Positions 0-1, 6-7 (edges): 1 point

You'll need to check which zone the position is in and award different points.

## Next Steps
- **Next tutorial:** [06-simple-state-machine](../06-simple-state-machine/) - Introduce game states
- **New concepts:** State machines, game modes, attract screen, game over

## Common Pitfalls

### Using delay() for debouncing
**Wrong:**
```cpp
if (button_pressed) {
    delay(50);  // ❌ Blocks everything!
    score++;
}
```

This blocks the entire program for 50ms. LED animation freezes, can't detect button release, terrible user experience.

**Right:**
```cpp
if (button_pressed && now - last_debounce_time >= 50) {
    last_debounce_time = now;  // ✅ Non-blocking
    score++;
}
```

### Forgetting to invert INPUT_PULLUP
**Wrong:**
```cpp
pinMode(BUTTON_PIN, INPUT_PULLUP);
if (digitalRead(BUTTON_PIN) == HIGH) {  // ❌ Backwards!
    // This triggers when button is NOT pressed
}
```

**Right:**
```cpp
pinMode(BUTTON_PIN, INPUT_PULLUP);
bool pressed = !digitalRead(BUTTON_PIN);  // ✅ Inverted
if (pressed) {
    // Correctly detects press
}
```

Or use LOW directly:
```cpp
if (digitalRead(BUTTON_PIN) == LOW) {  // ✅ Also correct
```

### Level detection instead of edge detection
**Wrong:**
```cpp
if (!digitalRead(BUTTON_PIN)) {
    score++;  // ❌ Increments every loop while held!
}
```

**Right:**
```cpp
if (button_just_pressed()) {
    score++;  // ✅ Increments once per press
}
```

### Not calling button check every loop
**Wrong:**
```cpp
void loop() {
    update_chase();

    if (some_condition) {
        if (button_just_pressed()) { /* ... */ }
    }
    // Button check skipped when condition false!
}
```

**Why it's bad:**
Edge detector needs to track every state change. If you skip checks, you miss transitions.

**Right:**
```cpp
void loop() {
    update_chase();

    if (button_just_pressed()) {
        if (some_condition) {
            // Handle press when condition met
        }
    }
    // Always check button, decide what to do with result
}
```

## Further Reading
- [Arduino INPUT_PULLUP](https://www.arduino.cc/en/Tutorial/DigitalInputPullup)
- [Switch Debouncing](https://www.arduino.cc/en/Tutorial/Debounce)
- [Arduino Serial Reference](https://www.arduino.cc/reference/en/language/functions/communication/serial/)

## Why This Matters

Button handling is fundamental to embedded systems:
- **User interfaces:** Every button, switch, joystick in consumer electronics
- **Industrial controls:** Emergency stops, limit switches, panel buttons
- **Robotics:** Tactile sensors, collision detection switches
- **Automotive:** Buttons, pedals, seatbelt sensors

The debouncing + edge detection pattern you learned here applies to **any digital input**:
- Mechanical switches
- Magnetic reed switches
- Optical sensors
- Proximity sensors
- Limit switches

Master this, and you can handle any binary input reliably.
