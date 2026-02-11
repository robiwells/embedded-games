# Quick Draw - Hardware Interrupts Tutorial Game

**Game 2 of the Embedded Games Tutorial Series**

## Overview

Quick Draw is a reaction-time measurement game that teaches **hardware interrupts** - a fundamental embedded systems concept. This game builds on the state machine patterns from Game 1 (Light Chaser) and introduces microsecond-precision input handling through interrupt-driven button input.

### What You'll Learn

- **Hardware interrupt setup** using `attachInterrupt()`
- **ISR (Interrupt Service Routine)** implementation and best practices
- **`volatile` keyword** for ISR-main loop communication
- **Microsecond timing** with `micros()` vs `millis()`
- **Software debouncing** in interrupt context
- **State validation pattern** - ISR captures moment-in-time data, main loop validates context
- **When to use interrupts vs polling** - decision matrix and trade-offs

## Gameplay

1. **Attract Screen** - Shows your best time from EEPROM
2. **Get Ready** - Random countdown (2-4 seconds) with distractor LED flashes
3. **Draw!** - Middle LED lights up - press button as fast as possible
4. **Result** - Shows your reaction time with feedback ("Lightning!", "Quick!", "OK", or "Slow")
5. **5 Rounds** - Game tracks average and best times
6. **New Record** - Celebration animation and EEPROM save when you beat your personal best

### False Starts

- Press button during countdown → "False Start!" penalty
- Press button during distractor LED flash → "False Start!" penalty
- Only presses during the real draw LED count!

### Reaction Time Feedback

- **Lightning!** - Under 300ms
- **Quick!** - Under 500ms
- **OK** - Under 800ms
- **Slow** - 800ms or slower

## Hardware Setup

### Required Components

- **Arduino Uno** (or compatible)
- **Push button** on **pin 2** (INT0 interrupt pin)
- **8 LEDs** on pins 3-10
- **Buzzer** on pin 11
- **16x2 I2C LCD** (SDA=A4, SCL=A5)

### Pin Configuration

```
Pin 2  - Button (INT0 interrupt capable)
Pins 3-10 - LEDs (index 4 is the "draw" LED)
Pin 11 - Buzzer
A4/A5 - I2C LCD (SDA/SCL)
```

**Important:** Arduino Uno only supports external interrupts on pins 2 (INT0) and 3 (INT1). This game uses pin 2.

## Key Teaching Concepts

### Hardware Interrupts vs Polling

#### Polling (Game 1 - Light Chaser)
```cpp
void loop() {
    if (button_just_pressed()) {  // Checked every loop iteration (~20ms)
        handle_button_press();
    }
}
```

**Limitations:**
- Precision: ~20ms (depends on loop speed)
- Can miss events between polls
- CPU wastes cycles checking

#### Interrupts (Game 2 - Quick Draw)
```cpp
void button_isr(void) {
    button_press_time_us = micros();  // <1µs response time
    button_pressed_flag = true;
}
```

**Advantages:**
- Precision: <1µs response time
- Never miss events (hardware-triggered)
- CPU does other work until event occurs

### ISR Best Practices

**CRITICAL CONSTRAINTS:**

1. **Keep execution time < 10µs** (ideally < 5µs)
2. **NO blocking calls:** No `delay()`, `Serial.print()`, `lcd.print()`
3. **NO complex logic** - just capture data and set flags
4. **Only access `volatile` variables**
5. **Return immediately**

**The Pattern:**

```cpp
// ISR: Capture moment-in-time data
volatile bool button_flag = false;
volatile uint32_t button_time = 0;
volatile GameState state_at_press = STATE_ATTRACT;

void button_isr(void) {
    // 1. Debounce
    uint32_t current_time = micros();
    if ((current_time - last_interrupt_time) > DEBOUNCE_US) {
        // 2. Capture data
        last_interrupt_time = current_time;
        button_time = current_time;
        state_at_press = game_get_state();

        // 3. Signal main loop
        button_flag = true;
    }
}

// Main loop: Process with full context
void draw_update(void) {
    if (button_was_pressed()) {
        // Validate state (ISR captured the moment-in-time state)
        if (button_get_state_at_press() == STATE_DRAW) {
            uint32_t reaction = button_time - draw_start;
            // Process valid reaction...
        } else {
            // Pressed during wrong state - false start
        }
    }
}
```

### The `volatile` Keyword

**Why `volatile` is Required:**

Without `volatile`, the compiler might optimise away checks:

```cpp
// BAD - Compiler may cache button_flag in a register
bool button_flag = false;

void loop() {
    while (!button_flag) {  // Compiler: "This never changes in the loop!"
        // Infinite loop - compiler doesn't see ISR changes
    }
}

// GOOD - Compiler always reads from memory
volatile bool button_flag = false;

void loop() {
    while (!button_flag) {  // Compiler: "Must check every time!"
        // Correctly exits when ISR sets flag
    }
}
```

**Rule:** Any variable shared between ISR and main code **MUST** be `volatile`.

### Software Debouncing in Interrupts

Physical buttons "bounce" (make/break contact multiple times in ~5-50ms). At microsecond precision, this creates hundreds of false triggers:

```cpp
volatile uint32_t last_interrupt_time_us = 0;
const uint32_t DEBOUNCE_US = 50000;  // 50ms

void button_isr(void) {
    uint32_t current_time = micros();

    // Ignore interrupts within 50ms of last one
    if ((current_time - last_interrupt_time_us) > DEBOUNCE_US) {
        last_interrupt_time_us = current_time;
        // Process interrupt...
    }
    // Bounces ignored silently
}
```

### State Validation Pattern

**The Challenge:** Button presses can happen in any state, but only some are valid.

**The Solution:** ISR captures the game state at the moment of the press:

```cpp
// ISR captures snapshot
void button_isr(void) {
    state_at_press = game_get_state();  // What state were we in?
    button_time = micros();              // When did it happen?
    button_flag = true;                  // Signal main loop
}

// Main loop validates with full context
void draw_update(void) {
    if (button_was_pressed()) {
        if (button_get_state_at_press() == STATE_DRAW) {
            // Valid: Pressed during draw state
            calculate_reaction_time();
        } else {
            // Invalid: Pressed during distractor or wrong state
            false_start();
        }
    }
}
```

## Architecture

### Enter/Exit/Update State Machine (from Game 1)

```cpp
typedef struct {
    void (*enter)(void);   // Called once when entering state
    void (*update)(void);  // Called every frame while in state
    void (*exit)(void);    // Called once when leaving state
} StateHandler;
```

All state transitions go through `game_transition_to()`:

```cpp
void game_transition_to(GameState new_state) {
    state_handlers[current_state].exit();   // Cleanup old state
    current_state = new_state;
    state_handlers[new_state].enter();      // Initialise new state
}
```

### 3-Layer Architecture

```
main.cpp          - Entry point, watchdog timer
    ↓
game.cpp          - State machine, game logic
    ↓
hardware.cpp      - HAL (ISR, LEDs, LCD, EEPROM)
```

**Separation of Concerns:**
- Game logic never touches pins or hardware registers
- Hardware layer exposes clean API: `button_was_pressed()`, `led_set()`, etc.
- Easy to port to different hardware platforms

### Game States

```
STATE_ATTRACT          - Waiting to start, show best time
STATE_READY            - Countdown with distractor LEDs
STATE_DRAW             - Draw LED on, measure reaction
STATE_RESULT           - Show reaction time and feedback
STATE_ROUND_COMPLETE   - Brief pause, show stats
STATE_FALSE_START      - Penalty for pressing too early
STATE_NEW_RECORD       - Celebration for personal best
STATE_GAME_COMPLETE    - Final stats, return to attract
```

## Building and Running

### PlatformIO

```bash
cd quick-draw/game
pio run --target upload
```

### Arduino IDE

1. Install library: `LiquidCrystal_I2C` (by Marco Schwartz)
2. Open `src/main.cpp`
3. Set board to "Arduino Uno"
4. Upload

### Wokwi Simulator

Test without hardware:
1. Copy project to [Wokwi](https://wokwi.com/)
2. Configure `diagram.json` for virtual components
3. Run in browser

## Code Structure

```
quick-draw/
├── 01-polling-baseline/        (Tutorial: Polling approach for comparison)
├── 02-simple-interrupt/        (Tutorial: First interrupt implementation)
├── 03-debounced-interrupt/     (Tutorial: Add software debouncing)
├── 04-false-start-detection/   (Tutorial: Defensive interrupt usage)
├── 05-distractor-leds/         (Tutorial: State validation pattern)
├── 06-complete-game/           (Production-quality complete game)
├── game/                       (Symlink to 06-complete-game)
└── README.md                   (This file)
```

### Tutorial Progression

Each tutorial stage builds on the previous, teaching one new concept:

1. **Polling Baseline** - Establish polling limitations (~20ms jitter)
2. **Simple Interrupt** - Migrate to INT0, see microsecond precision (and bounce bugs!)
3. **Debounced Interrupt** - Add 50ms software debounce in ISR
4. **False Start Detection** - Use interrupts across multiple states
5. **Distractor LEDs** - Teach state validation (ISR captures, main validates)
6. **Complete Game** - 5 rounds, statistics, EEPROM, new record celebration

## Interrupt vs Polling Decision Matrix

| Scenario | Use Interrupts When... | Use Polling When... |
|----------|------------------------|---------------------|
| **Timing Precision** | Need <1ms precision | 10-20ms is acceptable |
| **Event Frequency** | Events are rare/unpredictable | Events checked every loop anyway |
| **CPU Usage** | Want CPU free for other tasks | Simple single-task loop |
| **Complexity** | Worth the ISR constraints | Want simpler code |
| **Hardware Support** | Interrupt pins available | Limited interrupt pins |

**Examples:**
- **Interrupts:** Button press detection, encoder pulses, communication protocols (UART/SPI/I2C)
- **Polling:** Simple UI buttons, slow sensors, single-task applications

## Common Pitfalls and Solutions

### ❌ Problem: Forgot `volatile` keyword
```cpp
bool button_flag = false;  // BAD - compiler may optimise incorrectly
```
**Solution:**
```cpp
volatile bool button_flag = false;  // GOOD - tells compiler variable can change anytime
```

### ❌ Problem: ISR is too slow (>100µs)
```cpp
void button_isr(void) {
    lcd.print("Button pressed!");  // BAD - blocking I/O in ISR
    delay(100);                     // BAD - delay in ISR
}
```
**Solution:**
```cpp
void button_isr(void) {
    button_flag = true;  // GOOD - just set flag, return immediately
}

void loop() {
    if (button_flag) {
        lcd.print("Button pressed!");  // GOOD - complex work in main loop
        button_flag = false;
    }
}
```

### ❌ Problem: Button bouncing causes multiple triggers
```cpp
void button_isr(void) {
    button_flag = true;  // Triggered 10 times from one press!
}
```
**Solution:**
```cpp
void button_isr(void) {
    uint32_t current = micros();
    if ((current - last_time) > DEBOUNCE_US) {  // 50ms lockout
        last_time = current;
        button_flag = true;
    }
}
```

### ❌ Problem: Race condition between ISR and main loop
```cpp
// BAD - 32-bit write is not atomic on 8-bit AVR
volatile uint32_t timestamp;

void button_isr(void) {
    timestamp = micros();  // 4-byte write - can be interrupted mid-write!
}
```
**Solution:**
```cpp
// GOOD - Use critical section for multi-byte variables
uint32_t get_timestamp(void) {
    uint32_t t;
    noInterrupts();  // Disable interrupts
    t = timestamp;   // Atomic read
    interrupts();    // Re-enable interrupts
    return t;
}
```

**Note:** This game uses 8-bit flags (`bool`) and reads timestamps in ISR context only, avoiding race conditions.

## EEPROM Persistence

Best reaction time is saved to EEPROM with validation:

```
Address 0: Magic byte (0xA5) - validates EEPROM has been initialised
Address 1: Best time high byte
Address 2: Best time low byte
Address 3: Checksum (magic + high + low)
```

On boot:
1. Check magic byte - if not 0xA5, no valid data
2. Read 16-bit best time
3. Verify checksum - if mismatch, corrupted data
4. Display best time on attract screen

## Memory Usage

```
RAM:   TBD bytes / 2048 bytes (TBD%)
Flash: TBD bytes / 32256 bytes (TBD%)
```

(Compile to get actual values)

## Extending the Game

### Ideas for Enhancements

1. **Multi-player mode** - Use pin 3 (INT1) for second button
2. **Difficulty levels** - Shorter distractor flashes, more distractors
3. **Online leaderboard** - Send best times via serial/WiFi
4. **Reaction training** - Progressive difficulty, track improvement over time
5. **Sound effects** - Different tones for each feedback tier
6. **RGB LEDs** - Colour-coded distractors (red=distractor, green=draw)

### Tutorial Extensions

7. **Pin Change Interrupts** - Use PCINT for buttons on any pin
8. **Timer Interrupts** - Use Timer1 for periodic events
9. **External Interrupts** - Wake from sleep on button press

## Learning Resources

### Interrupt Concepts
- [AVR Interrupt Guide](https://www.nongnu.org/avr-libc/user-manual/group__avr__interrupts.html)
- [Arduino attachInterrupt() Reference](https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/)
- [Volatile Keyword Explained](https://www.arduino.cc/reference/en/language/variables/variable-scope-qualifiers/volatile/)

### Advanced Topics
- [AVR Hardware Interrupts (INT0/INT1)](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf) (Datasheet section 12)
- [Pin Change Interrupts (PCINT)](https://www.gammon.com.au/interrupts)
- [Timer Interrupts](https://www.instructables.com/Arduino-Timer-Interrupts/)

## License

MIT License - Free for educational and commercial use

## Credits

Part of the **Embedded Games Tutorial Series**
- **Game 1:** Light Chaser (State Machines, Non-blocking Timing)
- **Game 2:** Quick Draw (Hardware Interrupts) ← You are here
- **Game 3:** TBD

Created for teaching embedded systems concepts through hands-on gameplay.

---

**Next Steps:**
1. Build and play the complete game (`game/`)
2. Study the ISR implementation in `hardware.cpp`
3. Work through tutorials 01-06 to understand the progression
4. Modify the code - add features, change timings, experiment!
5. Move on to Game 3 for the next embedded concept

Have fun and happy coding! ⚡
