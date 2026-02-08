# Tutorial 9: Parallel Animations

## What You'll Learn
- Non-blocking animation state machines
- Parallel timing (multiple animations simultaneously)
- Cooperative multitasking pattern
- Eliminating delay() from production code
- EEPROM persistent storage
- Complex multi-sensory feedback

## Prerequisites
- Tutorial 8: Display and Sound (HAL pattern, I2C, PWM sound)

## Hardware Components
- Arduino Uno
- 8× LEDs (pins 2-9)
- 8× 220Ω resistors
- 1× Pushbutton (pin 10)
- 1× Piezo buzzer (pin 11)
- 1× I2C LCD display 16×2 (pins A4/A5)

## Code Overview
This tutorial eliminates all `delay()` calls by implementing a non-blocking animation system. We add:
- Animation state machine (separate from game state machine!)
- Parallel timing for buzzer + LED effects
- STATE_CELEBRATION for new high scores
- EEPROM persistence for high score

**Architecture:**
- Two state machines running simultaneously:
  - Game state machine (ATTRACT, PLAYING, RESULT, CELEBRATION, GAME_OVER)
  - Animation state machine (IDLE, BULLSEYE, CELEBRATION, GAME_OVER)
- Both called every loop iteration
- Cooperative multitasking - each yields control after doing small work

## File Structure
```
09-parallel-animations/
├── diagram.json
├── platformio.ini
├── wokwi.toml
├── include/
│   ├── config.h         # Adds EEPROM + celebration constants
│   ├── game.h           # Same interface
│   └── hardware.h       # Adds animation functions
└── src/
    ├── main.cpp         # Unchanged
    ├── game.cpp         # Adds STATE_CELEBRATION, uses animation_start_*()
    └── hardware.cpp     # Complete animation system, EEPROM functions
```

## Key Concepts

### The Problem with delay()

**Tutorial 8 approach (blocking):**
```cpp
void buzzer_bullseye(void) {
    tone(BUZZER_PIN, 800, 100);
    delay(100);  // ❌ FREEZES SYSTEM for 100ms
    tone(BUZZER_PIN, 1000, 100);
    delay(100);  // ❌ Can't check button, update display
    tone(BUZZER_PIN, 1200, 100);
}
```

**Problems:**
- Entire system frozen during melody (300ms)
- Can't respond to button presses
- Can't update display
- Watchdog timer will expire with longer delays
- No other code can run

**Tutorial 9 approach (non-blocking):**
```cpp
void animation_start_bullseye(void) {
    anim_state = ANIM_BULLSEYE;
    anim_step = 0;
    anim_last_update = millis();
    // Returns immediately!
}

bool animation_update(void) {
    uint32_t now = millis();

    if (now - anim_last_update >= note_duration) {
        play_next_note();
        anim_step++;
    }
    // Returns immediately regardless of timing
}
```

**Benefits:**
- Main loop continues running
- Can check button every iteration
- Can update display any time
- Watchdog timer stays fed
- System stays responsive

### Cooperative Multitasking

**What is it?**

Arduino has no operating system, no threads, no scheduler. Everything runs in a single execution thread. To do multiple things "at once", we use **cooperative multitasking**: each task runs a bit, then yields control.

**Analogy:**

**Preemptive multitasking (desktop OS):**
```
OS: "Task A, you get 10ms. Go!"
  Task A runs for 10ms
OS: *interrupts* "Time's up! Task B, your turn!"
  Task B runs for 10ms
OS: *interrupts* "Task C, you're up!"
```

**Cooperative multitasking (Arduino):**
```
loop() {
    task_a_update();  // Runs a bit, returns voluntarily
    task_b_update();  // Runs a bit, returns voluntarily
    task_c_update();  // Runs a bit, returns voluntarily
}
```

**Key difference:** Tasks must cooperate (return quickly). If task_a() runs delay(5000), all other tasks starve.

**Our implementation:**
```cpp
void loop() {
    game_update();      // Task 1: Game logic
      ↳ animation_update();  // Task 2: Animations
      ↳ update_chase();      // Task 3: LED chase
      ↳ button_check();      // Task 4: Input
    // All return quickly (~1ms total)
}
```

**Timeline:**
```
Time:    0ms    1ms    2ms    3ms    4ms    5ms
loop():  │      │      │      │      │      │
         ▼      ▼      ▼      ▼      ▼      ▼
game     run    run    run    run    run    run
anim      └─run  └─run  └─run  └─run  └─run  └─run
chase      └run  └run  └run  └run  └run  └run
button      └run  └run  └run  └run  └run  └run

Each function runs for ~0.1-0.5ms then returns
Loop iterates 1000-20000 times/second
```

### Animation State Machine

**Separate state machine for animations:**

Game state machine controls game flow:
- ATTRACT → PLAYING → RESULT → PLAYING → GAME_OVER → ATTRACT

Animation state machine controls sound/light effects:
- IDLE → BULLSEYE → IDLE
- IDLE → CELEBRATION → IDLE
- IDLE → GAME_OVER → IDLE

**Key insight:** Two independent state machines running in parallel!

**Animation states:**
```cpp
enum AnimationState {
    ANIM_IDLE,         // No animation playing
    ANIM_BULLSEYE,     // 3-note ascending
    ANIM_CELEBRATION,  // 5-note melody + LED wave
    ANIM_GAME_OVER     // 3-note descending + LED flash
};
```

**State variables:**
```cpp
static AnimationState anim_state = ANIM_IDLE;
static uint8_t anim_step = 0;           // Which note/step
static uint32_t anim_last_update = 0;   // Last buzzer update
static uint32_t led_last_update = 0;    // Last LED update (parallel!)
```

**Pattern:**

1. **Start animation:** Set state, reset counters, record timestamp
   ```cpp
   void animation_start_bullseye(void) {
       anim_state = ANIM_BULLSEYE;
       anim_step = 0;
       anim_last_update = millis();
   }
   ```

2. **Update animation:** Called every loop(), advances if enough time elapsed
   ```cpp
   bool animation_update(void) {
       if (anim_state == ANIM_IDLE) return true;

       uint32_t now = millis();

       switch (anim_state) {
           case ANIM_BULLSEYE:
               if (now - anim_last_update >= duration) {
                   play_note(anim_step);
                   anim_step++;
                   if (anim_step >= 3) {
                       anim_state = ANIM_IDLE;
                       return true;  // Complete
                   }
               }
               break;
       }
       return false;  // Still playing
   }
   ```

3. **Check completion:** Game logic can query status
   ```cpp
   if (!animation_is_playing()) {
       game_transition_to(NEXT_STATE);
   }
   ```

### Parallel Timing

**Challenge:** Run buzzer melody AND LED animation simultaneously with independent timing.

**Example: CELEBRATION animation**

**Buzzer sequence:**
- Note 0: C5 (523 Hz) for 150ms
- Note 1: E5 (659 Hz) for 150ms
- Note 2: G5 (784 Hz) for 150ms
- Note 3: C6 (1047 Hz) for 150ms
- Note 4: E6 (1319 Hz) for 300ms
- Total: ~900ms

**LED sequence:**
- Light LEDs 0→1→2→3→4→5→6→7, repeat 3 times
- Each LED lit for 40ms
- 3 sweeps × 8 LEDs × 40ms = 960ms

**Different timings!** Buzzer changes every 150-300ms, LED every 40ms.

**Solution: Independent timing variables**
```cpp
static uint32_t anim_last_update = 0;  // Buzzer timing
static uint32_t led_last_update = 0;   // LED timing
```

**Implementation:**
```cpp
case ANIM_CELEBRATION:
    // Buzzer sequence (150ms intervals)
    if (now - anim_last_update >= 150) {
        play_note(anim_step);
        anim_last_update = now;
        anim_step++;
    }

    // LED sequence (40ms intervals) - PARALLEL!
    if (now - led_last_update >= 40) {
        advance_led();
        led_last_update = now;
    }

    // Both complete?
    if (notes_done && leds_done) {
        anim_state = ANIM_IDLE;
    }
    break;
```

**Timeline:**
```
Time:     0ms   50ms  100ms 150ms 200ms 250ms 300ms
Buzzer:   C5    (C5)  (C5)  E5    (E5)  (E5)  G5
          └──────150ms──────┘└──────150ms──────┘

LEDs:     [0]   [1]   [2]   [3]   [4]   [5]   [6]
          └40ms┘└40ms┘└40ms┘└40ms┘└40ms┘└40ms┘

Notice: Buzzer updates at 0, 150, 300, ...
        LEDs update at 0, 40, 80, 120, 160, ...
        Completely independent!
```

**Key pattern:** Two timing checks in one update function
```cpp
if (now - buzzer_timer >= buzzer_interval) { /* buzzer work */ }
if (now - led_timer >= led_interval) { /* LED work */ }
```

### EEPROM Persistent Storage

**What is EEPROM?**

Electrically Erasable Programmable Read-Only Memory - non-volatile storage that survives power loss.

**Arduino Uno EEPROM:**
- Size: 1 KB (1024 bytes)
- Write speed: ~3.3ms per byte
- Endurance: ~100,000 write cycles per byte
- Contents: Random garbage on first boot
- Survives: Power cycles, resets, firmware updates

**Comparison to RAM/Flash:**
```
┌────────┬──────┬─────────┬─────────────┬───────────┐
│ Type   │ Size │ Speed   │ Persistence │ Endurance │
├────────┼──────┼─────────┼─────────────┼───────────┤
│ RAM    │ 2KB  │ 62.5ns  │ Lost on RST │ Unlimited │
│ Flash  │ 32KB │ Fast(R) │ Permanent   │ 10K writes│
│ EEPROM │ 1KB  │ 3.3ms   │ Permanent   │ 100K write│
└────────┴──────┴─────────┴─────────────┴───────────┘
```

**Use cases:**
- ✅ Settings, high scores, calibration (infrequent writes)
- ❌ Sensor data, temporary variables (frequent writes)

**Our data structure (4 bytes at address 0):**
```
Address 0: Score low byte  (bits 0-7)
Address 1: Score high byte (bits 8-15)
Address 2: Magic byte      (0xA5 = data valid marker)
Address 3: Checksum        (XOR of bytes 0-2)
```

**Example (score = 305):**
```
305 decimal = 0x0131 hex = 0000000100110001 binary

Address 0: 0x31 (low byte: 305 & 0xFF)
Address 1: 0x01 (high byte: 305 >> 8)
Address 2: 0xA5 (magic byte)
Address 3: 0x95 (checksum: 0x31 ^ 0x01 ^ 0xA5)
```

**Validation layers:**

1. **Magic byte check:** If byte 2 ≠ 0xA5, EEPROM never initialised
2. **Checksum verification:** Detect data corruption (bit flips, power loss)

**Reading:**
```cpp
uint16_t eeprom_read_high_score(void) {
    uint8_t low = EEPROM.read(0);
    uint8_t high = EEPROM.read(1);
    uint8_t magic = EEPROM.read(2);
    uint8_t checksum = EEPROM.read(3);

    if (magic != 0xA5) return 0;  // Uninitialised

    uint8_t expected = low ^ high ^ magic;
    if (checksum != expected) return 0;  // Corrupted

    return (uint16_t)low | ((uint16_t)high << 8);
}
```

**Writing:**
```cpp
void eeprom_write_high_score(uint16_t score) {
    uint8_t low = score & 0xFF;
    uint8_t high = (score >> 8) & 0xFF;
    uint8_t checksum = low ^ high ^ 0xA5;

    EEPROM.update(0, low);
    EEPROM.update(1, high);
    EEPROM.update(2, 0xA5);
    EEPROM.update(3, checksum);
}
```

**EEPROM.update() vs EEPROM.write():**

`update()` only writes if value differs:
```cpp
EEPROM.update(addr, val);  // Checks first, writes if different
// Preserves write endurance!
```

If high score is already 100, writing 100 again:
- `write()`: Writes unconditionally (4 cycles wasted)
- `update()`: Compares first, writes nothing (0 cycles)

### Five-State Game Flow

```
ATTRACT (demo mode)
   ↓ Button press
PLAYING (active game)
   ↓ Button + hit target
RESULT (300ms pause)
   ↓
PLAYING (continue)
   ↓ Button + miss + new high score
CELEBRATION (2 seconds)
   ↓
ATTRACT

   OR

PLAYING
   ↓ Button + miss + no high score
GAME_OVER (animation duration)
   ↓
ATTRACT
```

**New state: CELEBRATION**

Triggered when:
- Player misses (game ends)
- AND new high score was achieved during game

Behaviour:
- Display: "NEW HIGH SCORE! Score: 150"
- Animation: 5-note melody + 3 LED sweeps (parallel)
- Duration: 2 seconds minimum
- Then: Return to ATTRACT

## How to Run

**Local with PlatformIO:**
```bash
cd 09-parallel-animations
pio run --target upload
```

**Online with Wokwi:**
1. Visit https://wokwi.com/
2. Upload all files
3. Click "Start Simulation"

## Gameplay Instructions

1. **Attract:** Display shows high score (persists across resets!)
2. **Press button** to start
3. **Playing:** Score points by hitting green LEDs
4. **Hit:** Brief pause, score updates, continue
5. **Miss (new high score):** Celebration animation, save to EEPROM, return to attract
6. **Miss (no high score):** Game over animation, return to attract
7. **Power cycle:** High score preserved in EEPROM!

## Exercises

### Exercise 1: Verify Non-Blocking Behaviour
**Task:** Prove animations don't block main loop.

**Steps:**
1. Add LED13 blink to main loop:
   ```cpp
   void loop() {
       static uint32_t last_blink = 0;
       if (millis() - last_blink >= 100) {
           digitalWrite(13, !digitalRead(13));
           last_blink = millis();
       }
       game_update();
   }
   ```

2. Play game, trigger celebration animation
3. Observe: LED13 continues blinking during animation (system responsive!)

### Exercise 2: Add Bonus Animation
**Task:** Create "combo" animation for 3 bullseyes in a row.

**Steps:**
1. Add to AnimationState enum:
   ```cpp
   ANIM_COMBO
   ```

2. Track consecutive bullseyes in game.cpp:
   ```cpp
   static uint8_t combo_count = 0;

   // In playing_update after bullseye:
   if (points == BULLSEYE_SCORE) {
       combo_count++;
       if (combo_count >= 3) {
           animation_start_combo();
           combo_count = 0;
       }
   }
   ```

3. Implement animation_start_combo() and ANIM_COMBO case

### Exercise 3: Measure Animation Overhead
**Task:** How much CPU time does animation_update() use?

**Steps:**
1. Add timing:
   ```cpp
   uint32_t start = micros();
   animation_update();
   uint32_t elapsed = micros() - start;
   ```

2. Typical results:
   - IDLE: ~5 μs (just return)
   - Playing animation: ~50-100 μs
   - Still well under 1ms frame budget!

### Exercise 4: Debug Parallel Timing
**Task:** Visualise buzzer vs LED timing.

**Steps:**
1. Add Serial debugging to hardware.cpp:
   ```cpp
   case ANIM_CELEBRATION:
       if (note_triggered) {
           Serial.print("Buzzer: ");
           Serial.println(millis());
       }
       if (led_moved) {
           Serial.print("  LED: ");
           Serial.println(millis());
       }
   ```

2. Observe output shows interleaved timing
3. Proves parallel execution

### Exercise 5: Test EEPROM Persistence
**Task:** Verify high score survives power loss.

**Steps:**
1. Play game, achieve score of 50
2. Note high score displayed
3. Reset Arduino (power cycle or reset button)
4. Observe: High score still 50 (loaded from EEPROM!)
5. Upload different firmware
6. Re-upload this firmware
7. Observe: High score preserved (EEPROM survives firmware updates!)

## Key Takeaways

1. **Non-blocking is essential** - delay() blocks entire system, use millis() timestamps

2. **Cooperative multitasking** - Multiple tasks run "simultaneously" by each returning quickly

3. **Parallel timing** - Independent timers let different animations run at different speeds

4. **State machines scale** - Two state machines (game + animation) running concurrently

5. **EEPROM for persistence** - Store data that must survive power loss (with validation!)

6. **EEPROM.update() saves wear** - Only write when value changes, preserves endurance

7. **Always validate EEPROM data** - Magic byte + checksum detect uninitialised/corrupted data

## Performance Analysis

**Frame time breakdown (during CELEBRATION):**
```
animation_update():     ~80 μs
game_update():         ~200 μs
Total:                 ~280 μs

Loop frequency: ~3500 iterations/second
Plenty of CPU headroom for watchdog timer (Tutorial 10)
```

**Memory usage:**
```
Flash: ~8.0 KB (includes animation system)
RAM:   ~420 bytes
EEPROM: 4 bytes (high score storage)
```

## Coming in Tutorial 10

Tutorial 10 is the complete production game. It adds:
- Watchdog timer (automatic reset if code hangs)
- This validates our non-blocking architecture
- All pieces integrated: animations, EEPROM, HAL, state machines
- Production-ready embedded firmware

The groundwork we laid in Tutorials 1-9 makes this possible!
