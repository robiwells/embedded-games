# Tutorial 1: Circuit Only

## What You'll Learn
- Hardware layout and component placement
- Understanding the Wokwi simulator
- Pin assignments and wiring basics
- Component specifications and requirements

## Prerequisites
- None - this is the starting point!
- Basic familiarity with electronics concepts helps but isn't required

## Hardware Components

### Arduino Uno
The brain of our project. An 8-bit microcontroller with:
- **14 digital I/O pins** (pins 0-13) - can be INPUT or OUTPUT
- **6 analogue input pins** (A0-A5) - read voltages 0-5V
- **16 MHz clock speed** - executes ~16 million instructions per second
- **32 KB Flash memory** - stores your program
- **2 KB SRAM** - temporary data storage while running
- **1 KB EEPROM** - non-volatile storage (persists after power-off)

### 8× LEDs (Light Emitting Diodes)
Visual output for the game. LEDs require current-limiting resistors to prevent burnout.
- **6× Red LEDs** - Standard indicator LEDs
- **2× Green LEDs** - The "bullseye" target zone
- **Forward voltage** - ~2V typical for red, ~2.2V for green
- **Operating current** - 10-20mA (we'll use ~15mA)

### 8× 220Ω Resistors
Current limiters for the LEDs. Ohm's Law calculation:
```
Arduino output: 5V
LED forward voltage: ~2V
Desired current: ~15mA
Required resistance: (5V - 2V) / 0.015A = 200Ω
Using standard value: 220Ω (gives ~13.6mA - safe)
```

### Push Button
Player input device. Simple momentary switch.
- **Normally open** - no connection until pressed
- **Mechanical contacts** - will "bounce" when pressed (we'll handle this in software)
- **Wired to ground** - Arduino pin uses INPUT_PULLUP (internal resistor)

### Piezo Buzzer
Audio output for game sounds and feedback.
- **Passive buzzer** - requires PWM signal to generate tones
- **Frequency range** - typically 100Hz to 10kHz
- **Controlled by Arduino tone()** - generates square waves at specified frequencies

### LCD Display (16×2 I2C)
Text output for scores and messages.
- **16 columns × 2 rows** - displays up to 32 characters
- **I2C interface** - only uses 2 pins (SDA and SCL) instead of 16 parallel pins
- **I2C address** - typically 0x27 or 0x3F (we use 0x27)
- **Backlight** - always-on LED backlight for visibility

## Pin Assignments

### Digital Pins (LEDs)
| Pin | Component | Colour | Purpose |
|-----|-----------|--------|---------|
| 2 | LED 0 | Red | Chase position 0 |
| 3 | LED 1 | Red | Chase position 1 |
| 4 | LED 2 | Red | Chase position 2 |
| 5 | LED 3 | **Green** | **Bullseye start** |
| 6 | LED 4 | **Green** | **Bullseye end** |
| 7 | LED 5 | Red | Chase position 5 |
| 8 | LED 6 | Red | Chase position 6 |
| 9 | LED 7 | Red | Chase position 7 |

**Design rationale**: Sequential pins (2-9) allow us to use loops and arithmetic:
```cpp
for (uint8_t i = 0; i < 8; i++) {
    pinMode(LED_PIN_START + i, OUTPUT);
}
```

### Digital Pins (Input/Output)
| Pin | Component | Purpose |
|-----|-----------|---------|
| 10 | Button | Player input (INPUT_PULLUP mode) |
| 11 | Buzzer | PWM audio output (tone generation) |

**Why pin 11 for buzzer?** Pin 11 is PWM-capable (marked with ~ on Arduino board). The tone() function requires a PWM pin to generate audio frequencies.

### Analogue Pins (I2C)
| Pin | Signal | Purpose |
|-----|--------|---------|
| A4 | SDA | I2C data line (bidirectional) |
| A5 | SCL | I2C clock line (master to slave) |

**I2C Protocol**: Two-wire serial communication. Arduino is the "master", LCD is the "slave". Master generates clock signal (SCL), both devices share data line (SDA).

### Power and Ground
- **5V pin** - Powers LCD module
- **GND pins** - Common ground for all components (GND.1 for LEDs, GND.2 for button/buzzer/LCD)

## Physical Layout Rationale

### LED Arrangement
LEDs are arranged in a horizontal line:
```
[LED0] [LED1] [LED2] [LED3] [LED4] [LED5] [LED6] [LED7]
  Red    Red    Red   GREEN  GREEN   Red    Red    Red
```

This visual layout makes it easy to see the "chase" effect and identify the green bullseye zone at a glance.

### Component Grouping
- **LEDs at top** - Primary visual output, positioned for visibility
- **Button and Buzzer at bottom left/centre** - Input and audio feedback
- **LCD at bottom right** - Secondary display, doesn't need constant attention

### Wiring Colours (Convention)
- **Red** - Power (5V)
- **Black** - Ground (0V)
- **Green** - Signal wires (LED connections)
- **Blue** - Input signal (button)
- **Orange** - Output signal (buzzer)
- **Blue/Yellow** - I2C communication (SDA/SCL)

Consistent colour coding makes debugging easier and follows electronics conventions.

## Understanding the Wokwi Simulator

Wokwi is a web-based electronics simulator that lets you:
- **Build circuits** - Drag and drop components
- **Write code** - Upload Arduino sketches
- **Simulate in real-time** - Run your circuit without physical hardware
- **Debug** - Inspect pin states, serial output, and timing

### Key Wokwi Features
1. **No hardware required** - Test circuits before building
2. **Instant feedback** - See LED states, measure voltages, monitor serial output
3. **Debugging tools** - Logic analyser, serial monitor, pin state inspector
4. **Shareable** - Save and share circuit diagrams
5. **Fast iteration** - Upload new code instantly, no compilation wait

### How This Circuit Works
In Wokwi, the `diagram.json` file defines:
- **Parts list** - All components (Arduino, LEDs, resistors, button, buzzer, LCD)
- **Positions** - X/Y coordinates for layout
- **Connections** - Wire routing between components
- **Attributes** - Component-specific settings (LED colours, resistor values, etc.)

The simulator reads this JSON and creates an interactive visual circuit.

## What's Next?

In the next tutorial, we'll write our first code to:
- Initialise the hardware (pinMode)
- Blink a single LED (digitalWrite)
- Understand the setup() and loop() pattern
- Use delay() for timing (we'll improve this later!)

The circuit will remain the same throughout all tutorials - only the code changes.

## Further Reading
- [Arduino Pin Diagram](https://docs.arduino.cc/hardware/uno-rev3/) - Official hardware reference
- [LED Current Limiting](https://www.sparkfun.com/tutorials/219) - Understanding resistor calculations
- [I2C Communication](https://learn.sparkfun.com/tutorials/i2c) - How the two-wire protocol works
- [Wokwi Documentation](https://docs.wokwi.com/) - Simulator features and tutorials
