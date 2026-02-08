# Light Chaser - Progressive Embedded Systems Tutorial Series

A comprehensive tutorial series that teaches embedded systems programming by building a complete reaction-time game on Arduino Uno. Progress from basic LED blinking to production-quality firmware with state machines, non-blocking animations, and hardware safety mechanisms.

## 🎯 What You'll Build

A fast-paced LED chase game where players press a button to "catch" a moving light. Hit the green LEDs (bullseye) to score points, with increasing difficulty as the chase speeds up. Features include:

- 8-LED chase animation with bounce physics
- LCD display showing scores and messages
- Buzzer sound effects and melodies
- High score persistence (survives power-off)
- New high score celebration animations
- Automatic crash recovery (watchdog timer)

## 🎓 Learning Outcomes

By completing this series, you'll master:

### Core Embedded Concepts
- GPIO configuration and control (INPUT/OUTPUT modes)
- Non-blocking timing with `millis()` (why `delay()` is problematic)
- Button debouncing and edge detection
- State machines with enter/update/exit lifecycle
- Hardware abstraction layer (HAL) pattern
- Cooperative multitasking on bare metal

### Hardware Protocols
- I2C communication (LCD display)
- PWM for audio generation (piezo buzzer)
- EEPROM persistence with data validation
- Watchdog timer for system reliability

### Professional Patterns
- Table-driven state machines with function pointers
- Parallel timing (multiple independent timers)
- Non-blocking animation state machines
- Static memory allocation (no malloc)
- Magic bytes and checksums for data validation
- Production-ready error handling

## 📚 Tutorial Structure

### Difficulty Progression

| Tutorial | Lines | Concept | Difficulty | Duration |
|----------|-------|---------|------------|----------|
| [01-circuit-only](01-circuit-only/) | 0 | Hardware layout | ⭐ Beginner | 15 min |
| [02-single-led-blink](02-single-led-blink/) | ~40 | GPIO basics | ⭐ Beginner | 30 min |
| [03-led-chase-basic](03-led-chase-basic/) | ~80 | Arrays, loops | ⭐⭐ Beginner | 45 min |
| [04-nonblocking-chase](04-nonblocking-chase/) | ~100 | Non-blocking timing | ⭐⭐⭐ Intermediate | 1 hour |
| [05-button-debounce](05-button-debounce/) | ~150 | Button input | ⭐⭐⭐ Intermediate | 1 hour |
| [06-simple-state-machine](06-simple-state-machine/) | ~200 | State machines | ⭐⭐⭐ Intermediate | 1.5 hours |
| [07-state-lifecycle](07-state-lifecycle/) | ~400 | Enter/exit/update | ⭐⭐⭐⭐ Advanced | 2 hours |
| [08-display-and-sound](08-display-and-sound/) | ~600 | I2C, HAL pattern | ⭐⭐⭐⭐ Advanced | 2 hours |
| [09-parallel-animations](09-parallel-animations/) | ~900 | Parallel animations | ⭐⭐⭐⭐⭐ Expert | 3 hours |
| [10-complete-game](10-complete-game/) | ~600 | Full integration | ⭐⭐⭐⭐⭐ Expert | 2 hours |

**Total: ~14 hours of hands-on learning**

### Tutorial Descriptions

#### Tutorial 1: Circuit Only
**Hardware layout and Wokwi simulator introduction**

Learn about components (Arduino Uno, LEDs, button, buzzer, LCD), pin assignments, and circuit design rationale. No coding yet—focus on understanding the hardware.

**Key Topics:** GPIO pins, I2C protocol, current limiting resistors, pull-up resistors

---

#### Tutorial 2: Single LED Blink
**Basic GPIO output and Arduino setup/loop pattern**

Your first embedded program! Blink a single LED using `pinMode()`, `digitalWrite()`, and `delay()`. Understand the Arduino program lifecycle.

**Key Topics:** `setup()` vs `loop()`, OUTPUT mode, HIGH/LOW states, `delay()`, `uint8_t` types

---

#### Tutorial 3: LED Chase Animation
**Arrays, iteration, and bounce logic**

Create a chase effect bouncing left-to-right across all 8 LEDs. Learn array indexing and directional control.

**Key Topics:** Arrays, `for` loops, position tracking, direction reversal, signed integers

---

#### Tutorial 4: Non-Blocking Chase with millis()
**⚠️ CRITICAL TUTORIAL - Non-blocking timing fundamentals**

Rewrite the chase animation WITHOUT `delay()`. This is the most important pattern in embedded systems—everything after this builds on non-blocking timing.

**Key Topics:** `millis()` timestamps, elapsed time calculations, why `delay()` breaks responsiveness, `uint32_t` for timestamps

---

#### Tutorial 5: Button Input with Debouncing
**Button input, debouncing, and edge detection**

Add interactive button input with proper debouncing. Understand mechanical bounce and edge vs level detection.

**Key Topics:** `INPUT_PULLUP` mode, edge detection, debouncing algorithm, `Serial.print()` debugging

---

#### Tutorial 6: Simple State Machine
**State machine fundamentals**

Introduce game states (ATTRACT, PLAYING, GAME_OVER) with basic if/else dispatch. Learn when and why to use state machines.

**Key Topics:** `enum` types, state transitions, state-specific behaviour, game flow

---

#### Tutorial 7: State Lifecycle Pattern
**🔥 ADVANCED - Professional state machine architecture**

Refactor to the enter/update/exit pattern used in production embedded systems and game engines. Split code into modules (main.cpp + game.cpp/h).

**Key Topics:** Function pointers, table-driven programming, `StateHandler` struct, centralised transitions, code organisation

---

#### Tutorial 8: Display and Sound
**I2C communication and hardware abstraction**

Add LCD display (I2C protocol) and buzzer sound effects. Introduce the Hardware Abstraction Layer (HAL) pattern for clean code separation.

**Key Topics:** I2C protocol (SDA/SCL), LCD library usage, PWM tone generation, HAL pattern, modular architecture

---

#### Tutorial 9: Parallel Animations
**🎯 EXPERT - Non-blocking animation state machines**

Replace blocking `tone()` calls with a complete non-blocking animation system. Demonstrates parallel timing with independent buzzer and LED animations running simultaneously.

**Key Topics:** Animation state machines, cooperative multitasking, parallel timing, `animation_update()`, EEPROM persistence

---

#### Tutorial 10: Complete Game
**Production integration with watchdog timer**

The final, production-ready game with automatic crash recovery. All concepts from tutorials 1-9 integrated into reliable, maintainable firmware.

**Key Topics:** Watchdog timer (WDT), system reliability, memory usage analysis, production considerations, architectural review

## 🛠️ Prerequisites

### Hardware
- Arduino Uno (or compatible)
- 8× LEDs (6 red, 2 green)
- 8× 220Ω resistors
- 1× Push button
- 1× Piezo buzzer
- 1× LCD 16×2 with I2C backpack
- Breadboard and jumper wires

**OR** just use the **Wokwi simulator** (no physical hardware needed!)

### Software
- VS Code with PlatformIO extension
- OR Arduino IDE
- Git (for cloning repository)

### Knowledge
- Basic programming (variables, functions, loops)
- No embedded experience required!

## 🚀 Getting Started

### Option 1: Wokwi Simulator (Recommended for Beginners)

1. Navigate to a tutorial folder
2. Open in VS Code with PlatformIO
3. Build the project: `pio run`
4. Open Wokwi simulator
5. Upload firmware and run

**Advantage:** No hardware required, instant feedback, built-in debugging tools.

### Option 2: Physical Hardware

1. Wire the circuit according to `diagram.json` in tutorial 1
2. Connect Arduino via USB
3. Upload firmware using PlatformIO or Arduino IDE
4. Test and iterate

**Advantage:** Tactile experience, real-world behaviour, final product satisfaction.

## 📖 How to Use This Series

### For Self-Learners
1. **Read sequentially** - Each tutorial builds on previous ones
2. **Type, don't copy** - Muscle memory matters in embedded development
3. **Do the exercises** - Each tutorial includes 3-5 challenges
4. **Experiment freely** - Break things and understand why
5. **Read READMEs thoroughly** - Code is comment-free by design; all explanations are in READMEs

### For Instructors
- **Modular curriculum** - Use individual tutorials as lessons
- **Progressive difficulty** - Natural learning curve
- **Real project** - Students build something tangible
- **Professional patterns** - Industry-standard techniques
- **Assessment ready** - Exercises provide evaluation opportunities

### For Experienced Developers
- Jump to **Tutorial 4** (non-blocking patterns) if you know GPIO basics
- Focus on **Tutorial 7** (state machines) and **Tutorial 9** (animations) for architecture
- Read **Tutorial 10** README for production patterns summary

## 💡 Code Philosophy

### Comment-Free Code
All tutorial code files (.cpp, .h) have **zero comments**. This is intentional:

**Why?**
- Forces engagement with comprehensive README documentation
- Mimics production code (well-written code should be self-documenting)
- Prevents "comment rot" where comments become outdated
- Encourages understanding over memorisation

**Where are explanations?**
- **README.md files** contain detailed concept explanations
- **Exercises** reinforce understanding through practice
- **Code examples in READMEs** show patterns with annotations

### Professional Patterns from Day One
Unlike many tutorials that teach "toy code" first and "real code" later, this series teaches production patterns immediately:

- ✅ Proper data types (`uint8_t` not `int`)
- ✅ Static allocation (no `malloc`)
- ✅ Non-blocking timing (no `delay()` after Tutorial 3)
- ✅ Hardware abstraction (HAL pattern)
- ✅ Safety mechanisms (watchdog timer)

## 📊 Memory Usage

Final game (Tutorial 10) memory footprint:
```
RAM:   421 / 2048 bytes (20.6%) - Game state and buffers
Flash: 8026 / 32256 bytes (24.9%) - Program code
EEPROM: 4 / 1024 bytes (0.4%) - High score storage
```

**Plenty of headroom for expansion!**

## 🎮 Game Flow Diagram

```
         ┌─────────────┐
         │  ATTRACT    │ ◄──────────┐
         │ (demo mode) │            │
         └──────┬──────┘            │
                │                   │
         Press button               │
                │                   │
         ┌──────▼──────┐            │
    ┌───┤   PLAYING   ├───┐        │
    │   │ (active game)│   │        │
    │   └─────────────┘    │        │
    │                      │        │
Hit │                    Miss       │
    │                      │        │
┌───▼────┐           ┌────▼─────┐  │
│ RESULT │           │GAME_OVER │  │
│(pause) │           │(sad sound)│ │
└───┬────┘           └────┬─────┘  │
    │                     │        │
Continue               Animation   │
    │                   complete   │
    └─────────┐            │       │
              │            └───────┤
       High score?                 │
              │                    │
         ┌────▼────────┐           │
         │CELEBRATION  │           │
         │ (new record)│           │
         └────┬────────┘           │
              │                    │
       Animation complete          │
              │                    │
              └────────────────────┘
```

## 🏆 Challenges Beyond Tutorial 10

Ready to extend the complete game? Try these:

### Easy
1. **Speed modes** - Add difficulty settings (easy/normal/hard)
2. **Sound toggle** - Button combo to mute/unmute buzzer
3. **Colour patterns** - Change LED patterns (all on, wave, etc.)

### Medium
4. **Multiplayer** - Add second button, track two players
5. **Time attack** - 30-second timed mode with score counting
6. **Combo system** - Bonus points for consecutive bullseyes

### Hard
7. **Power-saving mode** - Use sleep modes between games
8. **Bluetooth scores** - Send high scores via HC-05 Bluetooth module
9. **OLED display** - Replace LCD with SSD1306 OLED for graphics
10. **Difficulty curve** - Dynamic speed adjustment based on player skill

## 📁 Repository Structure

```
light-chaser/
├── README.md (this file)
├── 01-circuit-only/
│   ├── diagram.json
│   └── README.md
├── 02-single-led-blink/
│   ├── diagram.json
│   ├── platformio.ini
│   ├── wokwi.toml
│   ├── include/config.h
│   ├── src/main.cpp
│   └── README.md
├── 03-led-chase-basic/
├── 04-nonblocking-chase/
├── 05-button-debounce/
├── 06-simple-state-machine/
├── 07-state-lifecycle/
│   ├── include/
│   │   ├── config.h
│   │   └── game.h
│   └── src/
│       ├── main.cpp
│       └── game.cpp
├── 08-display-and-sound/
│   ├── include/
│   │   ├── config.h
│   │   ├── game.h
│   │   └── hardware.h
│   └── src/
│       ├── main.cpp
│       ├── game.cpp
│       └── hardware.cpp
├── 09-parallel-animations/
├── 10-complete-game/
└── game/ (original heavily-commented reference code)
```

## 🤝 Contributing

Found a bug? Have a suggestion? Contributions welcome!

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## 📝 Licence

This tutorial series is provided as-is for educational purposes.

## 🙏 Acknowledgements

This tutorial series demonstrates professional embedded patterns inspired by:
- Production embedded systems in industrial automation
- Game engine architectures (Unity, Unreal)
- Real-Time Operating Systems (FreeRTOS, RTOS patterns)
- Arduino community best practices

## 📚 Further Learning

### Books
- "Making Embedded Systems" by Elecia White
- "The Art of Embedded Systems Programming" by Jack Ganssle
- "Embedded Systems Architecture" by Daniele Lacamera

### Online Resources
- [Arduino Documentation](https://docs.arduino.cc/)
- [AVR Libc Reference](https://www.nongnu.org/avr-libc/)
- [Embedded Artistry](https://embeddedartistry.com/)

### Next Projects
- **Temperature monitor** with LCD and DHT22 sensor
- **MIDI controller** with buttons and USB communication
- **Data logger** with SD card and RTC
- **Motor controller** with PWM and H-bridge driver

---

## ⭐ Star this Repository

If you find this tutorial series helpful, please star the repository!

**Happy Learning!** 🚀
