# Arduino Light Chaser Game

A fast-paced reaction game for Arduino Uno where you try to catch a bouncing light with perfect timing.

## Game Rules

- Watch the light chase back and forth across 8 LEDs
- Press the button when the light is in the **green target zone** (centre LEDs 3-4) for maximum points
- The game gets faster as your score increases
- One miss and it's game over!

## Scoring

- **Bullseye** (green LEDs): 10 points
- **Adjacent** (positions 2 or 5): 5 points
- **Outer** (edge positions): 1 point
- **Miss**: Game over

## Difficulty Progression

Every 10 points, the chase speed increases by 5ms (chase interval decreases), making the game more challenging. Minimum speed is 50ms.

## Hardware Setup

### Components Required
- Arduino Uno R3
- 8 LEDs (6 red + 2 green)
- 8 × 220Ω resistors
- 1 pushbutton
- 1 passive buzzer
- Breadboard and jumper wires

### Pin Configuration
- **LEDs**: Digital pins 2-9
  - Pins 2, 3, 4, 7, 8, 9: Red LEDs
  - Pins 5, 6: Green LEDs (target zone)
- **Button**: Digital pin 10 (with internal pull-up)
- **Buzzer**: Digital pin 11

## Building the Project

### Using PlatformIO
```bash
cd embedded-game-1
pio run
pio run --target upload
```

### Using Wokwi Simulator
1. Open [Wokwi](https://wokwi.com/)
2. Upload `diagram.json`
3. The circuit and code will be loaded automatically
4. Click "Start Simulation"

## Project Structure

```
embedded-game-1/
├── platformio.ini          # PlatformIO configuration
├── diagram.json            # Wokwi circuit definition
├── include/
│   ├── config.h           # All game constants and pin definitions
│   ├── hardware.h         # Hardware abstraction layer interface
│   └── game.h             # Game logic interface
└── src/
    ├── main.cpp           # Application entry point
    ├── hardware.cpp       # Hardware implementation (LEDs, button, buzzer)
    └── game.cpp           # Game state machine and logic
```

## Code Architecture

### Separation of Concerns
- **config.h**: Centralised constants (no magic numbers)
- **hardware layer**: Abstract hardware details (button debouncing, LED control, sound effects)
- **game layer**: Pure game logic (state machine, scoring, difficulty)
- **main.cpp**: Minimal glue code

### Key Features
- Non-blocking timing using `millis()` throughout
- Robust button handling with edge detection and debouncing
- State machine pattern for clear game flow
- Progressive difficulty system
- Sound feedback for all major events

## Game States

1. **ATTRACT**: Demo mode with chasing animation, waiting for start
2. **PLAYING**: Active gameplay with scoring
3. **RESULT**: Brief feedback display after successful hit
4. **GAME_OVER**: Score display and high score tracking

## Sound Effects

- **Tick**: Low beep on each LED position change
- **Hit**: Medium tone for regular hits
- **Bullseye**: Rising three-note sequence for perfect hits
- **Game Over**: Descending three-note sequence

## High Score

The game tracks your high score across play sessions (until power cycle).

## Licence

MIT
