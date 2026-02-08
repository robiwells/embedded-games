# Tutorial 10: Complete Production Game

## What You'll Learn
- Watchdog timer as safety mechanism
- Production embedded system integration
- Complete architecture review
- Memory analysis and optimisation
- Professional embedded patterns synthesis
- Building reliable, unattended systems

## Prerequisites
- Tutorials 1-9 (all concepts used here)

## Hardware Components
- Arduino Uno
- 8× LEDs (pins 2-9)
- 8× 220Ω resistors
- 1× Pushbutton (pin 10)
- 1× Piezo buzzer (pin 11)
- 1× I2C LCD display 16×2 (pins A4/A5)

## Code Overview
This is the **complete, production-ready** Light Chaser game. It integrates every concept from Tutorials 1-9:

- Tutorial 3: LED chase animation
- Tutorial 4: Non-blocking timing
- Tutorial 5: Button debouncing
- Tutorial 6: State machine
- Tutorial 7: Enter/update/exit lifecycle
- Tutorial 8: LCD display + HAL pattern
- Tutorial 9: Animation system + EEPROM

**New in Tutorial 10:**
- Watchdog timer (WDT) for automatic crash recovery
- Final integration and validation
- Production-ready reliability

## File Structure
```
10-complete-game/
├── diagram.json
├── platformio.ini
├── wokwi.toml
├── include/
│   ├── config.h         # All constants
│   ├── game.h           # State machine interface
│   └── hardware.h       # HAL interface
└── src/
    ├── main.cpp         # Adds watchdog timer
    ├── game.cpp         # Complete 5-state machine
    └── hardware.cpp     # Complete HAL + animations
```

## System Architecture

### Three-Layer Architecture

```
┌─────────────────────────────────────────┐
│           main.cpp (Entry Point)        │
│  - setup(): hardware_init, game_init    │
│  - loop(): game_update, wdt_reset       │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│         game.cpp (Game Logic)           │
│  - 5-state machine (ATTRACT, PLAYING,   │
│    RESULT, CELEBRATION, GAME_OVER)      │
│  - Enter/update/exit lifecycle          │
│  - Score tracking, state transitions    │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│      hardware.cpp (HAL + Drivers)       │
│  - GPIO (LEDs, button)                  │
│  - I2C (LCD display)                    │
│  - PWM (buzzer)                         │
│  - EEPROM (persistence)                 │
│  - Animation state machine              │
└─────────────────────────────────────────┘
```

### Two Concurrent State Machines

**Game State Machine (game.cpp):**
```
ATTRACT → PLAYING → RESULT → PLAYING → ...
                      ↓
              CELEBRATION or GAME_OVER
                      ↓
                  ATTRACT
```

**Animation State Machine (hardware.cpp):**
```
IDLE → BULLSEYE → IDLE
IDLE → CELEBRATION → IDLE
IDLE → GAME_OVER → IDLE
```

**Key insight:** Two state machines run every loop iteration, completely independently.

## Watchdog Timer Deep Dive

### What is a Watchdog Timer?

A watchdog timer (WDT) is a **hardware timer** that automatically resets the microcontroller if software hangs.

**Analogy:** Guard dog that attacks unless you pet it every 4 seconds. If you can't pet it (code crashed), dog resets system.

**How it works:**

1. Enable WDT with timeout (we use 4 seconds)
2. WDT counts down from 4000ms to 0
3. Code must call `wdt_reset()` before timer reaches 0
4. If timer expires → Arduino automatically resets (like pressing reset button)
5. `wdt_reset()` reloads timer to 4000ms

**Analogy with hardware:**
```
Timer: 4000 → 3999 → 3998 → ... → 1 → 0 → RESET!
                                    ↑
                            wdt_reset() prevents this
```

### Why Watchdog Timers?

Embedded systems often run **unattended** (in products, installations). If software crashes, no human available to press reset button.

**Failure scenarios WDT recovers from:**

1. **Infinite loop:**
   ```cpp
   while(1) {}  // Forgot break condition
   // wdt_reset() never called → WDT expires → Reset!
   ```

2. **Blocking code:**
   ```cpp
   delay(10000);  // 10 seconds > 4 second WDT timeout
   // WDT expires during delay → Reset!
   ```

3. **I2C hang:**
   ```cpp
   lcd.print(...);  // I2C device stops responding
   // Code stuck waiting → WDT expires → Reset!
   ```

4. **Memory corruption:**
   ```cpp
   // Bug causes stack/heap corruption
   // Code crashes, hangs, or infinite loops
   // WDT expires → Reset!
   ```

**Without WDT:** System stays hung forever (needs human intervention)
**With WDT:** System automatically recovers in 4 seconds

### Our WDT Configuration

```cpp
void setup() {
    hardware_init();
    game_init();
    wdt_enable(WDTO_4S);  // Enable 4-second watchdog
}

void loop() {
    game_update();
    wdt_reset();  // Pet the watchdog
}
```

**Timeout choice (4 seconds):**

Available timeouts on AVR:
- WDTO_15MS, WDTO_30MS, WDTO_60MS, WDTO_120MS
- WDTO_250MS, WDTO_500MS, WDTO_1S, WDTO_2S
- WDTO_4S, WDTO_8S

We chose 4 seconds because:
- Our loop() executes in ~0.3-1ms (very fast)
- Longest operation: EEPROM write (~3.3ms)
- Longest animation: GAME_OVER (~1500ms)
- 4 seconds gives HUGE safety margin (4000ms ÷ 1ms = 4000× headroom!)

**Critical requirement:** Every code path in loop() must complete in < 4 seconds.

### Why Our Code is WDT-Safe

**No blocking code anywhere:**
```cpp
// ❌ BAD (would cause WDT reset):
void loop() {
    game_update();
    delay(5000);  // 5s > 4s WDT timeout → RESET!
    wdt_reset();  // Never reached
}

// ✅ GOOD (our code):
void loop() {
    game_update();  // Returns in ~1ms
    wdt_reset();    // Always reached quickly
}
```

**All timing uses millis(), never delay():**
```cpp
// Non-blocking pattern (safe):
if (millis() - last_update >= 300) {
    do_action();
}
// Returns immediately regardless of timing
```

**Animations are non-blocking state machines:**
```cpp
bool animation_update(void) {
    if (time_for_next_step()) {
        advance_animation();
    }
    return true;  // Always returns quickly
}
```

**Execution time analysis:**
```
Typical loop() duration:
  animation_update():     ~50-100 μs
  game state update():    ~200-500 μs
  wdt_reset():            ~2 μs
  Total:                  ~300-600 μs

Worst case (CELEBRATION):
  animation_update():     ~100 μs
  celebration_update():   ~50 μs
  Total:                  ~150 μs

WDT timeout: 4,000,000 μs (4 seconds)
Our worst case: 150 μs
Safety margin: 26,666× !!!
```

We could run **26,666 loop iterations** before WDT expires!

### WDT Testing

**Test 1: Verify WDT is active**
```cpp
void loop() {
    game_update();
    // wdt_reset();  // Comment this out
}
// Result: Arduino resets every 4 seconds
```

**Test 2: Trigger WDT with delay()**
```cpp
void loop() {
    game_update();
    delay(5000);  // Force WDT timeout
    wdt_reset();
}
// Result: Arduino resets after 4 seconds (before delay completes)
```

**Test 3: Verify reset recovery**
```cpp
// WDT reset counter survives reboot
// Can detect if last reset was caused by WDT
uint8_t mcusr = MCUSR;
if (mcusr & (1 << WDRF)) {
    // Last reset was watchdog timer
    Serial.println("WDT RESET!");
}
MCUSR = 0;  // Clear flags
```

## Complete System Integration

### Game Flow

**Complete state diagram:**
```
            ┌──────────────┐
            │   ATTRACT    │ ← Demo mode, waiting
            │ "Press Play" │
            └───────┬──────┘
                    │ Button press
            ┌───────▼──────┐
            │   PLAYING    │ ← Active game
            │ Score: 45    │
            └───┬──────┬───┘
                │      │
         Hit    │      │ Miss + new high
                │      │
        ┌───────▼──┐   │
        │  RESULT  │   │
        │ 300ms    │   │
        │ pause    │   │
        └─────┬────┘   │
              │        │
         ┌────▼────┐   │
         │ PLAYING │   │
         └────┬────┘   │
              │        │
         Miss │        │
              │   ┌────▼────────┐
              │   │ CELEBRATION │ Miss + no high
              │   │ 2 seconds   │
              │   └─────┬───────┘
              │         │
         ┌────▼─────────▼──┐
         │   GAME_OVER     │
         │ Animation done  │
         └────────┬────────┘
                  │
            ┌─────▼──────┐
            │  ATTRACT   │
            └────────────┘
```

### Animation Orchestration

**Parallel animations during CELEBRATION:**
```
Timeline (milliseconds):

Time:     0     150   300   450   600   750   900
Buzzer:   C5    E5    G5    C6    ────E6────  (done)
Notes:    └150┘ └150┘ └150┘ └150┘ └───300───┘

Time:     0  40 80 120 160 200 240 280 320 360...960
LEDs:     0  1  2  3   4   5   6   7   0   1  ...done
Sweep:    └──────Sweep 1─────────┘ └──Sweep 2──┘...

Both complete → return to ATTRACT
```

**Key pattern:** Independent timing variables
```cpp
static uint32_t anim_last_update = 0;  // Buzzer
static uint32_t led_last_update = 0;   // LEDs

// In animation_update():
if (now - anim_last_update >= note_duration) { /* buzzer */ }
if (now - led_last_update >= led_delay) { /* LEDs */ }
```

### Memory Map

**Flash (Program Memory) - 32 KB:**
```
┌──────────────────┬───────┐
│ Program code     │ 8 KB  │ ← game.cpp, hardware.cpp, main.cpp
├──────────────────┼───────┤
│ Libraries        │ 3 KB  │ ← LCD, EEPROM, Arduino core
├──────────────────┼───────┤
│ String constants │ 0.5KB │ ← "Press to Play!", etc.
├──────────────────┼───────┤
│ Available        │ 20KB  │ ← Plenty of room!
└──────────────────┴───────┘
```

**RAM (Data Memory) - 2 KB:**
```
┌──────────────────┬────────┐
│ Stack            │ ~200B  │ ← Function call stack
├──────────────────┼────────┤
│ Static variables │ ~420B  │ ← Game state, animation vars
├──────────────────┼────────┤
│ LCD buffer       │ ~50B   │ ← LiquidCrystal_I2C
├──────────────────┼────────┤
│ Available        │ ~1400B │ ← 68% free!
└──────────────────┴────────┘
```

**EEPROM (Persistent) - 1 KB:**
```
┌──────────────────┬───────┐
│ High score data  │ 4 B   │ ← Used
├──────────────────┼───────┤
│ Available        │ 1020B │ ← Could store settings, stats
└──────────────────┴───────┘
```

**Memory usage summary:**
- Flash: 24.9% (8026/32256 bytes)
- RAM: 20.6% (421/2048 bytes)
- EEPROM: 0.4% (4/1024 bytes)

**Plenty of headroom for:**
- More game modes
- Sound effects library
- Animation sequences
- Player statistics
- Multiple high scores

## Performance Analysis

**Loop timing breakdown:**
```
Function                Time (μs)   % of loop
──────────────────────────────────────────────
button_just_pressed()   ~50         ~17%
update_chase_position() ~100        ~33%
animation_update()      ~80         ~27%
State update()          ~50         ~17%
wdt_reset()            ~2          <1%
──────────────────────────────────────────────
Total per loop:         ~280μs      100%

Loop frequency:         ~3500 Hz
Frame time:            ~0.28ms
```

**Comparison to other systems:**
- Desktop game (60 FPS): 16.67ms per frame
- Our game: 0.28ms per frame
- **We're 59× faster!** (because we have less to compute)

**Responsiveness:**
- Button latency: < 1ms (checked every loop)
- Display update: ~3-4ms (I2C communication)
- Sound feedback: < 1ms (tone() returns immediately)
- State transition: < 0.1ms

**This is why the game feels so responsive!**

## Production Deployment Considerations

### Reliability Features

1. **Watchdog timer:** Auto-recovery from crashes
2. **EEPROM validation:** Magic byte + checksum prevent corruption
3. **Button debouncing:** Eliminates false presses
4. **Non-blocking code:** System always responsive
5. **State machine:** Clear, predictable behaviour

### Power Considerations

**Current draw (estimated):**
- Arduino idle: ~50mA
- 1 LED on: +20mA
- LCD backlight: +60mA
- Total: ~130mA @ 5V = 0.65W

**For battery operation:**
- 9V battery (500mAh): ~3-4 hours runtime
- 4× AA batteries (2000mAh): ~12-15 hours runtime
- USB power bank (10000mAh): ~70+ hours runtime

**Power optimisation opportunities:**
- Turn off LCD backlight when idle (save 60mA)
- Sleep mode in ATTRACT state (save 30-40mA)
- PWM LED dimming (save 10-15mA per LED)

### Failure Modes

**Handled automatically:**
- Software crash → WDT reset
- EEPROM corruption → Return default high score (0)
- Button bounce → Debouncing filters
- Uninitialised EEPROM → Magic byte check returns 0

**Not handled (require hardware intervention):**
- Power loss → Expected (EEPROM preserved)
- LCD disconnected → Code continues, display blank
- Button stuck pressed → Game won't start (by design)
- LED burnout → Visual only, game still playable

## How to Run

**Local with PlatformIO:**
```bash
cd 10-complete-game
pio run --target upload
```

**Online with Wokwi:**
1. Visit https://wokwi.com/
2. Upload all files
3. Click "Start Simulation"

## Complete Gameplay

1. **Power on:** High score loads from EEPROM (0 if first boot)
2. **Attract mode:** LED bounces, display shows "Press to Play!"
3. **Press button:** Game starts, score resets to 0
4. **Playing:** Hit green LEDs (positions 3-4) for 10 points each
5. **Hit:** Bullseye melody plays, score updates, 300ms pause, continue
6. **Game speeds up:** Each hit decreases LED speed by 10ms (200→190→180...)
7. **Miss (new high):** Save to EEPROM, celebration animation, return to attract
8. **Miss (no high):** Game over animation, return to attract
9. **Power cycle:** High score preserved!

## Exercises

### Exercise 1: Test WDT Recovery
**Task:** Verify watchdog timer works.

**Steps:**
1. Add infinite loop to playing_update():
   ```cpp
   if (current_score == 20) {
       while(1) {}  // Deliberate hang
   }
   ```
2. Play until score = 20
3. Observe: Arduino resets after 4 seconds
4. Game resumes in ATTRACT (high score preserved!)

### Exercise 2: Measure Loop Performance
**Task:** Profile loop() execution time.

**Steps:**
1. Add timing to main.cpp:
   ```cpp
   void loop() {
       uint32_t start = micros();
       game_update();
       uint32_t elapsed = micros() - start;

       static uint32_t max_time = 0;
       if (elapsed > max_time) {
           max_time = elapsed;
           // Blink LED13 when new max found
       }
       wdt_reset();
   }
   ```
2. Play complete game
3. Check max_time (should be < 1000μs)

### Exercise 3: Add Statistics Tracking
**Task:** Track total games played.

**Steps:**
1. Add to EEPROM (addresses 4-7):
   ```cpp
   uint32_t games_played;
   ```
2. Increment in attract_exit()
3. Write to EEPROM on power-down (or periodically)
4. Display in attract mode

### Exercise 4: Create Difficulty Levels
**Task:** Add Easy/Normal/Hard modes.

**Steps:**
1. Add to config.h:
   ```cpp
   enum Difficulty { EASY, NORMAL, HARD };
   ```
2. Store in EEPROM
3. Adjust INITIAL_CHASE_SPEED, MIN_CHASE_SPEED, TARGET_ZONE
4. Cycle difficulty with long button press in ATTRACT

### Exercise 5: Implement Power Saving
**Task:** Add sleep mode to save battery.

**Steps:**
1. After 60s in ATTRACT with no button press:
   ```cpp
   lcd.noBacklight();
   // Could add sleep_mode() for more savings
   ```
2. Wake on button press
3. Measure current reduction (should save ~60mA)

## Key Takeaways

1. **Watchdog timer is essential** for unattended embedded systems - provides automatic recovery

2. **Non-blocking architecture enables WDT** - every path through loop() must be fast

3. **Three-layer architecture scales** - main/game/hardware separation keeps code organised

4. **Two concurrent state machines** - game and animation run independently every frame

5. **EEPROM for persistence** - data survives power loss, firmware updates

6. **Memory efficiency matters** - static allocation, no malloc, careful with RAM

7. **Cooperative multitasking works** - multiple tasks run "simultaneously" by yielding quickly

8. **HAL pattern pays off** - hardware changes don't affect game logic

9. **State machines prevent bugs** - clear states, defined transitions, no spaghetti code

10. **Production code is deliberate** - every design choice has reasoning behind it

## Journey Complete

**Tutorials 1-9 built up to this:**

- T1-2: Project setup, basic wiring
- T3: LED animation fundamentals
- T4: Non-blocking timing (critical for WDT!)
- T5: Button debouncing (reliability)
- T6: State machine basics
- T7: Enter/update/exit pattern (organisation)
- T8: I2C, LCD, HAL pattern (architecture)
- T9: Animation system, EEPROM (complexity)
- **T10: Integration + watchdog (production)**

**You now have:**
- Complete, production-ready embedded game
- Professional embedded architecture patterns
- Experience with core embedded concepts
- Foundation for more complex projects

## Next Steps

**Expand this game:**
- Multiple difficulty levels
- Sound effects library
- Statistics tracking
- Tournament mode (timed rounds)
- Multiplayer (2 buttons, 2 scores)

**New projects using these patterns:**
- Reaction time tester
- Memory game (Simon Says)
- Maze navigation game
- Sensor data logger
- Home automation controller

**Advanced topics:**
- Interrupts for ultra-low latency input
- DMA for efficient data transfer
- Power management modes
- Real-time operating system (RTOS)
- Wireless communication (RF, Bluetooth)

## Conclusion

This complete game demonstrates professional embedded systems engineering:

✅ **Reliable:** Watchdog timer, validation, debouncing
✅ **Responsive:** Non-blocking, < 1ms latency
✅ **Maintainable:** HAL, state machines, clear architecture
✅ **Efficient:** 20% RAM, 25% Flash, plenty of headroom
✅ **Persistent:** EEPROM storage survives power loss
✅ **Scalable:** Easy to add features, modes, animations

**This is production-quality embedded firmware.**

Congratulations on completing the Light Chaser series!
