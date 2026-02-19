# Emotion Station — Hardware Guide

Bill of Materials, wiring reference, SD card setup, and build/flash instructions.

---

## 1. Bill of Materials

### Components

| Qty | Component | Spec | Notes |
|-----|-----------|------|-------|
| 1 | ESP32 Dev Board | ESP32-WROOM-32 DevKit v1, 38-pin | Any standard 38-pin variant works |
| 1 | NFC Reader | PN532 breakout | Must be configured for I2C mode (DIP switch) |
| 6 | NFC Token | MIFARE Classic ISO14443A, 7-byte UID | One per mood |
| 1 | WS2812B LED Ring/Strip | 5V, 16 LEDs | Pre-built 16-LED ring preferred |
| 1 | I2S Audio DAC | MAX98357A breakout | 3W class-D, 5V input |
| 1 | Speaker | 4Ω or 8Ω, ≥1W | Matches MAX98357A output |
| 1 | SD Card Module | SPI, 3.3V logic | Standard breakout |
| 1 | MicroSD Card | ≥4GB, FAT32 | Class 10 recommended |
| 2 | Resistor 10kΩ | ¼W | Battery voltage divider |
| 2 | Resistor 4.7kΩ | ¼W | I2C pull-ups (recommended) |
| 1 | Resistor 330Ω | ¼W | NeoPixel data line protection |
| 1 | LiPo Battery | 3.7V, 2000–5000mAh | Optional, for portable use |
| 1 | LiPo BMS/Charger | TP4056 with protection | Required if using battery |

### Libraries

Installed automatically by PlatformIO on first build:

| Library | Version |
|---------|---------|
| `adafruit/Adafruit NeoPixel` | `^1.11.0` |
| `adafruit/Adafruit PN532` | `^1.3.1` |
| `bblanchon/ArduinoJson` | `^6.21.3` |
| `schreibfaul1/ESP32-audioI2S` | `v2.0.0` (GitHub) |

---

## 2. Pin Wiring

All pin assignments are defined in `src/base/utils/config.h`.

| GPIO | Signal | Component | Notes |
|------|--------|-----------|-------|
| 2 | STATUS_LED | Status LED | Active-HIGH; do NOT pull LOW during boot |
| 4 | SD_CS | SD Card | SPI chip select |
| 5 | LED_DATA | WS2812B strip | 330Ω series resistor recommended |
| 18 | SD_SCK | SD Card | SPI clock |
| 19 | SD_MISO | SD Card | SPI data in |
| 21 | I2C_SDA | PN532 NFC | Pull up to 3.3V via 4.7kΩ (recommended) |
| 22 | I2C_SCL | PN532 NFC | Pull up to 3.3V via 4.7kΩ (recommended) |
| 23 | SD_MOSI | SD Card | SPI data out |
| 25 | I2S_BCLK | MAX98357A | I2S bit clock |
| 26 | I2S_LRC | MAX98357A | I2S left/right clock |
| 27 | I2S_DOUT | MAX98357A | I2S data out |
| 34 | BATTERY_ADC | Voltage divider | Input-only pin — do NOT drive |

### Battery Voltage Divider

Two 10kΩ resistors scale the 3.3–4.2V battery voltage to the 1.65–2.1V range read by GPIO 34:

```
BATT+ ─── R1(10kΩ) ─── GPIO34 ─── R2(10kΩ) ─── GND
```

---

## 3. PN532 I2C Mode

The PN532 module ships in UART/HSU mode by default. Switch it to I2C before wiring:

- **DIP switch 1:** ON
- **DIP switch 2:** OFF

The I2C address is fixed at `0x24` (as configured in the firmware).

---

## 4. SD Card Setup

Format the card as **FAT32** (not exFAT — this is not supported by the Arduino SD library).

Required file structure:

```
/
├── activities.json          ← REQUIRED at boot
└── audio/
    ├── happy/
    │   └── 25_sunshine_dance.mp3
    ├── sad/
    │   └── 17_rainbow_breath.mp3
    ├── calm/
    │   └── 41_slow_breathing.mp3
    ├── energetic/
    │   └── movement_1.mp3
    ├── anxious/
    │   └── 9_bubble_breath.mp3
    └── angry/
        └── 1_dragon_breath.mp3
```

`sessions.csv` is created automatically on first boot.

### activities.json Schema

Defined in `src/middleware/activity_repository/activity_repository_sd.cpp`:

```json
{
  "activities": [
    {
      "id": 1,
      "mood": "happy",
      "name": "Sunshine Dance",
      "file_path": "/audio/happy/25_sunshine_dance.mp3",
      "duration_seconds": 120,
      "type": "movement",
      "time_of_day": ["morning", "afternoon"]
    }
  ]
}
```

**Valid field values:**

| Field | Valid values |
|-------|-------------|
| `mood` | `happy`, `sad`, `calm`, `energetic`, `anxious`, `angry` |
| `time_of_day` | `morning` (06–11h), `afternoon` (12–16h), `evening` (17–20h), `bedtime` (21–05h) |
| `type` | `movement`, `breathing`, `visualisation`, `mindfulness` |

> **Buffer limit:** The JSON document must fit within 8192 bytes (~50 activities maximum).

---

## 5. NFC Token Programming

Each physical token must have its 7-byte UID registered in `src/middleware/mood_registry/mood_registry.cpp`.

**The UIDs in the source code are placeholders.** You must read each physical card's UID and update the corresponding entry:

```cpp
// mood_registry.cpp — update each UID array to match your physical cards:
{ MOOD_HAPPY,     "Happy",     "/audio/happy/...",  {0x04, 0xA1, ...} },
{ MOOD_SAD,       "Sad",       "/audio/sad/...",    {0x04, 0xB2, ...} },
// etc.
```

**To read a card's UID:**

1. Flash the `esp32dev` build and open the serial monitor.
2. Press `n` — the device enters a 10-second NFC scan window.
3. Present the card; the UID is printed to serial.
4. Update `mood_registry.cpp` with the UID, then rebuild and reflash.

---

## 6. Prerequisites

| Tool | Purpose |
|------|---------|
| [VS Code](https://code.visualstudio.com/) + [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) | Recommended IDE |
| PlatformIO CLI | Alternative to VS Code |
| Git | Clone the repository |
| USB driver (CP2102 or CH340) | Depends on your ESP32 board's UART bridge |

Install PlatformIO CLI via pip:

```bash
pip install platformio
```

---

## 7. Build and Flash

```bash
# Clone the repository
git clone <repo-url>
cd emotion-station

# Run unit tests (no hardware required)
~/.platformio/penv/bin/pio test -e native

# Debug build — compile and flash
~/.platformio/penv/bin/pio run -e esp32dev --target upload

# Release build (optimised, serial debug output disabled)
~/.platformio/penv/bin/pio run -e esp32dev-release --target upload

# Monitor serial output (115200 baud)
~/.platformio/penv/bin/pio device monitor -e esp32dev
```

PlatformIO downloads all library dependencies automatically on first build.

---

## 8. First Boot Verification

Expected serial output after a successful boot:

```
=== Emotion Station Booting ===
[BATTERY] Initial voltage: 4.15V
NFC: Found chip PN532
[ACTIVITY] SD card mounted
[ACTIVITY] Loaded 18 activities
Audio: init OK
[LOGGER] Created sessions.csv with header
Hardware initialisation complete
```

### Troubleshooting

| Error message | Cause | Fix |
|---------------|-------|-----|
| `NFC: ERROR — chip not found` | PN532 not wired or wrong mode | Check DIP switches (I2C mode), check SDA/SCL wiring |
| `[ACTIVITY] ERROR: SD card mount failed` | SD card not detected | Check CS/SCK/MOSI/MISO wiring, confirm FAT32 format |
| `[ACTIVITY] ERROR: /activities.json not found` | Missing file | Copy `activities.json` to SD card root |
| `[ACTIVITY] ERROR: JSON parse failed` | Malformed JSON | Validate JSON, ensure file is UTF-8 without BOM |
| No audio output | MAX98357A not wired | Check BCLK/LRC/DOUT wiring |

---

## 9. Serial Debug Commands

Available in the `esp32dev` build only (not in `esp32dev-release`).

| Key | Action |
|-----|--------|
| `n` | Scan NFC for 10 seconds — prints the UID of any card presented |
| `t` | Run state machine transition test |
| `a` | Print the loaded activity list |
| `SETTIME YYYY-MM-DD HH:MM:SS` | Set the RTC time |

---

## Critical Notes

- **GPIO 2 boot pin:** Do NOT pull GPIO 2 LOW during power-up — this prevents the ESP32 from booting.
- **GPIO 34 is input-only:** Connect the voltage divider output here. Never configure it as an output.
- **SD card must be FAT32:** exFAT is not supported by the Arduino SD library.
- **activities.json is required:** The device halts at boot if this file is missing or unreadable.
- **PN532 must be in I2C mode:** Set the DIP switches before wiring the module.
- **NFC UIDs are placeholders:** Physical card UIDs must be updated in `mood_registry.cpp` and reflashed before tokens will be recognised.
