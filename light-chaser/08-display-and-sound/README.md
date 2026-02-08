# Tutorial 8: Display and Sound

## What You'll Learn
- I2C communication protocol (2-wire serial)
- Controlling 16×2 LCD character displays
- PWM-based sound generation with piezo buzzers
- Hardware Abstraction Layer (HAL) pattern
- Blocking vs non-blocking code (and why delay() is sometimes acceptable)
- Multi-sensory feedback for better user experience

## Prerequisites
- Tutorial 7: State Lifecycle Pattern (enter/update/exit states)

## Hardware Components
- Arduino Uno
- 8× LEDs (pins 2-9)
- 8× 220Ω resistors
- 1× Pushbutton (pin 10)
- 1× Piezo buzzer (pin 11)
- 1× I2C LCD display 16×2 (pins A4/A5)

## Code Overview
This tutorial adds visual feedback (LCD display) and audio feedback (buzzer melodies) to the game. We introduce the **Hardware Abstraction Layer (HAL)** pattern - a clean interface that hides implementation details from game logic.

**Key additions:**
- LCD display shows score, high score, and game messages
- Buzzer plays different sounds for different events
- HAL pattern separates hardware control from game logic
- Still uses 4 states (CELEBRATION comes in Tutorial 9)

**Note:** This tutorial uses `delay()` in buzzer functions for simplicity. Tutorial 9 will refactor to non-blocking animations.

## File Structure
```
08-display-and-sound/
├── diagram.json          # Wokwi circuit (adds LCD + buzzer)
├── platformio.ini        # Build config (adds LCD library)
├── wokwi.toml           # Wokwi simulator settings
├── include/
│   ├── config.h         # Hardware pins + game constants
│   ├── game.h           # Game state machine interface
│   └── hardware.h       # HAL interface (NEW!)
└── src/
    ├── main.cpp         # Entry point (setup/loop)
    ├── game.cpp         # State machine logic
    └── hardware.cpp     # HAL implementation (NEW!)
```

## Key Concepts

### Hardware Abstraction Layer (HAL)

**What is HAL?**

A Hardware Abstraction Layer provides a clean interface between hardware and application logic. Game code calls functions like `display_show_game(score)` without knowing how the LCD works internally.

**Architecture:**
```
┌──────────────┐
│  game.cpp    │  High-level logic (scoring, states)
│              │  Calls: display_show_game(), buzzer_bullseye()
└──────┬───────┘
       │
       │  HAL Interface (hardware.h)
       │  Function declarations only
       │
┌──────▼───────┐
│ hardware.cpp │  Implementation (I2C, GPIO, PWM)
└──────┬───────┘
       │
┌──────▼───────┐
│  Physical    │  LCD, buzzer, LEDs
│  Hardware    │
└──────────────┘
```

**Benefits:**

1. **Portability:** Change LCD from I2C to parallel? Only edit hardware.cpp
2. **Testability:** Mock hardware functions for desktop testing
3. **Readability:** `display_show_attract(score)` vs `lcd.setCursor(0,0); lcd.print(...)`
4. **Maintainability:** Pin changes don't affect game logic

**Example HAL functions:**
```cpp
void display_show_game(uint16_t score, uint16_t high_score);
void buzzer_bullseye(void);
void led_set(uint8_t position, bool state);
```

Game code stays simple:
```cpp
if (button_just_pressed()) {
    display_show_game(score, high_score);
    buzzer_hit();
}
```

### I2C Communication Protocol

**What is I2C?**

I2C (Inter-Integrated Circuit, pronounced "eye-squared-C") is a 2-wire serial communication protocol for connecting peripherals to microcontrollers.

**I2C Architecture:**
```
        Arduino Uno (Master)
             │
             ├─ A4 (SDA) ──┬──► LCD (Slave 0x27)
             │             │
             ├─ A5 (SCL) ──┘
             │
             └─ GND ─────────► Common ground

Pull-up resistors (4.7kΩ) on SDA/SCL lines
(Usually built into LCD module)
```

**Two wires:**
- **SDA (Serial Data):** Bidirectional data line
- **SCL (Serial Clock):** Clock signal from master (Arduino)

**Key concepts:**

1. **Master-Slave:** Arduino initiates all communication
2. **Addressing:** Each I2C device has unique 7-bit address (0x00-0x7F)
3. **Multi-device:** Multiple slaves can share same 2 wires
4. **Pull-ups required:** Both lines need pull-up resistors to 5V

**Common I2C addresses:**
- LCD displays: 0x27 or 0x3F
- EEPROM: 0x50-0x57
- RTC: 0x68
- Sensors: Various

**I2C vs Parallel LCD:**

| Feature | I2C LCD | Parallel LCD |
|---------|---------|--------------|
| Pins | 2 (SDA/SCL) | 6+ (RS/E/D4-D7) |
| Speed | ~3-4ms full screen | ~100μs per char |
| Complexity | Library handles | Manual timing |
| Use case | Pin-constrained | Speed-critical |

**For our game:** Pin savings >> speed. We only update display on state changes, so 3ms delay is fine.

### LCD Display Control

**16×2 Character LCD:**

```
Column: 0123456789ABCDEF
Row 0:  [Press to Play!]
Row 1:  [HiScore: 100   ]
```

**Basic operations:**
```cpp
lcd.init();              // Initialise LCD
lcd.backlight();         // Turn on backlight
lcd.clear();            // Clear display
lcd.setCursor(col, row); // Position cursor
lcd.print("text");       // Print string/number
```

**Display layouts in our game:**

**Attract mode:**
```
┌────────────────┐
│Press to Play!  │
│HiScore: 100    │
└────────────────┘
```

**Playing:**
```
┌────────────────┐
│Score:   45     │
│HiScore: 100    │
└────────────────┘
```

**Flicker reduction:**

When updating scores, we DON'T clear the whole screen (causes flicker):

Bad (flickers):
```cpp
lcd.clear();           // Screen goes blank
lcd.print("Score: ");
lcd.print(score);      // Text reappears
```

Good (smooth):
```cpp
lcd.setCursor(8, 0);   // Jump to number position
lcd.print(score);
lcd.print("    ");     // Clear trailing digits
```

### PWM Sound Generation

**How buzzer makes sound:**

Arduino's `tone()` function generates square waves using PWM (Pulse Width Modulation):

```
HIGH (5V) ─┐     ┌─┐     ┌─┐
           │     │ │     │ │
LOW (0V)   └─────┘ └─────┘ └─
           <─────>
           Period = 1/frequency

Example: 1000 Hz tone
Period = 1/1000s = 1ms
Pin toggles HIGH/LOW 1000 times per second
```

**Buzzer membrane vibrates at this frequency → audible tone**

**Musical notes (Hz):**
- C5: 523 Hz
- E5: 659 Hz
- G5: 784 Hz
- C6: 1047 Hz

**Our sound effects:**
```cpp
buzzer_tick()      // 100 Hz, 20ms - LED movement
buzzer_hit()       // 500 Hz, 100ms - Regular hit
buzzer_bullseye()  // 800→1000→1200 Hz - Bullseye hit
buzzer_game_over() // 400→300→200 Hz - Game over
```

**Non-blocking nature:**

`tone()` returns immediately - sound plays in background using hardware timers:
```cpp
tone(pin, 500, 100);  // Start 500 Hz for 100ms
// Execution continues immediately
led_set(0, true);     // Can do other work
```

**Blocking delay() for multi-note sequences:**

In this tutorial, we use `delay()` for multi-note melodies:
```cpp
void buzzer_bullseye(void) {
    tone(BUZZER_PIN, 800, 100);
    delay(100);  // ⚠️ Blocks for 100ms
    tone(BUZZER_PIN, 1000, 100);
    delay(100);  // ⚠️ Blocks for 100ms
    tone(BUZZER_PIN, 1200, 100);
}
```

**Why delay() here is acceptable:**
- Only happens during brief result pause (300ms)
- Game not accepting input during this time
- Total blocking time: 200-600ms (acceptable)
- Tutorial 9 will show non-blocking alternative

**Why delay() is normally bad:**
- Freezes entire system
- Can't check button
- Can't update display
- Watchdog timer can expire

### Button Debouncing

**Mechanical bounce problem:**

Physical buttons "bounce" - contacts make/break rapidly for 5-20ms:

```
Voltage
 5V ┤ ──┐   ┌──┐ ┌─┐ ┌────────  (Released = HIGH)
    │   └───┘  └─┘ └─┘          (Pressed = LOW)
 0V ┤
    └──────────────────────────► Time
                └─┬─┘
              5-20ms bounce period
```

**Without debouncing:** One press registers as 3-5 presses

**Solution: Time-based debouncing**

After detecting edge, ignore transitions for 50ms:
```cpp
if (edge_detected && (now - last_press >= 50ms)) {
    // Valid press
    last_press = now;
}
```

**Implementation in hardware.cpp:**
```cpp
bool button_just_pressed(void) {
    uint32_t now = millis();
    bool current_state = !digitalRead(BUTTON_PIN);

    bool pressed = false;
    if (current_state && !last_button_state) {
        if (now - last_debounce_time >= DEBOUNCE_MS) {
            pressed = true;
            last_debounce_time = now;
        }
    }

    last_button_state = current_state;
    return pressed;
}
```

### Game State Flow (4 States)

```
ATTRACT
   ↓ Button press
PLAYING
   ↓ Hit target
RESULT (300ms pause)
   ↓
PLAYING (continue)
   ↓ Miss target
GAME_OVER
   ↓ Wait 2s
ATTRACT
```

**State responsibilities:**

**ATTRACT:**
- Display: "Press to Play!" + high score
- LED: Bouncing chase animation
- Wait for button

**PLAYING:**
- Display: Current score + high score
- LED: Bouncing chase (speed increases)
- Check button press → score or game over

**RESULT:**
- Display: Updated score
- LED: Frozen at hit position
- Sound: Bullseye melody or hit beep
- Wait 300ms → return to PLAYING

**GAME_OVER:**
- Display: Final score (from PLAYING)
- LED: Clear (no chase)
- Sound: Descending "sad trombone"
- Wait 2s → return to ATTRACT

## How to Run

**Local with PlatformIO:**
```bash
cd 08-display-and-sound
pio run
pio run --target upload
```

**Online with Wokwi:**
1. Visit: https://wokwi.com/
2. Upload all files from this directory
3. Click "Start Simulation"

## Gameplay Instructions

1. **Attract Mode:** Display shows "Press to Play!" and high score. LED bounces.
2. **Press button** to start game.
3. **Playing:** Display shows current score. LED bounces faster as you score.
4. **Press button** when LED is on green LEDs (positions 3-4) to score 10 points.
5. **Hit:** Hear melody, score updates, brief pause, continue playing.
6. **Miss:** Hit red LED → Game over sound, wait 2s, return to attract.

## Exercises

### Exercise 1: Understand HAL Benefits
**Task:** Change LED wiring from pins 2-9 to pins 3-10.

**Steps:**
1. Only edit `config.h`: Change `LED_PIN_START` from 2 to 3
2. No changes needed to game.cpp!
3. This demonstrates HAL portability

**Question:** What would you need to change without HAL?
- Answer: Every `digitalWrite(2, ...)` call throughout the code

### Exercise 2: Custom Display Messages
**Task:** Add "GAME OVER" message to game over state.

**Steps:**
1. Add to hardware.h:
   ```cpp
   void display_show_game_over(uint16_t final_score);
   ```

2. Implement in hardware.cpp:
   ```cpp
   void display_show_game_over(uint16_t final_score) {
       lcd.clear();
       lcd.setCursor(0, 0);
       lcd.print("GAME OVER!");
       lcd.setCursor(0, 1);
       lcd.print("Score: ");
       lcd.print(final_score);
   }
   ```

3. Call from game.cpp in `game_over_enter()`:
   ```cpp
   display_show_game_over(current_score);
   ```

### Exercise 3: Try Different I2C Address
**Task:** If LCD doesn't work, try alternate address.

**Steps:**
1. Change in config.h:
   ```cpp
   const uint8_t LCD_ADDRESS = 0x3F;  // Was 0x27
   ```

2. Most I2C LCDs use either 0x27 or 0x3F
3. If neither works, use I2C scanner sketch to find address

### Exercise 4: Create New Sound Effect
**Task:** Add "bonus" sound for hitting bullseye 3 times in a row.

**Steps:**
1. Add to config.h:
   ```cpp
   const uint16_t FREQ_BONUS = 1500;
   const uint16_t DURATION_BONUS = 50;
   ```

2. Add to hardware.h/cpp:
   ```cpp
   void buzzer_bonus(void) {
       for (int i = 0; i < 5; i++) {
           tone(BUZZER_PIN, FREQ_BONUS, DURATION_BONUS);
           delay(DURATION_BONUS);
       }
   }
   ```

3. Track consecutive bullseyes in game.cpp and call when count reaches 3

### Exercise 5: Measure Display Update Time
**Task:** How long does `display_show_game()` take?

**Steps:**
1. Add timing code:
   ```cpp
   uint32_t start = micros();
   display_show_game(current_score, high_score);
   uint32_t elapsed = micros() - start;
   // Print elapsed to Serial or blink LED
   ```

2. Typical result: 3000-4000 microseconds (3-4ms)
3. Compare to Serial.println() timing (much faster)

## Key Takeaways

1. **HAL pattern** separates hardware details from application logic - makes code portable and maintainable

2. **I2C saves pins** - Control full LCD with just 2 wires (vs 6+ for parallel)

3. **PWM generates audio** - Square waves at audio frequencies make piezo buzzers sing

4. **Debouncing is essential** - Mechanical buttons bounce, need 50ms lockout

5. **delay() trade-offs** - Sometimes acceptable for short, non-critical durations (but Tutorial 9 shows better way)

6. **Multi-sensory feedback** - Combining visual (LCD) + audio (buzzer) + tactile (LEDs) creates better UX

7. **LCD flicker reduction** - Don't clear entire screen, just overwrite changed areas

## Coming in Tutorial 9

Tutorial 9 will eliminate `delay()` calls by introducing a non-blocking animation system. We'll implement:
- State machine for animations
- Parallel timing (buzzer + LEDs simultaneously)
- Cooperative multitasking pattern
- Add STATE_CELEBRATION with complex animation

This will prepare us for Tutorial 10's watchdog timer, which cannot tolerate blocking code.

## Memory Usage

Flash: ~6.5 KB (includes LCD library)
RAM: ~350 bytes

Plenty of headroom for animation system in Tutorial 9!
