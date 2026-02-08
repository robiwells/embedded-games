# Tutorial 2: Single LED Blink

## What You'll Learn
- Basic GPIO (General Purpose Input/Output) control
- The Arduino setup() and loop() pattern
- pinMode(), digitalWrite(), and delay() functions
- Understanding program structure and execution flow

## Prerequisites
- Tutorial 1: Circuit Only (understanding the hardware layout)

## Hardware Components
- Arduino Uno
- 1× LED (we'll use the first red LED on pin 2)
- 1× 220Ω resistor

## Code Overview
This tutorial introduces the classic "Hello, World!" of embedded systems: blinking an LED. The code:
1. Configures pin 2 as an OUTPUT in setup()
2. Repeatedly turns the LED on for 500ms, off for 500ms in loop()
3. Creates a visible 1Hz blink pattern (1 cycle per second)

## File Structure
```
02-single-led-blink/
├── diagram.json          # Wokwi circuit (same as Tutorial 1)
├── platformio.ini        # Build configuration
├── wokwi.toml           # Wokwi simulator settings
├── include/
│   └── config.h         # Hardware pin definitions
└── src/
    └── main.cpp         # Main program
```

## Key Concepts

### The Arduino Program Structure

Every Arduino program has two required functions:

**setup()** - Runs once when the Arduino powers on or resets
- Purpose: Initialise hardware, set pin modes, configure peripherals
- Timing: Executes immediately after boot, before loop() starts
- Use case: One-time configuration that doesn't need to repeat

**loop()** - Runs repeatedly forever after setup() completes
- Purpose: Main program logic, continuous operations
- Timing: Executes as fast as possible (unless delayed by delay() or other blocking calls)
- Use case: All ongoing behaviour - reading sensors, updating outputs, checking conditions

Think of it like:
```cpp
void main() {
    setup();           // Run once
    while (true) {     // Run forever
        loop();
    }
}
```

### GPIO Pin Configuration: pinMode()

Before using a pin, you must configure its direction:

```cpp
pinMode(pin_number, mode);
```

**Modes:**
- `OUTPUT` - Pin drives voltage (sources/sinks current) - use for LEDs, buzzers, etc.
- `INPUT` - Pin reads voltage (high impedance) - use for sensors
- `INPUT_PULLUP` - Pin reads voltage with internal pull-up resistor enabled (we'll use this for the button in Tutorial 5)

**Why this is necessary:**
Arduino pins can be configured as inputs OR outputs. The hardware physically switches internal transistors to change the pin's electrical behaviour. You must tell the microcontroller which mode before using the pin.

**In our code:**
```cpp
pinMode(LED_PIN_START, OUTPUT);
```
This configures pin 2 as an output so we can drive current through the LED.

### Controlling Pin State: digitalWrite()

Once a pin is configured as OUTPUT, you can set its voltage level:

```cpp
digitalWrite(pin_number, state);
```

**States:**
- `HIGH` - Sets pin to +5V (logic 1, binary 1, "on")
- `LOW` - Sets pin to 0V (logic 0, binary 0, "off")

**How it works electrically:**
- `HIGH`: Arduino's internal transistor connects pin to +5V rail → current flows through LED → LED lights up
- `LOW`: Arduino's internal transistor connects pin to GND (0V) → no voltage difference → LED stays off

**In our code:**
```cpp
digitalWrite(LED_PIN_START, HIGH);  // LED turns on
digitalWrite(LED_PIN_START, LOW);   // LED turns off
```

### Timing with delay()

```cpp
delay(milliseconds);
```

**What it does:**
Pauses program execution for the specified number of milliseconds.

**How it works:**
Blocking function - the CPU literally does nothing (enters a tight loop checking millis() internally) until time expires.

**Duration conversion:**
- 1000 ms = 1 second
- 500 ms = 0.5 seconds (half-second)
- 100 ms = 0.1 seconds (tenth of a second)

**In our code:**
```cpp
delay(500);  // Pause for 500 milliseconds (0.5 seconds)
```

**WARNING:** delay() is convenient but problematic for complex programs:
- **Blocks everything** - Can't read buttons, can't update other LEDs, can't respond to anything
- **Wastes CPU** - Microcontroller just sits idle instead of doing useful work
- **Not scalable** - Adding multiple delays creates timing conflicts

We're using delay() here for simplicity, but we'll fix this with non-blocking timing in Tutorial 4.

### Data Types: uint8_t

```cpp
const uint8_t LED_PIN_START = 2;
```

**What is uint8_t?**
- `uint` = unsigned (no negative values, range 0 to positive max)
- `8` = 8 bits of storage
- `_t` = type (naming convention for portable types)

**Why use uint8_t instead of int?**

**Memory efficiency:**
| Type | Size | Range | Notes |
|------|------|-------|-------|
| int | 16 bits (2 bytes) | -32,768 to +32,767 | Arduino default integer type |
| uint8_t | 8 bits (1 byte) | 0 to 255 | Perfect for pin numbers (0-13) |

**Arduino Uno has only 2KB of RAM.** Using uint8_t instead of int for pin numbers saves 1 byte per variable. This adds up quickly in larger programs.

**When to use uint8_t:**
- Pin numbers (0-13 fits in 0-255 range)
- Counters with small maximum values
- Boolean flags (0 or 1)
- Any value guaranteed to be 0-255

**When to use larger types:**
- `uint16_t` (0-65,535): Timing values (milliseconds), sensor readings
- `uint32_t` (0-4,294,967,295): Timestamps from millis()
- `int` / `int16_t`: Values that can be negative

### Constants with const

```cpp
const uint8_t LED_PIN_START = 2;
```

**Why use const?**
1. **Prevents accidental modification** - Compiler error if you try to change the value
2. **Self-documenting** - Clearly indicates this value shouldn't change
3. **Optimisation** - Compiler can replace constant with its literal value (no RAM used)
4. **Magic number elimination** - `LED_PIN_START` is clearer than `2` scattered throughout code

**Flash vs RAM storage:**
- `const` variables live in Flash memory (32KB) - the read-only program storage
- Non-const variables live in RAM (2KB) - scarce resource
- Always use `const` for values that never change at runtime

## Running the Code

### Using PlatformIO + Wokwi:
1. Open this folder in VS Code with PlatformIO extension installed
2. Click the "Build" button (✓ icon) in the bottom toolbar or run: `pio run`
3. Open the Wokwi simulator and upload the firmware
4. Click "Start Simulation"

### What You Should See:
- The first LED (leftmost red LED on pin 2) blinks on and off
- **On phase**: LED lit for 500ms (half-second)
- **Off phase**: LED dark for 500ms (half-second)
- **Frequency**: 1 complete cycle per second (1 Hz)
- Pattern repeats forever

### Debugging If It Doesn't Work:
- **No LEDs light up**: Check that build succeeded, firmware uploaded correctly
- **Wrong LED blinks**: Verify LED_PIN_START = 2 in config.h
- **LED always on or always off**: Check digitalWrite() calls have HIGH and LOW (not both HIGH or both LOW)
- **Blink too fast/slow**: Modify delay() values in main.cpp

## Exercises

### Easy: Change the blink rate
1. Modify the `delay(500)` values in main.cpp to change blink speed
2. Try: 1000 (slower), 250 (faster), 100 (rapid blink)
3. What happens with delay(10)? delay(1)?

### Medium: Create a heartbeat pattern
Instead of equal on/off times, create a "double-pulse" pattern:
- Short blink (100ms on, 100ms off)
- Short blink (100ms on, 100ms off)
- Long pause (1000ms off)
- Repeat

Hint: You'll need 6 delay() calls total.

### Hard: Blink multiple LEDs
Modify main.cpp to blink the first THREE LEDs (pins 2, 3, 4) simultaneously. You'll need:
- 3× pinMode() calls in setup()
- 6× digitalWrite() calls in loop() (3 HIGH, 3 LOW)
- All LEDs should blink in unison

**Challenge:** Can you make them blink in sequence instead? (LED 0 on, LED 1 on, LED 2 on, all off, repeat)

## Next Steps
- **Next tutorial:** [03-led-chase-basic](../03-led-chase-basic/) - Chase animation with bounce logic
- **New concepts:** Arrays, loops, position tracking, direction reversal

## Common Pitfalls

### Forgetting pinMode()
**Wrong:**
```cpp
void setup() {
    // Nothing here!
}
void loop() {
    digitalWrite(2, HIGH);  // Pin not configured - undefined behaviour
}
```

**Right:**
```cpp
void setup() {
    pinMode(2, OUTPUT);     // Configure before use
}
void loop() {
    digitalWrite(2, HIGH);  // Now it works
}
```

### Using delay() in production code
delay() is fine for learning, but terrible for real applications:
- Can't handle button presses during delay
- Can't update multiple things with different timing
- Wastes power (CPU running but doing nothing)

We'll fix this in Tutorial 4 with non-blocking timing.

## Further Reading
- [Arduino digitalWrite Reference](https://www.arduino.cc/reference/en/language/functions/digital-io/digitalwrite/)
- [Arduino pinMode Reference](https://www.arduino.cc/reference/en/language/functions/digital-io/pinmode/)
- [Arduino delay Reference](https://www.arduino.cc/reference/en/language/functions/time/delay/)
- [Arduino Data Types](https://www.arduino.cc/reference/en/language/variables/data-types/)
