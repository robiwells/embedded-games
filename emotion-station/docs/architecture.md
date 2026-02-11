# Emotion Check-In Station - Architecture Document

## 1. System Overview

### Project Description
The Emotion Check-In Station is an audio-first emotional regulation device for children aged 4-10. Children use NFC emotion tokens to select how they're feeling, and the device responds with age-appropriate guided activities (breathing exercises, movement activities, mindfulness practices) delivered via high-quality audio.

### Design Philosophy
- **Radical Simplicity**: Single interaction model (tap token → receive activity)
- **Screen-Free**: Audio-only output to reduce overstimulation
- **Non-Blocking**: All operations use state machines and millis()-based timing
- **Child-Centred**: Large tactile tokens, immediate feedback, no complex menus
- **Parent-Supportive**: Background data logging for insight without intrusion

### Architectural Principles (Inherited from Light Chaser)
1. **Enter/Exit/Update State Machine** - Clear lifecycle management
2. **Hardware Abstraction Layer** - Testable, maintainable modules
3. **Non-Blocking Everywhere** - Watchdog timer compliant
4. **Centralised State Transitions** - Single transition function prevents bugs
5. **Memory Conservative** - Headroom for future features

### High-Level Block Diagram
```
┌─────────────┐     I2C      ┌──────────────┐
│  PN532 NFC  │◄────────────►│              │
└─────────────┘              │              │
                             │              │     I2S      ┌──────────────┐
┌─────────────┐   One-Wire   │    ESP32     │◄────────────►│ MAX98357A    │
│ WS2812B LED │◄─────────────│   DevKit     │             │  I2S DAC     │──► Speaker
│   Ring      │              │              │             └──────────────┘
└─────────────┘              │              │
                             │              │     SPI      ┌──────────────┐
┌─────────────┐              │              │◄────────────►│  MicroSD     │
│   2000mAh   │──► Power ───►│              │             │   Module     │
│   LiPo +    │              └──────────────┘             └──────────────┘
│   TP4056    │                    ▲
└─────────────┘              ADC   │ (Battery Monitoring)
                                   │
```

---

## 2. Hardware Architecture

### Component List

| Component | Model/Part | Purpose | Interface |
|-----------|------------|---------|-----------|
| Microcontroller | ESP32 DevKit | Main processor | - |
| NFC Reader | PN532 Breakout | Read emotion tokens | I2C |
| Audio DAC | MAX98357A I2S | High-quality audio output | I2S |
| LEDs | WS2812B Ring (16 LEDs) | Visual feedback | One-Wire |
| Storage | MicroSD Module | Activity audio + logs | SPI |
| Battery | 2000mAh LiPo 3.7V | Portable power | - |
| Charger | TP4056 Module | USB-C charging | - |
| Speaker | 3W 4Ω | Audio playback | Analogue |

### Pin Assignments

| GPIO | Function | Direction | Notes |
|------|----------|-----------|-------|
| GPIO21 | I2C SDA (PN532) | Bidirectional | Pull-up enabled |
| GPIO22 | I2C SCL (PN532) | Output | Pull-up enabled |
| GPIO25 | I2S BCLK (Audio) | Output | Bit clock |
| GPIO26 | I2S LRC (Audio) | Output | Left/right clock |
| GPIO27 | I2S DIN (Audio) | Output | Data output |
| GPIO5 | LED Data (WS2812B) | Output | 5V level shifted |
| GPIO18 | SPI SCK (SD) | Output | SD card clock |
| GPIO19 | SPI MISO (SD) | Input | SD card data in |
| GPIO23 | SPI MOSI (SD) | Output | SD card data out |
| GPIO4 | SPI CS (SD) | Output | SD card chip select |
| GPIO34 | Battery ADC | Input | Analogue only pin |
| GPIO2 | Status LED | Output | Onboard blue LED (boot strapping pin - must be high/floating during boot; LED will be initialised in hardware_init() after boot completes) |

### Communication Buses

- **I2C (NFC)**: 400kHz Fast Mode, 7-bit addressing (PN532 address: 0x24)
- **I2S (Audio)**: 44.1kHz sample rate, 16-bit, mono, hardware-driven
- **SPI (SD Card)**: 4MHz clock, Mode 0, hardware CS control
- **One-Wire (LEDs)**: 800kHz data rate, 5V logic (level shifted from 3.3V)

### Power Architecture

| Component | Typical Current | Peak Current | Voltage | Notes |
|-----------|----------------|--------------|---------|-------|
| ESP32 (active, no WiFi/BLE) | 80mA | 160mA | 3.3V | |
| PN532 | 60mA | 150mA | 3.3V | |
| MAX98357A | 100mA | 700mA | 3.3V | |
| WS2812B (eco mode) | 32mA | 960mA | 5V | 50% brightness during idle |
| SD Card | 80mA | 200mA | 3.3V | |
| **Total (MVP)** | **~352mA** | **~1.5A** | - | Eco mode active during idle |

**Battery Life Estimates:**
- **Continuous use:** 2000mAh ÷ 352mA = **5.7 hours** (with LED power saving)
- **With 50% duty cycle** (idle between activities): **9-13 hours** (eco mode during idle)
- **Low-power idle mode**: >24 hours standby (25% LED brightness + reduced NFC polling)

**Charging:** TP4056 provides 1A charging rate (2 hours for full charge)

---

## 3. Software Architecture

### Module Breakdown

```
emotion-station/
├── platformio.ini           # PlatformIO configuration
├── include/
│   ├── config.h            # Pin definitions, enums, constants
│   ├── game.h              # State machine interface
│   ├── hardware.h          # HAL interface
│   ├── event_bus.h         # Event bus interface and event types
│   ├── nfc_handler.h       # NFC reading and mood mapping
│   ├── audio_player.h      # Audio playback management
│   ├── led_controller.h    # LED animation state machine
│   ├── activity_manager.h  # Selection algorithm and history
│   └── data_logger.h       # Session logging to SD card
└── src/
    ├── main.cpp            # Setup, main loop, watchdog
    ├── game.cpp            # State machine implementation
    ├── hardware.cpp        # Hardware initialisation
    ├── event_bus.cpp       # Event queue and dispatch logic
    ├── nfc_handler.cpp     # NFC polling and event publishing
    ├── audio_player.cpp    # MP3 playback via I2S and events
    ├── led_controller.cpp  # Non-blocking LED effects
    ├── activity_manager.cpp # Activity selection logic
    └── data_logger.cpp     # CSV logging functions
```

### State Machine Design

```c
typedef enum {
    STATE_IDLE,              // Waiting for NFC token
    STATE_NFC_DETECTED,      // Token detected, reading UID
    STATE_VALIDATING,        // Mapping UID to mood category
    STATE_SELECTING,         // Choosing activity based on mood/time/history
    STATE_PLAYING_ACTIVITY,  // Audio playing, LED breathing
    STATE_ACTIVITY_COMPLETE, // Brief completion sound/animation
    STATE_ERROR,             // Error display with recovery
    STATE_LOW_BATTERY        // Low power mode
} GameState;

typedef struct {
    void (*enter)(void);     // Called once when entering state
    void (*update)(void);    // Called every frame while in state
    void (*exit)(void);      // Called once when leaving state
} StateHandler;
```

### State Handler Table

```c
// In game.cpp
const StateHandler state_handlers[NUM_STATES] = {
    {idle_enter,              idle_update,              idle_exit},
    {nfc_detected_enter,      nfc_detected_update,      nfc_detected_exit},
    {validating_enter,        validating_update,        validating_exit},
    {selecting_enter,         selecting_update,         selecting_exit},
    {playing_activity_enter,  playing_activity_update,  playing_activity_exit},
    {activity_complete_enter, activity_complete_update, activity_complete_exit},
    {error_enter,             error_update,             error_exit},
    {low_battery_enter,       low_battery_update,       low_battery_exit}
};
```

### State Lifecycle Details

#### STATE_IDLE
- **Enter**: LED pulsing white slowly (50% brightness for power saving), NFC polling enabled
- **Update**: Check for NFC detection every 100ms with debouncing (100ms stable presence required), monitor battery voltage
- **Exit**: Stop LED animation, clear NFC buffer, reset debounce timer
- **Transitions**: → NFC_DETECTED (token stable for 100ms) | → LOW_BATTERY (voltage < 3.4V)
- **Power Saving**: LED brightness limited to 50% (128/255) during idle, reducing current from 64mA to 32mA average

#### STATE_NFC_DETECTED
- **Enter**: Play "beep" confirmation sound, LED flash green (100% brightness), initialise retry counter
- **Update**: Read UID from PN532 with retry logic (max 3 attempts, 200ms between retries), timeout after 1000ms per attempt
- **Exit**: Store UID for validation, clear retry state
- **Transitions**: → VALIDATING (UID read success) | → ERROR (3 failed attempts) | → IDLE (token removed)
- **Retry Strategy**: Up to 3 read attempts with 200ms delays. Total worst-case: 1300ms (suitable for children aged 4-10)
- **Debouncing**: Token must be present for 100ms in IDLE before transitioning to NFC_DETECTED (prevents false positives)

#### STATE_VALIDATING
- **Enter**: Start validation timer (100ms max)
- **Update**: Look up UID in mood mapping table, validate format
- **Exit**: Store validated mood category
- **Transitions**: → SELECTING (valid mood) | → ERROR (invalid UID)

#### STATE_SELECTING
- **Enter**: Start selection timer, load activity metadata from SD
- **Update**: Run selection algorithm (filter by mood/time/history), timeout after 1s
- **Exit**: Store selected Activity struct
- **Transitions**: → PLAYING_ACTIVITY (activity selected) | → ERROR (no activities available)

#### STATE_PLAYING_ACTIVITY
- **Enter**: Start audio playback, LED breathing animation (mood-specific colour), log session start
- **Update**: Call audio.loop(), update LED animation, check for completion or cancellation
- **Exit**: Stop audio, fade out LEDs, log session end
- **Transitions**: → ACTIVITY_COMPLETE (audio finished) | → IDLE (token removed early)

#### STATE_ACTIVITY_COMPLETE
- **Enter**: Play completion sound (gentle chime), LED sparkle animation, display duration
- **Update**: Wait 2 seconds for feedback
- **Exit**: Clear activity history entry
- **Transitions**: → IDLE (timeout) | → NFC_DETECTED (new token tapped)

#### STATE_ERROR
- **Enter**: Play error tone (gentle, non-scary), LED pulsing red slowly, display error code
- **Update**: Wait for error timeout (5 seconds) or user intervention
- **Exit**: Clear error state, reset hardware modules
- **Transitions**: → IDLE (timeout or recovery successful)

#### STATE_LOW_BATTERY
- **Enter**: Play low battery warning (once), LED pulsing amber, disable NFC polling
- **Update**: Monitor battery voltage every 10 seconds
- **Exit**: Re-enable NFC polling
- **Transitions**: → IDLE (voltage > 3.5V after charging) | Deep sleep (voltage < 3.3V)

### Event Bus Architecture

The architecture uses an event-driven communication pattern via a lightweight event bus. This reduces coupling between modules while maintaining the non-blocking design principles.

#### Event Bus Overview

- **Lightweight Implementation:** Static circular queue with synchronous dispatch
- **Queue Capacity:** 8 events (176 bytes total)
- **Event Size:** 22 bytes each (fixed size for predictable memory usage)
- **Dispatch Model:** Synchronous processing in main loop (non-blocking)
- **Pattern:** Publisher/subscriber decoupling

#### Event Types

Events are categorised by their source and purpose:

**Hardware Events:**
- `NFC_DETECTED` - NFC token detected with UID
- `NFC_REMOVED` - NFC token removed from field
- `AUDIO_COMPLETE` - Audio playback finished
- `BATTERY_LOW` - Battery voltage below threshold

**State Events:**
- `STATE_ENTERED` - State machine entered new state
- `STATE_EXITED` - State machine exited previous state

**Application Events:**
- `ACTIVITY_SELECTED` - Activity chosen by selection algorithm
- `SESSION_STARTED` - Activity playback began
- `SESSION_COMPLETED` - Activity finished successfully

**System Events:**
- `ERROR_OCCURRED` - Error condition detected
- `BOOT_COMPLETE` - System initialisation finished

#### Event Structure

```c
typedef enum {
    // Hardware events
    NFC_DETECTED,
    NFC_REMOVED,
    AUDIO_COMPLETE,
    BATTERY_LOW,

    // State events
    STATE_ENTERED,
    STATE_EXITED,

    // Application events
    ACTIVITY_SELECTED,
    SESSION_STARTED,
    SESSION_COMPLETED,

    // System events
    ERROR_OCCURRED,
    BOOT_COMPLETE,

    NUM_EVENT_TYPES
} EventType;

typedef enum {
    PRIORITY_CRITICAL = 0,  // System errors, battery critical
    PRIORITY_HIGH,          // State transitions, NFC events
    PRIORITY_NORMAL,        // Activity events, audio complete
    PRIORITY_LOW            // Logging, diagnostics
} EventPriority;

typedef struct {
    EventType type;           // Event type (1 byte)
    EventPriority priority;   // Event priority (1 byte)
    uint32_t timestamp;       // millis() when published (4 bytes)
    union {
        struct {
            uint8_t uid[7];
            MoodCategory mood;
        } nfc_detected;                              // 8 bytes

        struct {
            GameState old_state;
            GameState new_state;
        } state_transition;                          // 2 bytes

        struct {
            uint8_t activity_id;
            MoodCategory mood;
        } activity;                                  // 2 bytes

        struct {
            ErrorCode error_code;
        } error;                                     // 1 byte

        uint8_t raw[16];                             // Generic payload
    } payload;                                       // 16 bytes
} Event;  // Total: 22 bytes
```

#### Memory Budget

**Event Bus Components:**
- Event Queue (8 events × 22 bytes): **176 bytes**
- Subscriber Table (11 event types × 4 subscribers max): **176 bytes**
- Module Overhead (queue pointers, indices): **54 bytes**
- **Total SRAM:** **406 bytes** (0.078% of 520KB)

**Flash Memory:**
- Event bus implementation: **~3.5KB** (0.09% of 4MB)

#### Benefits

**Reduced Module Coupling:**
- Hardware modules publish events without knowing subscribers
- State machine subscribes to events without polling hardware
- New subscribers don't require publisher changes

**Easier Testing:**
- Mock events for unit testing state transitions
- Inject test events without hardware present
- Replay event sequences for debugging

**Better Scalability:**
- Add new event types without modifying existing modules
- Multiple subscribers per event type
- Centralised event logging for diagnostics

**Improved Maintainability:**
- Clear data flow between modules
- Single location for event type definitions
- Self-documenting system behaviour

#### Event Flow Diagram

```
Event-Driven Communication Architecture:

Publishers                    Event Bus                  Subscribers
┌───────────────┐            ┌─────────┐              ┌──────────────┐
│ nfc_handler   │──publish──►│  Queue  │──dispatch──►│  game.cpp    │
│               │            │ [8 evts]│              │ (on_nfc_*)   │
└───────────────┘            └─────────┘              └──────────────┘

┌───────────────┐                                     ┌──────────────┐
│ audio_player  │──publish────────────────dispatch──►│  game.cpp    │
│               │                                     │ (on_audio_*) │
└───────────────┘                                     └──────────────┘

┌───────────────┐                                     ┌──────────────┐
│ game.cpp      │──publish────────────────dispatch──►│ data_logger  │
│ (state trans) │                                     │ (log events) │
└───────────────┘                                     └──────────────┘

Benefits: Modules depend on event interface, not each other
Trade-off: Adds ~406 bytes SRAM, slight event latency (~10ms)
```

**Communication Pattern:** Modules publish events (NFC detected, audio complete) and subscribe to events they care about, reducing direct dependencies between modules. The event bus acts as a message broker, routing events from publishers to registered subscribers.

---

## 4. Data Structures

### NFC Tag Format

**Hardware:** NTAG215 NFC tags (504 bytes user memory, ISO14443A)

**UID Mapping Table** (stored in config.h):
```c
typedef enum {
    MOOD_HAPPY = 0,
    MOOD_SAD,
    MOOD_ANGRY,
    MOOD_WORRIED,
    MOOD_FRUSTRATED,
    MOOD_CALM,
    NUM_MOODS
} MoodCategory;

typedef struct {
    uint8_t uid[7];          // 7-byte NFC UID
    MoodCategory mood;       // Mapped mood category
    const char* display_name; // Human-readable name
} NfcMoodMapping;

const NfcMoodMapping nfc_mappings[] = {
    {{0x04, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6}, MOOD_HAPPY,      "Happy"},
    {{0x04, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0xA1}, MOOD_SAD,        "Sad"},
    {{0x04, 0xC3, 0xD4, 0xE5, 0xF6, 0xA1, 0xB2}, MOOD_ANGRY,      "Angry"},
    {{0x04, 0xD4, 0xE5, 0xF6, 0xA1, 0xB2, 0xC3}, MOOD_WORRIED,    "Worried"},
    {{0x04, 0xE5, 0xF6, 0xA1, 0xB2, 0xC3, 0xD4}, MOOD_FRUSTRATED, "Frustrated"},
    {{0x04, 0xF6, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5}, MOOD_CALM,       "Calm"}
};
```

### Activity Metadata

**activities.json** structure:
```json
{
  "activities": [
    {
      "id": 1,
      "mood": "angry",
      "name": "Dragon Breath",
      "file_path": "/audio/angry/1_dragon_breath.mp3",
      "duration_seconds": 180,
      "morning": true,
      "afternoon": true,
      "evening": true,
      "bedtime": false,
      "type": "breathing"
    },
    ...
  ]
}
```

**C++ Structure** (activity_manager.h):
```c
typedef enum {
    TIME_MORNING = 0,    // 06:00-11:59
    TIME_AFTERNOON,      // 12:00-16:59
    TIME_EVENING,        // 17:00-20:59
    TIME_BEDTIME         // 21:00-05:59
} TimeOfDay;

typedef struct {
    uint8_t id;
    MoodCategory mood;
    char name[32];
    char file_path[64];
    uint16_t duration_seconds;
    bool time_flags[4];  // Indexed by TimeOfDay enum
    char type[16];       // "breathing", "movement", "mindfulness", "storytelling"
} Activity;
```

### Session Log Format

**sessions.csv** columns:
```csv
timestamp,mood,activity_id,activity_name,duration_seconds,completed,time_of_day
2026-02-09T14:23:10,angry,1,Dragon Breath,180,true,afternoon
2026-02-09T19:45:32,calm,44,Starlight Relaxation,240,false,evening
```

**C++ Structure** (data_logger.h):
```c
typedef struct {
    uint32_t timestamp;      // Unix timestamp (converted to ISO 8601 when writing CSV)
    MoodCategory mood;
    uint8_t activity_id;
    char activity_name[32];
    uint16_t duration_seconds;
    bool completed;          // false if cancelled early
    TimeOfDay time_of_day;
} SessionLog;
```

**Note:** The `timestamp` field stores Unix epoch seconds internally, but the CSV writer converts it to ISO 8601 format (YYYY-MM-DDTHH:MM:SS) for human readability.

### Activity History

**Purpose:** Prevent immediate repeats of same activity for a mood

**Structure** (activity_manager.cpp):
```c
#define HISTORY_SIZE 5

typedef struct {
    uint8_t recent_activities[NUM_MOODS][HISTORY_SIZE]; // Last 5 activity IDs per mood
    uint8_t history_index[NUM_MOODS];                    // Circular buffer index
} ActivityHistory;

static ActivityHistory activity_history = {0};
```

---

## 5. File Organisation on SD Card

### Directory Structure
```
/                           # Root of SD card
├── activities.json         # Activity metadata (10KB)
├── sessions.csv           # Session logs (grows over time, ~10KB/month)
└── audio/
    ├── system/
    │   ├── startup.mp3         # "Hello, how are you feeling today?" (3s)
    │   ├── error.mp3           # Gentle error tone (2s)
    │   ├── completion.mp3      # Completion chime (2s)
    │   └── low_battery.mp3     # "Time to charge me" (3s)
    ├── angry/
    │   ├── 1_dragon_breath.mp3         # (3min)
    │   ├── 2_volcano_stomp.mp3         # (4min)
    │   ├── 3_roar_release.mp3          # (2min)
    │   ├── 4_power_poses.mp3           # (3min)
    │   ├── 5_angry_scribble.mp3        # (5min)
    │   ├── 6_pillow_punch.mp3          # (3min)
    │   ├── 7_emotion_map.mp3           # (4min)
    │   └── 8_feelings_letter.mp3       # (5min)
    ├── worried/
    │   ├── 9_bubble_breath.mp3         # (3min)
    │   ├── 10_worry_jar.mp3            # (4min)
    │   ├── 11_cloud_watching.mp3       # (3min)
    │   ├── 12_butterfly_hug.mp3        # (2min)
    │   ├── 13_worry_friend.mp3         # (5min)
    │   ├── 14_safe_space.mp3           # (4min)
    │   ├── 15_hand_squeeze.mp3         # (3min)
    │   └── 16_worry_checklist.mp3      # (5min)
    ├── sad/
    │   ├── 17_rainbow_breath.mp3       # (3min)
    │   ├── 18_comfort_corner.mp3       # (4min)
    │   ├── 19_kindness_list.mp3        # (3min)
    │   ├── 20_colour_feelings.mp3      # (5min)
    │   ├── 21_gratitude_jar.mp3        # (4min)
    │   ├── 22_cosy_nest.mp3            # (3min)
    │   ├── 23_feelings_journal.mp3     # (5min)
    │   └── 24_gentle_stretch.mp3       # (4min)
    ├── happy/
    │   ├── 25_sunshine_dance.mp3       # (3min)
    │   ├── 26_joy_journal.mp3          # (4min)
    │   ├── 27_silly_sounds.mp3         # (3min)
    │   ├── 28_gratitude_walk.mp3       # (4min)
    │   ├── 29_happy_memory.mp3         # (3min)
    │   └── 30_kindness_plan.mp3        # (5min)
    ├── frustrated/
    │   ├── 31_progressive_relaxation.mp3 # (5min)
    │   ├── 32_problem_detective.mp3      # (4min)
    │   ├── 33_reset_routine.mp3          # (3min)
    │   ├── 34_break_time.mp3             # (3min)
    │   ├── 35_frustration_scale.mp3      # (4min)
    │   ├── 36_help_asking.mp3            # (3min)
    │   ├── 37_strategy_cards.mp3         # (5min)
    │   └── 38_calm_down_kit.mp3          # (4min)
    └── calm/
        ├── 39_body_scan.mp3              # (5min)
        ├── 40_peaceful_place.mp3         # (4min)
        ├── 41_slow_breathing.mp3         # (3min)
        ├── 42_nature_sounds.mp3          # (5min)
        ├── 43_mindful_listening.mp3      # (3min)
        └── 44_starlight_relaxation.mp3   # (4min)
```

### Audio File Specifications
- **Format:** MP3 (MPEG-1 Audio Layer 3)
- **Sample Rate:** 44.1kHz
- **Bit Depth:** 16-bit
- **Channels:** Mono
- **Bitrate:** 128kbps CBR (constant bitrate)
- **File Size:** ~1MB per minute (typical 3-5MB per activity)
- **Total Storage:** ~150MB for all 44 activities + system sounds

### File Naming Convention
- **Pattern:** `{id}_{snake_case_name}.mp3`
- **Example:** `1_dragon_breath.mp3`, `43_mindful_listening.mp3`
- **Benefits:** Sortable by ID, human-readable, filesystem-safe

---

## 6. Activity Selection Algorithm

### Input Parameters
1. **Mood Category** (from NFC token)
2. **Current Time** (hour from ESP32 RTC)
3. **Activity History** (last 5 activities per mood)

### Selection Process

```c
// Pseudocode for activity_manager.cpp
Activity* select_activity(MoodCategory mood, TimeOfDay time_of_day) {
    // Step 1: Filter by mood
    Activity* candidates = filter_by_mood(mood);  // ~6-8 activities

    // Step 2: Filter by time-of-day appropriateness
    candidates = filter_by_time(candidates, time_of_day);  // ~4-6 activities

    // Step 3: Remove recently played activities (last 5 for this mood)
    candidates = remove_recent(candidates, mood);  // ~2-4 activities

    // Step 4: Random selection from remaining candidates
    if (candidates.size() > 0) {
        return random_select(candidates);
    }

    // Fallback: Reset history if all filtered out (rare edge case)
    clear_history_for_mood(mood);
    return select_activity(mood, time_of_day);  // Recursive retry
}
```

### Time-of-Day Filtering

| Time Period | Hours (24hr) | Activity Characteristics |
|-------------|--------------|--------------------------|
| Morning | 06:00-11:59 | Energising, preparatory (morning breathing, focus activities) |
| Afternoon | 12:00-16:59 | Active, engaging (movement, interactive) |
| Evening | 17:00-20:59 | Calming transition (moderate energy, reflection) |
| Bedtime | 21:00-05:59 | Very calming, sleep-prep (body scan, gentle breathing) |

**Example:** "Dragon Breath" (angry, breathing) is appropriate for morning/afternoon/evening but NOT bedtime.

### History Management

**Add to History:**
```c
void add_to_history(MoodCategory mood, uint8_t activity_id) {
    uint8_t idx = activity_history.history_index[mood];
    activity_history.recent_activities[mood][idx] = activity_id;
    activity_history.history_index[mood] = (idx + 1) % HISTORY_SIZE;
}
```

**Check if Recent:**
```c
bool is_recent(MoodCategory mood, uint8_t activity_id) {
    for (int i = 0; i < HISTORY_SIZE; i++) {
        if (activity_history.recent_activities[mood][i] == activity_id) {
            return true;
        }
    }
    return false;
}
```

### Fallback Strategy

If all activities filtered out (very rare):
1. Clear history for that specific mood
2. Retry selection (will now have full pool)
3. If still fails (data corruption?), trigger ERROR state

---

## 7. Non-Blocking Design

### Audio Playback

**Library:** ESP32-audioI2S v2.0.0

**Pattern:**
```c
// In audio_player.cpp
#include "Audio.h"

Audio audio;

void audio_init() {
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);  // GPIO25, GPIO26, GPIO27
    audio.setVolume(18);  // 0-21 (18 = comfortable listening level)
}

void audio_play(const char* file_path) {
    audio.connecttoFS(SD, file_path);  // Non-blocking setup
}

void audio_loop() {
    audio.loop();  // MUST be called every iteration (~100ms max gap)
}

bool audio_is_running() {
    return audio.isRunning();
}

void audio_stop() {
    audio.stopSong();
}
```

**Key Point:** `audio.loop()` is called every iteration of the main loop (alongside game_update()), ensuring <100ms latency.

### LED Animations

**Pattern:** Separate state machine for LED effects

```c
// In led_controller.cpp
typedef enum {
    LED_IDLE,           // Slow white pulse
    LED_DETECTED,       // Quick green flash
    LED_BREATHING,      // Mood-specific colour breathing
    LED_SPARKLE,        // Completion sparkle
    LED_ERROR           // Slow red pulse
} LedAnimationState;

static LedAnimationState led_state = LED_IDLE;
static uint32_t animation_start = 0;
static uint8_t animation_frame = 0;

// Note: For WS2812B ring with 16 LEDs
#define NUM_LEDS 16

void led_update() {
    switch (led_state) {
        case LED_BREATHING: {
            uint32_t elapsed = millis() - animation_start;
            uint8_t brightness = (sin((elapsed / 2000.0) * 2 * PI) + 1) * 127;  // 0-255 sine wave

            // Set all 16 LEDs to same colour/brightness
            for (int i = 0; i < NUM_LEDS; i++) {
                pixels.setPixelColor(i, pixels.Color(
                    mood_colour.r * brightness / 255,
                    mood_colour.g * brightness / 255,
                    mood_colour.b * brightness / 255
                ));
            }
            break;
        }
        // ... other animations
    }
    pixels.show();  // Non-blocking update
}
```

**Called from:** Main loop (every iteration alongside game_update())

### Power-Saving Brightness Control

**Strategy:** Dynamic brightness limiting based on game state and battery level.

**Brightness Levels:**
```c
#define LED_BRIGHTNESS_ACTIVE 255      // 100% during activities
#define LED_BRIGHTNESS_IDLE 128        // 50% during idle pulsing
#define LED_BRIGHTNESS_LOW_BATTERY 64  // 25% when battery < 20%
```

**State-Based Brightness:**

| Game State | Max Brightness | LED Current | Power Savings |
|------------|---------------|-------------|---------------|
| IDLE | 50% (128) | 32mA avg | -32mA vs full |
| NFC_DETECTED | 100% (255) | 240mA peak | Brief flash, OK |
| PLAYING_ACTIVITY | 100% (255) | 240mA avg | Engaging experience |
| ACTIVITY_COMPLETE | 100% (255) | 240mA peak | Brief (2s), OK |
| ERROR | 50% (128) | 32mA avg | Gentle indication |
| LOW_BATTERY | 25% (64) | 16mA avg | Maximum conservation |

**Implementation:**
```c
void led_set_brightness_from_battery(float voltage) {
    if (voltage < 3.4) {
        pixels.setBrightness(LED_BRIGHTNESS_LOW_BATTERY);
    } else if (current_state == STATE_IDLE || current_state == STATE_ERROR) {
        pixels.setBrightness(LED_BRIGHTNESS_IDLE);
    } else {
        pixels.setBrightness(LED_BRIGHTNESS_ACTIVE);
    }
}
```

**Battery Life Impact:**
- Idle current reduction: 64mA → 32mA (50% saving)
- Total system current: 384mA → 352mA average
- **Battery life improvement:** 5.2hrs → 5.7hrs continuous (+30min, ~10% increase)

**Note:** Full brightness (100%) retained for activities and celebrations to maintain engaging user experience.

### NFC Polling

**Non-Blocking Pattern:**
```c
// In nfc_handler.cpp
static uint32_t last_poll = 0;
const uint32_t POLL_INTERVAL = 100;  // 100ms

bool nfc_poll(uint8_t uid[7]) {
    if (millis() - last_poll < POLL_INTERVAL) {
        return false;  // Too soon, skip this iteration
    }
    last_poll = millis();

    // readPassiveTargetID with 0ms timeout = non-blocking
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 0)) {
        return true;  // Token detected
    }
    return false;  // No token
}
```

### NFC Retry Logic & Debouncing

**Debounce Detection (in IDLE state):**
```c
// In idle_update()
static uint32_t debounce_start = 0;

if (nfc_token_detected()) {
    if (!debounce_start) {
        debounce_start = millis();
    } else if (millis() - debounce_start >= 100) {
        // Token stable for 100ms
        game_transition_to(STATE_NFC_DETECTED);
    }
} else {
    debounce_start = 0;  // Reset if removed
}
```

**Retry Logic (in NFC_DETECTED state):**
```c
// In nfc_detected_update()
static uint8_t attempt_count = 0;
static uint32_t retry_timestamp = 0;

if (millis() - retry_timestamp < 200) {
    return;  // Wait between attempts
}

if (nfc_read_uid(uid_buffer)) {
    attempt_count = 0;
    game_transition_to(STATE_VALIDATING);
} else {
    attempt_count++;
    retry_timestamp = millis();

    if (attempt_count >= 3) {
        attempt_count = 0;
        handle_error(ERROR_NFC_READ_TIMEOUT);
        game_transition_to(STATE_ERROR);
    }
}
```

**Timing Breakdown:**
- Debounce delay: 100ms (ensures stable token presence)
- Read attempt: Up to 1000ms per attempt
- Retry delay: 200ms between attempts
- **Total worst-case:** 100ms + (3 × 1200ms) = 3.7 seconds
- **Typical success:** 100ms + 200ms = 300ms (first attempt succeeds)

**Benefits:**
- ✅ Prevents false positives from quick token passes
- ✅ Handles temporary RF issues (orientation, interference)
- ✅ Child-friendly timeout (1s per attempt, not 500ms)
- ✅ Graceful degradation (3 attempts before error)

### Main Loop Structure

```c
// In main.cpp
void loop() {
    wdt_reset();              // Reset watchdog (4-second timeout)

    game_update();            // State machine update
    audio_loop();             // Audio library update
    led_update();             // LED animation update
    event_bus_process();      // Process and dispatch events

    // No delay() calls anywhere
}
```

**Event Bus Integration:** The architecture uses an event bus to reduce coupling between modules. Hardware modules (NFC, audio) publish events instead of being polled, and the state machine subscribes to these events. The `event_bus_process()` function dispatches queued events synchronously (non-blocking, <200µs for 8 events). See Section 3.5 for detailed event bus architecture including event types, memory overhead (~406 bytes), and publisher/subscriber pattern.

**Watchdog Timer:**
- Timeout: 4 seconds (WDTO_4S)
- Reset every loop iteration
- Prevents infinite loops or hung states
- All animations/operations designed to complete within 1 second per frame
- Event processing completes well under 200µs per iteration

---

## 8. Memory Budget

### Flash Memory (4MB Total)

| Component | Size | Percentage |
|-----------|------|------------|
| ESP32 Core (no WiFi/BLE) | ~400KB | 10% |
| Adafruit PN532 Library | ~50KB | 1.25% |
| Adafruit NeoPixel Library | ~10KB | 0.25% |
| ESP32-audioI2S Library | ~200KB | 5% |
| ArduinoJson Library | ~30KB | 0.75% |
| Application Code | ~200KB | 5% |
| Event Bus Implementation | ~3.5KB | 0.09% |
| **Used Total** | **~0.893MB** | **22.3%** |
| **Free** | **~3.107MB** | **77.7%** |

**Notes:**
- WiFi/BLE stacks NOT compiled in (saves ~450KB Flash)
- No static audio buffer allocated in Flash (streamed from SD card)
- Plenty of headroom for future features (connectivity: BLE +80KB, WiFi +250KB)

### SRAM (520KB Total)

| Component | Size | Percentage |
|-----------|------|------------|
| ESP32 System (no WiFi/BLE) | ~80KB | 15.4% |
| Audio Decode Buffer | ~20KB | 3.8% |
| JSON Parser Buffer (temp) | ~8KB | 1.5% |
| Activity Metadata Array | ~5KB | 1% |
| Activity History | ~1KB | 0.2% |
| NFC Buffers | ~2KB | 0.4% |
| NFC Retry State | ~29 bytes | 0.006% |
| LED Frame Buffer | ~768 bytes | 0.15% |
| LED Power Config | ~6 bytes | 0.001% |
| Event Bus (queue + subscribers) | ~406 bytes | 0.078% |
| Stack + Heap Overhead | ~15KB | 2.9% |
| **Used Total** | **~132.04KB** | **25.4%** |
| **Free** | **~387.96KB** | **74.6%** |

**Notes:**
- Heap fragmentation risk low (most allocations static or done at init)
- ArduinoJson uses ~8KB for activities.json parsing (released after parse, not persistent)
- No dynamic string allocations in hot paths
- Audio decode buffer (20KB) is the working buffer for MP3 decompression
- Event bus overhead is minimal (~406 bytes) for significant architectural benefit
- NFC retry logic adds 29 bytes (attempt counter, debounce timer, retry timestamp)
- LED brightness control adds 6 bytes (power mode, max brightness, power budget)
- Total enhancement overhead: 35 bytes (negligible, <0.01% of SRAM)
- Ample headroom for future connectivity features (BLE: +15KB SRAM, WiFi: +40KB SRAM)

### SD Card (2GB Minimum)

| Content | Size |
|---------|------|
| Audio Files (44 activities) | ~150MB |
| System Sounds | ~5MB |
| activities.json | ~10KB |
| sessions.csv (1 year) | ~120KB |
| **Total Used** | **~155MB** |
| **Free** | **~1845MB** |

---

## 9. Library Dependencies

### platformio.ini Configuration

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

lib_deps =
    adafruit/Adafruit PN532@^1.3.1
    adafruit/Adafruit NeoPixel@^1.11.0
    https://github.com/schreibfaul1/ESP32-audioI2S.git#v2.0.0
    bblanchon/ArduinoJson@^6.21.3

build_flags =
    -DCORE_DEBUG_LEVEL=3          ; Enable debug logging
    -DBOARD_HAS_PSRAM=0           ; No external PSRAM
    -DCONFIG_ARDUHAL_LOG_DEFAULT_LEVEL_INFO
    -DCONFIG_BT_ENABLED=0         ; Disable Bluetooth
    -DCONFIG_WIFI_ENABLED=0       ; Disable WiFi (saves ~450KB Flash)

monitor_speed = 115200
upload_speed = 921600
```

### Library Details

**Adafruit PN532 v1.3.1**
- Purpose: NFC/RFID communication
- Features: I2C/SPI support, ISO14443A cards, non-blocking read support
- Licence: BSD
- Documentation: https://github.com/adafruit/Adafruit-PN532

**Adafruit NeoPixel v1.11.0**
- Purpose: WS2812B LED control
- Features: Hardware-accelerated timing on ESP32, brightness control, HSV colour space
- Licence: LGPL v3
- Documentation: https://github.com/adafruit/Adafruit_NeoPixel

**ESP32-audioI2S v2.0.0**
- Purpose: MP3/AAC/FLAC audio decoding and I2S output
- Features: Non-blocking playback, built-in decoder, SD card support, volume control
- Licence: GPL v3
- Documentation: https://github.com/schreibfaul1/ESP32-audioI2S

**ArduinoJson v6.21.3**
- Purpose: JSON parsing for activities.json
- Features: Efficient parsing, minimal heap usage, filter support
- Licence: MIT
- Documentation: https://arduinojson.org/

---

## 10. Error Handling

### Error Types Enum

```c
typedef enum {
    ERROR_NONE = 0,
    ERROR_SD_INIT_FAILED,        // SD card not detected or mount failed
    ERROR_SD_READ_FAILED,        // File read error during playback
    ERROR_NFC_INIT_FAILED,       // PN532 not responding on I2C bus
    ERROR_NFC_READ_TIMEOUT,      // UID read timeout (3 failed attempts, 1000ms each)
    ERROR_AUDIO_INIT_FAILED,     // I2S DAC initialisation failed
    ERROR_AUDIO_FILE_NOT_FOUND,  // MP3 file path invalid
    ERROR_JSON_PARSE_FAILED,     // activities.json malformed
    ERROR_INVALID_UID,           // NFC UID not in mapping table
    ERROR_NO_ACTIVITIES,         // No activities match filters (should never happen)
    ERROR_BATTERY_CRITICAL,      // Battery <3.3V (immediate shutdown required)
    ERROR_WATCHDOG_RESET         // Detected previous watchdog reset
} ErrorCode;
```

### Error Handling Strategy

| Error Category | Strategy | Recovery |
|----------------|----------|----------|
| **SD Card Failures** | Play cached error sound from Flash, LED red pulse | Retry init on next token tap |
| **NFC Errors** | Skip to ERROR state, timeout 5s | Auto-return to IDLE, retry init |
| **Audio Playback** | Skip to next activity (random from mood), log error | Continue operation |
| **JSON Parse** | Use hardcoded fallback activity list (3 per mood) | Notify parent via LED (amber) |
| **Battery Critical** | Play warning, save logs, deep sleep | Prevent data loss |
| **Watchdog Reset** | Log event, LED flash pattern (diagnostic), continue | Safe recovery |

### Error Function

```c
// In game.cpp
void handle_error(ErrorCode error) {
    current_error = error;
    game_transition_to(STATE_ERROR);

    // Log error with timestamp
    log_error(error);

    // Play appropriate error sound (non-scary for children)
    switch (error) {
        case ERROR_SD_INIT_FAILED:
            audio_play_from_flash(error_sound_gentle);  // "Oops, let's try that again"
            break;
        case ERROR_BATTERY_CRITICAL:
            audio_play_from_flash(low_battery_sound);   // "Time to charge me up"
            prepare_deep_sleep();
            break;
        // ... other cases
    }

    // LED feedback
    led_set_animation(LED_ERROR);
}
```

### Graceful Degradation

**Scenario: SD Card Removed During Playback**
1. Audio library detects read failure
2. `audio_is_running()` returns false prematurely
3. STATE_PLAYING_ACTIVITY detects early completion
4. Transition to ERROR state with ERROR_SD_READ_FAILED
5. Play cached error sound from Flash memory
6. Return to IDLE after 5s timeout
7. Next token tap will retry SD init

**Scenario: NFC Tag Removed During Activity**
1. NFC polling (in background) detects tag removal
2. STATE_PLAYING_ACTIVITY checks `nfc_is_present()` flag
3. If false, transition directly to IDLE (cancellation, not error)
4. Log session as incomplete (completed=false)
5. Activity can be re-triggered immediately

---

## 11. Testing Strategy

### Unit Testing

**Frameworks:** Unity (PlatformIO native test framework)

**Test Modules:**

1. **HAL Functions** (hardware.cpp)
   - Mock I2C/I2S/SPI peripherals
   - Test initialisation sequences
   - Verify pin configurations

2. **Activity Selection Algorithm** (activity_manager.cpp)
   - Test time-of-day filtering (boundary conditions: 05:59, 06:00, 11:59, 12:00, etc.)
   - Test history prevention (verify no immediate repeats)
   - Test fallback logic (all activities filtered out)
   - Test random distribution (10,000 iterations, verify uniform distribution)

3. **NFC UID Mapping** (nfc_handler.cpp)
   - Test all 6 valid UIDs map correctly
   - Test invalid UID returns error
   - Test UID comparison (byte-by-byte equality)

4. **Data Logging** (data_logger.cpp)
   - Test CSV formatting (commas, quotes, newlines)
   - Test timestamp generation
   - Mock SD write, verify correct file append

**Command:** `pio test -e native` (runs on host machine, no hardware required)

### Integration Testing

**Setup:** Full hardware breadboard prototype

**Test Cases:**

1. **Full Flow Test**
   - Tap token → hear activity → remove token → return to idle
   - Verify: LED animations, audio playback, logging

2. **State Transition Coverage**
   - Trigger all 8 states manually
   - Verify: Enter/update/exit functions called correctly
   - Check: No missed cleanup (LED states, button clearing)

3. **Error Recovery Test**
   - Remove SD card during playback → verify error state → reinsert → verify recovery
   - Block NFC antenna → verify timeout → unblock → verify retry

4. **Timing Accuracy**
   - Verify audio.loop() called every <100ms (oscilloscope on test pin)
   - Verify LED update rate (visual inspection, should be smooth)
   - Verify NFC poll interval (100ms ±10ms)

5. **Watchdog Test**
   - Introduce deliberate infinite loop → verify watchdog reset
   - Check: Device recovers to IDLE state after reset

**Duration:** 2-3 days comprehensive testing

### Hardware Testing

**Test Points:**

1. **NFC Range and Reliability**
   - Measure max read distance (expected: 3-5cm)
   - Test 100 consecutive taps (success rate >98%)
   - Test with various NFC tag orientations

2. **Audio Quality**
   - Frequency response (should be flat 200Hz-8kHz)
   - Volume levels (comfortable for children, not too loud)
   - Distortion at max volume (THD <1%)

3. **Battery Runtime**
   - Full charge → continuous use (token every 10min) → measure runtime
   - Expected: 8-12 hours
   - Monitor voltage curve (should be linear discharge)

4. **Temperature**
   - Measure ESP32 temperature during 1-hour use (should be <60°C)
   - Check for thermal throttling (none expected at this load)

**Equipment:** Oscilloscope, multimeter, audio analyser, stopwatch

### Field Testing

**Participants:** 20 beta families with children aged 4-10

**Duration:** 6-8 weeks

**Data Collection:**
- Session logs (copied from SD card at end of testing period)
- Parent survey (weekly): ease of use, child engagement, technical issues, data access experience
- Child feedback (via parent): favourite activities, clarity of audio
- CSV readability feedback: Can parents understand the data format?

**Success Criteria:**
- >80% parent satisfaction
- >70% child engagement (use device >3x/week)
- <5% technical failure rate (errors, crashes)

**Iteration:** Refine activity content, fix bugs, tune selection algorithm based on feedback

---

## 12. Critical Design Decisions

### ESP32 vs Arduino Uno/Nano

**Decision:** Use ESP32 DevKit

**Rationale:**
- **SRAM:** 520KB vs 2KB (Arduino) → required for audio buffers and JSON parsing
- **Flash:** 4MB vs 32KB → required for libraries and application code
- **I2S Hardware:** Native I2S peripheral → 16-bit audio without CPU load
- **WiFi Built-In:** Enables future connectivity features without additional hardware
- **Clock Speed:** 240MHz vs 16MHz → smooth audio decoding and LED animations
- **Cost:** £5-8 vs £3-5 → minimal difference, huge capability gain

**Trade-offs:**
- Slightly higher power consumption (but offset by efficient peripherals)
- More complex development environment (PlatformIO vs Arduino IDE)
- Overkill for basic functionality (but future-proof for extensibility)

### I2S Audio DAC vs PWM

**Decision:** Use MAX98357A I2S DAC

**Rationale:**
- **Audio Quality:** 16-bit I2S vs 8-bit PWM → professional-grade audio
- **CPU Load:** Hardware I2S offloads decoding → no blocking, smooth animations
- **Filtering:** No external low-pass filter required (PWM needs it)
- **Volume Control:** Software-controlled volume without distortion
- **Library Support:** ESP32-audioI2S mature and well-documented

**Trade-offs:**
- +£3 per unit cost vs £0 for PWM
- Requires 3 GPIO pins vs 1 for PWM
- Slight increase in PCB complexity

### NFC (ISO14443A) vs RFID 125kHz

**Decision:** Use PN532 NFC Reader (ISO14443A)

**Rationale:**
- **Writeable Tags:** Can update UID-to-mood mapping later if needed
- **Ecosystem:** NFC tags ubiquitous (phone compatibility, sourcing easy)
- **Range:** 3-5cm "intentional tap" vs 10cm "accidental trigger" with RFID
- **Security:** NFC supports encryption (future-proof for multi-user profiles)
- **Library Support:** Adafruit PN532 library mature, I2C/SPI support

**Trade-offs:**
- +£8 per unit cost vs £2 for RFID reader
- Slightly more complex initialisation code
- Requires I2C bus (but ESP32 has multiple I2C controllers)

### SD Card vs Internal Flash Storage

**Decision:** Use MicroSD card (SPI interface)

**Rationale:**
- **Capacity:** 2GB+ vs 4MB internal → required for 150MB audio content
- **Easy Updates:** Parents can update content via computer (remove SD, copy files)
- **Readable Logs:** sessions.csv can be opened in Excel without special tools
- **Cost:** +£1 per unit vs £0 → minimal for huge flexibility
- **Failure Recovery:** If SD fails, replace card without firmware reflash

**Trade-offs:**
- Potential for SD card corruption (mitigated by append-only writes)
- Mechanical wear from insertion/removal (use spring-loaded socket)
- Requires SPI bus and 4 GPIO pins

### JSON vs Binary Configuration

**Decision:** Use JSON for activities.json

**Rationale:**
- **Human-Readable:** Parents/developers can edit with text editor
- **Extensible:** Easy to add new fields without breaking parser
- **ArduinoJson Efficiency:** Streaming parser uses minimal RAM (~8KB)
- **Debugging:** Easy to validate structure, spot errors
- **Standard:** Wide tooling support (validators, formatters)

**Trade-offs:**
- +2KB file size vs binary (negligible on 2GB SD card)
- Slightly slower parsing (but only done once at boot)
- Requires JSON library (+30KB Flash)

### CSV vs SQLite for Logs

**Decision:** Use CSV for sessions.csv

**Rationale:**
- **Simplicity:** Append-only writes, no corruption risk
- **Universally Readable:** Excel, Google Sheets, Python pandas
- **No Library Overhead:** Custom CSV writer ~20 lines of code
- **Low Risk:** Even if file corrupted, partial data recoverable
- **No Indexing Needed:** Time-series data, sequential reads fine

**Trade-offs:**
- No relational queries (but not needed for this use case)
- Slightly larger file size (CSV headers repeated)
- Manual parsing required for analysis (vs SQL queries)

### WS2812B vs Analogue RGB LEDs

**Decision:** Use WS2812B addressable LED ring (16 LEDs)

**Rationale:**
- **Full Colour Control:** 16.7 million colours vs 7 basic colours (analogue RGB)
- **Mood-Specific Colours:** Can map each mood to unique hue/saturation
- **Animations:** Individual LED control enables breathing, sparkle, chase effects
- **Single Pin:** One GPIO vs 3 for RGB (PWM requires 3 timers)
- **Library Support:** Adafruit NeoPixel library well-optimised for ESP32

**Trade-offs:**
- +£3 per unit cost vs £1 for analogue RGB
- Requires 5V logic (need level shifter from ESP32's 3.3V)
- Slightly higher power consumption (64mA typical vs 20mA for RGB)

---

## 13. Diagrams

### System Block Diagram

```
                    ┌─────────────────────────────────────┐
                    │           ESP32 DevKit              │
                    │  (520KB SRAM, 4MB Flash, 240MHz)    │
                    │                                     │
                    │  ┌─────────────────────────────┐   │
                    │  │     State Machine Core      │   │
                    │  │  (Enter/Update/Exit Pattern)│   │
                    │  └──────────────┬──────────────┘   │
                    │                 │ subscribe         │
                    │                 ▼                   │
                    │  ┌─────────────────────────────┐   │
                    │  │       Event Bus (Queue)     │   │
                    │  │     [8 events, 406 bytes]   │   │
                    │  └──────────────┬──────────────┘   │
                    │                 ▲ publish          │
                    │         ┌───────┴───────┐          │
   ┌────────────┐   │  ┌─────┴─────┐ ┌───────┴──────┐  │   ┌────────────┐
   │            │◄──┼──┤NFC Handler│ │ Audio Player │◄─┼──►│ MAX98357A  │
   │  PN532 NFC │   │  │ (I2C I/O) │ │  (I2S I/O)   │  │   │  I2S DAC   │
   │   Reader   │   │  └───────────┘ └──────────────┘  │   │            │
   │            │   │    GPIO21/22      GPIO25/26/27    │   └──────┬─────┘
   └────────────┘   │                                   │          │
                    │  ┌─────────────┐ ┌──────────────┐ │          ▼
   ┌────────────┐   │  │ LED Control │ │ Activity Mgr │ │   ┌────────────┐
   │ WS2812B    │◄──┼──┤  (One-Wire) │ │ (Selection)  │ │   │  3W 4Ω     │
   │ LED Ring   │   │  └─────────────┘ └──────────────┘ │   │  Speaker   │
   │ (16 LEDs)  │   │       GPIO5                        │   └────────────┘
   └────────────┘   │                                    │
                    │  ┌─────────────┐ ┌──────────────┐ │
   ┌────────────┐   │  │ Data Logger │ │ Battery Mon  │ │
   │  MicroSD   │◄──┼──┤  (SPI I/O)  │ │  (ADC GPIO34)│ │
   │   Module   │   │  └─────────────┘ └──────────────┘ │
   │  (2GB+)    │   │    GPIO18/19/23/4                  │
   └────────────┘   └────────────────────────────────────┘
                                  ▲
                                  │ Power (3.3V)
                                  │
                    ┌─────────────┴─────────────┐
                    │   2000mAh LiPo + TP4056   │
                    │  (USB-C Charging, 3.7V)   │
                    └───────────────────────────┘
```

**Event-Driven Communication:** Modules communicate via the event bus rather than direct function calls. This reduces coupling and enables easier testing and scalability. See Section 3.5 for detailed event bus architecture.

**See also:**
- Section 3.5: Event Bus Architecture for event-driven communication details
- Section 13.3: Timing Diagram for typical session flow

### State Transition Diagram

```
                    ┌──────────────┐
                    │ Power On     │
                    └──────┬───────┘
                           ▼
                    ┌──────────────┐
              ┌────►│   IDLE       │◄──────┐
              │     └──────┬───────┘       │
              │            │ NFC Detected  │
              │            ▼               │
              │     ┌──────────────┐       │
              │     │NFC_DETECTED  │       │
              │     └──────┬───────┘       │
              │            │ UID Read      │
              │            ▼               │
              │     ┌──────────────┐       │
              │  ┌──┤ VALIDATING   │──┐    │
              │  │  └──────┬───────┘  │    │
              │  │         │ Valid    │    │
              │  │         ▼          │    │
              │  │  ┌──────────────┐  │    │
              │  │  │  SELECTING   │  │    │
              │  │  └──────┬───────┘  │    │
              │  │         │ Activity │    │
              │  │         │ Chosen   │    │
              │  │         ▼          │    │
              │  │  ┌──────────────┐  │    │
              │  │  │   PLAYING    │  │    │
Token Removed │  │  │   ACTIVITY   │  │    │ Timeout
Early         │  │  └──────┬───────┘  │    │ or Recovery
              │  │         │ Complete │    │
              │  │         ▼          │    │
              │  │  ┌──────────────┐  │    │
              │  │  │  ACTIVITY    │  │    │
              │  │  │  COMPLETE    │  │    │
              │  │  └──────┬───────┘  │    │
              │  │         │ Timeout  │    │
              │  │         │ (2s)     │    │
              └──┼─────────┴──────────┘    │
                 │                         │
                 │  ┌──────────────┐       │
                 └─►│    ERROR     │───────┘
                    └──────┬───────┘
                           │
                           │ Battery < 3.3V (or from IDLE)
                           ▼
                    ┌──────────────┐
                    │ LOW_BATTERY  │◄─── Can be entered from IDLE or ERROR
                    │ (Deep Sleep) │     when battery voltage < 3.4V
                    └──────────────┘
```

### Timing Diagram for Typical Session

```
Time (seconds)  0     1     2     3     4     5     6     ... 180   181   182   184
                │     │     │     │     │     │     │           │     │     │     │
NFC Token       │ TAP ─────────────────────────────────────── HOLD ──── REMOVE ───│
                │     │     │     │     │     │     │           │     │     │     │
State Machine   IDLE──┤DETC─┤VALD─┤SEL──┤PLAY────────────────────────┤COMP─┤IDLE─│
                │     │     │     │     │     │     │           │     │     │     │
LED Animation   Pulse─┤Flash┤     │Breath────────────────────────────┤Spark┤Pulse│
                │     │     │     │     │     │     │           │     │     │     │
Audio Output    ──────┼─────┼─────┼─────┤MP3 Playback (3 minutes)────┤Chime┤─────│
                │     │     │     │     │     │     │           │     │     │     │
SD Card         ──────┼─────┼─────┼READ─┤STREAM─────────────────────┤WRITE┤─────│
                │     │     │     │     │     │     │           │     │     │     │
Session Log     ──────┼─────┼─────┼─────┤ (activity running)   ├─────┤Log──│─────│
                │     │     │     │     │     │     │           │     │     │     │

DETC = NFC_DETECTED
VALD = VALIDATING
SEL = SELECTING
PLAY = PLAYING_ACTIVITY
COMP = ACTIVITY_COMPLETE
```

**Key Observations:**
1. **State transitions fast** (<1s from tap to audio start)
2. **No blocking delays** (all operations non-blocking)
3. **Logging happens at start and end** (not during playback)
4. **LED animations continuous** (breathing effect throughout activity)
5. **Token can be removed early** (transitions directly to IDLE, logs incomplete)

### Memory Map

```
┌─────────────────────────────────────────────┐
│           Flash Memory (4MB)                │
├─────────────────────────────────────────────┤
│  ESP32 Bootloader (0x1000)          16KB    │
│  Partition Table (0x8000)            4KB    │
│  NVS (Non-Volatile Storage)         20KB    │
│  OTA Data                            8KB    │
├─────────────────────────────────────────────┤
│  Application Partition (0x10000)  ~0.9MB    │
│    - ESP32 Core (no WiFi/BLE)     400KB     │
│    - Libraries (PN532, NeoPixel,  290KB     │
│      audioI2S, ArduinoJson)                 │
│    - Application Code             200KB     │
├─────────────────────────────────────────────┤
│  OTA Update Partition (future)     ~2MB     │  ◄─ Reserved for dual-boot OTA
└─────────────────────────────────────────────┘

┌─────────────────────────────────────────────┐
│          SRAM (520KB)                       │
├─────────────────────────────────────────────┤
│  ESP32 System (RTOS, drivers)       80KB    │
├─────────────────────────────────────────────┤
│  Application Heap                  440KB    │
│    - Audio Decode Buffer           20KB     │
│    - JSON Parser Buffer (temp)      8KB     │
│    - Activity Metadata Array        5KB     │
│    - Activity History               1KB     │
│    - NFC Buffers                    2KB     │
│    - Event Bus (queue + subs)     406 bytes │
│    - LED Frame Buffer             768 bytes │
│    - Stack + Misc                  15KB     │
│    - Free Heap                    ~388KB    │ ◄─ Headroom for future features
└─────────────────────────────────────────────┘
```

---

## Summary

This architecture document provides a comprehensive blueprint for implementing the Emotion Check-In Station. The design leverages proven patterns from the Light Chaser game (enter/exit/update state machine, non-blocking animations, HAL abstraction) while addressing the unique requirements of audio playback, NFC interaction, and data logging.

### Key Strengths:
- ✅ **Technically Feasible** - All components available, mature libraries, no blockers
- ✅ **Memory Conservative** - 22% Flash, 25% SRAM with substantial headroom for extensibility
- ✅ **Non-Blocking Design** - Watchdog timer compliant, smooth animations and audio
- ✅ **Professional Architecture** - State machine pattern scales well, easy to test and maintain
- ✅ **Future-Proof** - Plenty of headroom for connectivity, OTA updates, multi-user support
- ✅ **Child-Centred** - Immediate feedback, gentle error handling, engaging audio/visual design

---

*Document Version: 2.4*
*Last Updated: 2026-02-10*
*Author: Architecture planned by Claude (Sonnet 4.5)*

**Changelog:**
- v1.0 (2026-02-09): Initial architecture
- v1.1 (2026-02-10): **Simplified for MVP** - Local-only operation, no connectivity required. All data on removable SD card. Parents access sessions.csv via computer.
- v2.1 (2026-02-10): **Consistency fixes** - Fixed time-of-day boundaries, CSV examples, memory budgets, removed WiFi/BLE from MVP build. Removed future extensibility and development phases sections to focus on core architecture.
- v2.2 (2026-02-10): **Event Bus Architecture** - Added Section 3.5 documenting event-driven communication pattern. Updated main loop, memory budgets, and module breakdown to reflect event bus as core architectural component.
- v2.3 (2026-02-10): **Architecture Document Corrections** - Fixed event type count (11 types, not 20). Corrected subscriber table memory (176 bytes based on actual enum). Updated total event bus overhead (406 bytes, not 550 bytes). Corrected SRAM budget (132KB / 25.4%). Updated System Block Diagram to show event bus architecture. Improved STATE_LOW_BATTERY diagram clarity. Expanded GPIO2 bootstrap note. Added cross-references for improved navigation.
- v2.4 (2026-02-10): **Reliability & Power Enhancements** - Added NFC retry logic with 3-attempt strategy and 100ms debouncing for improved child usability. Implemented LED brightness limiting (50% during idle, 25% when low battery) to extend battery life by ~10% (5.2hrs → 5.7hrs). Updated power budget and memory calculations. Added Power-Saving Brightness Control subsection and NFC Retry Logic & Debouncing subsection.
