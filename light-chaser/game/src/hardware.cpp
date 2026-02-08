/******************************************************************************
 * HARDWARE.CPP - Hardware Abstraction Layer Implementation
 *
 * TUTORIAL: Complete Hardware Driver Suite for Embedded Systems
 *
 * This file implements all low-level hardware interactions for the Light
 * Chaser game. It's the most complex file in the codebase, demonstrating
 * professional embedded systems patterns used in production devices.
 *
 * WHAT YOU'LL LEARN:
 * - GPIO configuration and control (LEDs, buttons, buzzer)
 * - Button debouncing and edge detection algorithms
 * - Non-blocking animation state machines
 * - Parallel timing (multiple animations running simultaneously)
 * - I2C communication protocol
 * - EEPROM data persistence with validation
 * - Watchdog timer safety considerations
 *
 * FILE ORGANISATION (5 major sections):
 *
 * 1. GPIO CONTROL (Lines 60-165)
 *    - hardware_init(): Pin configuration
 *    - LED control: led_set(), led_clear_all()
 *    - Button input: button_just_pressed(), button_clear_state()
 *    - Basic sound: buzzer_tick(), buzzer_hit()
 *
 * 2. NON-BLOCKING ANIMATION SYSTEM (Lines 167-370) ⭐ MOST COMPLEX
 *    - AnimationState enum: IDLE, BULLSEYE, CELEBRATION, GAME_OVER
 *    - animation_update(): Main animation dispatcher (called every frame)
 *    - animation_start_*(): Animation initialisation functions
 *    - DEMONSTRATES: Parallel timing, state machines, cooperative multitasking
 *
 * 3. LCD DISPLAY (Lines 372-420)
 *    - I2C communication with 16×2 character LCD
 *    - display_show_*(): Different screen layouts
 *    - Flicker reduction techniques
 *
 * 4. EEPROM PERSISTENCE (Lines 422-500)
 *    - eeprom_read_high_score(): Load and validate persistent data
 *    - eeprom_write_high_score(): Save with magic byte + checksum
 *    - DEMONSTRATES: Data validation, corruption detection, wear levelling
 *
 * ARCHITECTURE HIGHLIGHTS:
 *
 * Non-Blocking Design:
 * - No delay() calls anywhere in this file
 * - All timing uses millis() timestamps
 * - Animations run as state machines
 * - Functions return immediately (< 100μs execution time)
 *
 * Parallel Animations:
 * - Buzzer and LEDs run on independent timers
 * - Multiple animations can overlap (e.g., tick sounds during celebration)
 * - See CELEBRATION animation for parallel timing example
 *
 * Memory Safety:
 * - All variables statically allocated (no malloc)
 * - Bounds checking on all array accesses
 * - Input validation on all public functions
 *
 * Watchdog Timer Safety:
 * - All functions complete in < 5ms (typically < 100μs)
 * - EEPROM writes (~3.3ms) are the slowest operation
 * - Total well within 4-second watchdog timeout
 *
 * READING GUIDE FOR BEGINNERS:
 * 1. Start with GPIO section (simple, familiar concepts)
 * 2. Read button_just_pressed() carefully (fundamental pattern)
 * 3. Skip animation system initially, return after understanding state machines
 * 4. Read game.cpp first to see how animations are used
 * 5. Come back to animation_update() with context of how it's called
 *
 * Related files:
 * - hardware.h: Public interface (what game.cpp sees)
 * - config.h: All pin numbers and timing constants
 * - game.cpp: Uses this hardware abstraction layer
 ******************************************************************************/

#include "hardware.h"
#include "config.h"
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

/******************************************************************************
 * SECTION 1: GPIO CONTROL - Basic Input/Output
 *
 * GPIO = General Purpose Input/Output
 * Digital pins that can be configured as inputs (read voltage) or outputs
 * (apply voltage). Arduino Uno has 14 digital GPIO pins (D0-D13).
 ******************************************************************************/

// Button state tracking for edge detection
// Static variables persist between function calls (not on stack)
static bool last_button_state = false;      // Previous button reading
static uint32_t last_debounce_time = 0;     // Timestamp of last detected press

// LCD object (I2C communication)
static LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

/**
 * hardware_init - One-time hardware initialisation
 *
 * Called from main.cpp:setup() before any other hardware operations.
 *
 * RESPONSIBILITIES:
 * - Configure all GPIO pins (set as INPUT or OUTPUT)
 * - Initialise peripherals (LCD via I2C)
 * - Set safe initial states (LEDs off, no sounds)
 *
 * EMBEDDED CONCEPT: Pin Configuration
 *
 * Microcontroller pins start in undefined states. We MUST configure them
 * before use. pinMode() configures direction and internal circuitry:
 *
 * pinMode(pin, OUTPUT):
 *   - Pin can source current (HIGH = 5V) or sink current (LOW = 0V)
 *   - Typical use: LEDs, buzzer, motor drivers
 *   - Output impedance: ~25Ω (can drive reasonable loads)
 *
 * pinMode(pin, INPUT):
 *   - Pin reads external voltage (HIGH if >3V, LOW if <1.5V)
 *   - High impedance (~100MΩ) - doesn't affect external circuit
 *   - Floating if unconnected (reads random values!)
 *   - Typical use: Sensors with external pull-up/down resistors
 *
 * pinMode(pin, INPUT_PULLUP):
 *   - Same as INPUT but with internal ~20kΩ pull-up resistor enabled
 *   - Pulls pin to 5V when nothing connected
 *   - Perfect for buttons: button connects pin to GND when pressed
 *   - No external resistor needed (saves components!)
 *
 * BUTTON WIRING (INPUT_PULLUP):
 *
 *        5V
 *         │
 *         ┝ 20kΩ (internal pull-up resistor)
 *         │
 *   Pin ──┤
 *         │
 *        [Button]  ← Tactile switch
 *         │
 *        GND
 *
 * Button released: Pin pulled to 5V by resistor → reads HIGH
 * Button pressed: Pin connected to GND → reads LOW
 *
 * LED INITIALISATION PATTERN:
 *
 * We have 8 LEDs on consecutive pins (2-9). Instead of 8 separate pinMode()
 * calls, we use a loop:
 *
 *   for (uint8_t i = 0; i < 8; i++) {
 *       pinMode(LED_PIN_START + i, OUTPUT);  // Configure pin 2, 3, ..., 9
 *   }
 *
 * This is more maintainable. To change to 10 LEDs, just update NUM_LEDS in
 * config.h. No code changes needed here.
 */
void hardware_init(void) {
    // Initialise LED pins as outputs
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        pinMode(LED_PIN_START + i, OUTPUT);  // Set pin as output
        digitalWrite(LED_PIN_START + i, LOW);  // Start with LED off (0V)
    }

    // Initialise button with internal pull-up resistor (active-low)
    // See button wiring diagram above for how this works
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Initialise button edge detection state to current physical state
    // This prevents detecting a "press" on boot if button happens to be held
    last_button_state = digitalRead(BUTTON_PIN);

    // Initialise buzzer pin as output
    pinMode(BUZZER_PIN, OUTPUT);
    noTone(BUZZER_PIN);  // Ensure no tone playing (stop any residual PWM)

    // Initialise I2C LCD display
    // I2C pins (A4/A5) are automatically configured by Wire library
    lcd.init();       // Initialise LCD controller, establish I2C communication
    lcd.backlight();  // Turn on backlight LED (makes display visible)
    lcd.clear();      // Clear display buffer (blank screen)
}

/**
 * led_set - Turn a single LED on or off
 * @param position: LED index (0-7)
 * @param state: true = ON (5V), false = OFF (0V)
 *
 * BOUNDS CHECKING:
 *
 * We validate position < NUM_LEDS before accessing hardware. Why?
 *
 * Without checking:
 *   led_set(10, true);  // position 10 doesn't exist!
 *   digitalWrite(LED_PIN_START + 10, HIGH);  // Write to pin 12 (wrong!)
 *   // Might be unconnected, or worse, connected to something else!
 *
 * With checking:
 *   if (position >= NUM_LEDS) return;  // Silently ignore invalid positions
 *   // Safe: won't corrupt other pins or crash
 *
 * EMBEDDED SAFETY PRINCIPLE:
 * Always validate inputs from higher-level code. Embedded systems have no
 * memory protection - invalid access doesn't throw an exception, it corrupts
 * hardware state or memory. Defensive programming is essential.
 */
void led_set(uint8_t position, bool state) {
    // Bounds checking (prevent invalid pin access)
    if (position >= NUM_LEDS) {
        return;  // Silently ignore invalid positions (fail-safe)
    }

    // Calculate physical pin number: LED 0 = pin 2, LED 1 = pin 3, etc.
    uint8_t pin = LED_PIN_START + position;

    // Set pin voltage: HIGH = 5V (LED on), LOW = 0V (LED off)
    digitalWrite(pin, state ? HIGH : LOW);
}

/**
 * led_clear_all - Turn off all LEDs
 *
 * Bulk operation equivalent to calling led_set(i, false) for all LEDs.
 * Used during:
 * - State transitions (clean visual state)
 * - Chase LED updates (turn off old position before lighting new)
 * - Game over animation (stop chase before flash begins)
 */
void led_clear_all(void) {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        digitalWrite(LED_PIN_START + i, LOW);
    }
}

/******************************************************************************
 * CRITICAL EMBEDDED PATTERN: Button Debouncing + Edge Detection
 *
 * This function demonstrates TWO essential techniques that every embedded
 * developer must master:
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * TECHNIQUE 1: EDGE DETECTION
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * PROBLEM: Detecting button STATE vs button PRESS EVENT
 *
 * State detection (what NOT to do):
 *   if (digitalRead(BUTTON_PIN) == LOW) {
 *       score++;  // ❌ BUG! Increments 1000+ times per second while held!
 *   }
 *
 * Why it fails:
 *   - Loop runs 1000-20000 times/second
 *   - Button held for 1 second = 1000-20000 increments!
 *   - We want ONE increment per button PRESS, not per loop iteration
 *
 * SOLUTION: Edge detection (detect transitions)
 *
 * State diagram:
 *
 *   Button Released        Button Pressed         Button Released
 *        HIGH      ────────►    LOW      ────────►      HIGH
 *                  PRESS EVENT          RELEASE EVENT
 *                       ↑                     ↑
 *                  Detect this!         Ignore this
 *
 * We track the PREVIOUS state and detect rising edges (LOW → HIGH transition
 * from button perspective, HIGH → LOW from pin perspective due to pull-up).
 *
 * Algorithm:
 *   current_state = read_button();
 *   if (current_state == PRESSED && last_state == RELEASED) {
 *       // Rising edge detected!
 *       return true;  // One press event
 *   }
 *   last_state = current_state;  // Remember for next check
 *   return false;
 *
 * Result: Returns true ONCE per button press, regardless of how long held.
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * TECHNIQUE 2: DEBOUNCING
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * PROBLEM: Mechanical switches "bounce"
 *
 * When you press a physical button, the metal contacts don't make solid
 * contact immediately. They bounce apart and together multiple times over
 * 5-20ms before settling:
 *
 * Oscilloscope view of real button press:
 *
 *   Voltage
 *    5V ┤ ──┐   ┌──┐ ┌─┐ ┌────────────  (Released = HIGH)
 *       │   └───┘  └─┘ └─┘              (Pressed = LOW)
 *    0V ┤
 *       └─────────────────────────────► Time
 *                   └─┬─┘
 *                  5-20ms bounce period
 *
 * Without debouncing:
 *   - Edge detection sees 4-5 separate presses in 20ms
 *   - One physical press = multiple detected presses
 *   - Game becomes unplayable (button registers 3× per press)
 *
 * SOLUTION: Time-based debouncing
 *
 * After detecting an edge, ignore all transitions for 50ms:
 *
 *   last_debounce_time = 0ms
 *   ↓
 *   Press detected at 100ms → return true, set last_debounce_time = 100ms
 *   ↓
 *   Bounce at 105ms → Ignored (105 - 100 = 5ms < 50ms threshold)
 *   Bounce at 110ms → Ignored (110 - 100 = 10ms < 50ms)
 *   Bounce at 115ms → Ignored (115 - 100 = 15ms < 50ms)
 *   ↓
 *   Next press at 200ms → Allowed (200 - 100 = 100ms > 50ms)
 *
 * WHY 50MS DEBOUNCE TIME?
 * - Too short (10ms): Might not filter all bounces
 * - Too long (200ms): User can't press button rapidly
 * - 50ms is sweet spot: Filters bounces, feels responsive
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * IMPLEMENTATION BELOW
 * ═══════════════════════════════════════════════════════════════════════════
 ******************************************************************************/

/**
 * button_just_pressed - Detect button press event with debouncing
 * @return: true if button just pressed (rising edge), false otherwise
 *
 * CALL THIS EVERY FRAME. It maintains internal state to track edges.
 *
 * ACTIVE-LOW LOGIC:
 * INPUT_PULLUP makes unpressed button read HIGH, pressed reads LOW.
 * We invert the reading so our code works with natural logic:
 *   current_state = !digitalRead(BUTTON_PIN);
 *   // true = pressed, false = released (natural!)
 */
bool button_just_pressed(void) {
    uint32_t now = millis();  // Current time (milliseconds since boot)

    // Read button state and invert (INPUT_PULLUP is active-low)
    // digitalRead returns: HIGH (unpressed), LOW (pressed)
    // We invert: false (unpressed), true (pressed)
    bool current_state = !digitalRead(BUTTON_PIN);

    // Detect rising edge (unpressed → pressed transition)
    bool pressed = false;
    if (current_state && !last_button_state) {
        // Edge detected! But is it valid or just a bounce?

        // Check debounce timeout: has 50ms elapsed since last detected press?
        if (now - last_debounce_time >= DEBOUNCE_MS) {
            // Valid press! Enough time has passed.
            pressed = true;  // Signal press event to caller
            last_debounce_time = now;  // Lock out further presses for 50ms
        }
        // If timeout not met, ignore this edge (it's a bounce)
    }

    // Remember current state for next edge detection
    last_button_state = current_state;

    return pressed;
}

/**
 * button_clear_state - Reset edge detector to current physical state
 *
 * Called during state transitions to prevent "stale" button presses from
 * carrying over between states.
 *
 * PROBLEM WITHOUT CLEARING:
 *
 * Scenario:
 * 1. User presses button in STATE_ATTRACT (starts game)
 * 2. Transition to STATE_PLAYING occurs
 * 3. User still holding button from step 1
 * 4. User releases button in STATE_PLAYING
 * 5. Edge detector sees LOW → HIGH transition (release)
 * 6. Wait... user presses again
 * 7. Edge detector sees HIGH → LOW transition (press)
 * 8. But last_button_state might still be LOW from the held button!
 * 9. False press detected or missed press
 *
 * SOLUTION:
 * Call button_clear_state() in state exit functions. This resets the edge
 * detector to match the current physical button state, ensuring only NEW
 * presses in the NEW state are detected.
 */
void button_clear_state(void) {
    // Sync edge detector to current physical state
    last_button_state = !digitalRead(BUTTON_PIN);  // Invert (active-low)

    // Reset debounce timer to prevent immediate press detection
    last_debounce_time = millis();
}

/**
 * buzzer_tick - Play brief tick sound
 *
 * Used for: Chase LED movement feedback (every LED step)
 *
 * Arduino tone() function:
 *   tone(pin, frequency_hz, duration_ms);
 *
 * PWM generation: Toggles pin HIGH/LOW at specified frequency.
 * Example: 100 Hz = pin toggles 100 times/second
 *
 * NON-BLOCKING: tone() returns immediately! Sound plays in background using
 * hardware timer. No delay() needed.
 */
void buzzer_tick(void) {
    tone(BUZZER_PIN, FREQ_TICK, DURATION_TICK);  // 100 Hz, 20ms
}

/**
 * buzzer_hit - Play hit confirmation sound
 *
 * Used for: Non-bullseye successful hits
 */
void buzzer_hit(void) {
    tone(BUZZER_PIN, FREQ_HIT, DURATION_HIT);  // 500 Hz, 100ms
}

/******************************************************************************
 * SECTION 2: NON-BLOCKING ANIMATION SYSTEM ⭐ MOST COMPLEX SECTION
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * PROBLEM: Multi-Step Animations Without Blocking
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Challenge: Play multi-note melodies and LED sequences without using delay()
 *
 * Why delay() is forbidden:
 *   void play_melody() {
 *       tone(pin, 500, 100);
 *       delay(100);  // ❌ ENTIRE SYSTEM FROZEN for 100ms
 *       tone(pin, 600, 100);
 *       delay(100);  // ❌ Can't check button, update display, reset watchdog!
 *   }
 *
 * Requirements:
 * - Must return to main loop every iteration (for watchdog reset)
 * - Must remain responsive to user input during animations
 * - Must support multiple simultaneous animations (buzzer + LEDs)
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * SOLUTION: State Machine + Cooperative Multitasking
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * WHAT IS COOPERATIVE MULTITASKING?
 *
 * Arduino Uno has:
 * - Single CPU core (no parallelism)
 * - No operating system (no scheduler)
 * - No threads (no preemption)
 *
 * To do multiple things "at once", we use cooperative multitasking:
 * Each "task" runs a bit, then yields control back to caller.
 *
 * Example: Two tasks running cooperatively
 *
 *   Time:  0ms    10ms   20ms   30ms   40ms   50ms
 *   Loop:  │      │      │      │      │      │
 *          ▼      ▼      ▼      ▼      ▼      ▼
 *   Task A runs ─┘      └─── runs ────┘      └──── runs
 *   Task B  └─── runs ────┘      └─── runs ────┘
 *
 * Both tasks "run" by being called from loop() every iteration.
 * Each checks if it's time to do work, does a tiny bit, then returns.
 *
 * OUR IMPLEMENTATION:
 *
 * Animation state machine with independent timing:
 *
 * 1. animation_start_*() - Initialise animation state
 *    - Set anim_state to target animation type
 *    - Reset step counters (anim_step = 0)
 *    - Reset timing variables (anim_last_update = millis())
 *    - Returns immediately
 *
 * 2. animation_update() - Advance animation (called every loop)
 *    - Check current animation state
 *    - Check if enough time elapsed for next step
 *    - If yes: advance to next step, play next note/update LEDs
 *    - If no: return immediately (nothing to do yet)
 *    - Returns true if animation complete
 *
 * 3. animation_is_playing() - Query animation status
 *    - Returns: true if busy, false if idle
 *    - Used by game logic to wait for completion
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * PARALLEL TIMING: Buzzer + LEDs Simultaneously
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * For CELEBRATION and GAME_OVER, we run TWO animations in parallel:
 * - Buzzer plays melody
 * - LEDs play visual effect
 *
 * Each has INDEPENDENT timing:
 *
 *   anim_last_update: Buzzer timing (note intervals)
 *   led_last_update: LED timing (sweep/flash intervals)
 *
 * Timeline example (CELEBRATION):
 *
 *   Time:     0ms   50ms  100ms 150ms 200ms 250ms 300ms ...
 *   Buzzer:   C     (C)   E     (E)   G     (G)   C    ...
 *             └─150ms─┘   └─150ms─┘   └─150ms─┘
 *
 *   LEDs:     [0]   [1]   [2]   [3]   [4]   [5]   [6]  ...
 *             └─40ms┘─40ms┘─40ms┘─40ms┘─40ms┘─40ms┘
 *
 * Notice: Note plays every 150ms, LED advances every 40ms.
 * Completely independent! Handled by separate timer checks:
 *
 *   if (now - anim_last_update >= note_duration) { play_note(); }
 *   if (now - led_last_update >= led_delay) { advance_led(); }
 *
 * WATCHDOG TIMER SAFETY:
 *
 * Longest animation: GAME_OVER
 * - Buzzer: 3 notes × 200ms = 600ms
 * - LEDs: 5 flashes × 300ms = 1500ms
 * - Total: 1500ms
 *
 * Watchdog timeout: 4000ms
 * Safety margin: 4000 - 1500 = 2500ms (plenty!)
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * CODE STRUCTURE BELOW
 * ═══════════════════════════════════════════════════════════════════════════
 ******************************************************************************/

/**
 * AnimationState enum - Animation types
 *
 * The animation system is itself a state machine! At any moment, it's in
 * one of these states:
 *
 * ANIM_IDLE: No animation playing (default state)
 * ANIM_BULLSEYE: 3-note ascending melody (800→1000→1200 Hz)
 * ANIM_CELEBRATION: Complex multi-sensory (buzzer melody + LED wave)
 * ANIM_GAME_OVER: Descending tones + LED flash
 */
enum AnimationState {
    ANIM_IDLE,         // No animation playing
    ANIM_BULLSEYE,     // Bullseye hit animation
    ANIM_CELEBRATION,  // New high score celebration
    ANIM_GAME_OVER     // Game over animation
};

// Animation state variables (persist between animation_update() calls)
static AnimationState anim_state = ANIM_IDLE;  // Current animation
static uint8_t anim_step = 0;                  // Current step in sequence (note counter)
static uint32_t anim_last_update = 0;          // Timestamp of last buzzer update (ms)

// LED animation state (separate from buzzer for parallel timing)
static uint8_t led_sweep = 0;                  // Celebration: which sweep (0-2)
static uint8_t led_pos = 0;                    // Celebration: LED position in sweep
static uint8_t flash_count = 0;                // Game over: number of flashes completed
static bool flash_state = false;               // Game over: current flash state (on/off)
static uint32_t led_last_update = 0;           // Timestamp of last LED update (ms)

/**
 * animation_update - Advance current animation state
 * @return: true if animation completed this frame, false if still playing/idle
 *
 * CALL THIS EVERY LOOP ITERATION (from game_update()).
 *
 * This function checks the current animation state and advances it if enough
 * time has elapsed. It handles all animation logic in one place using a big
 * switch statement.
 *
 * EXECUTION TIME: ~50-100μs (very fast, even when animating)
 */
bool animation_update(void) {
    // Fast path: No animation playing
    if (anim_state == ANIM_IDLE) {
        return true;  // Idle = "complete" (nothing to do)
    }

    uint32_t now = millis();  // Current time (check once per frame)

    switch (anim_state) {

        /**************************************************************************
         * BULLSEYE ANIMATION - 3-Note Ascending Melody
         *
         * Sequence:
         *   Step 0: 800 Hz for 100ms
         *   Step 1: 1000 Hz for 100ms
         *   Step 2: 1200 Hz for 100ms
         *   Step 3: Complete (transition to IDLE)
         *
         * Timeline:
         *   Time:  0ms      100ms     200ms     300ms
         *   Step:  0        1         2         IDLE
         *   Note:  800Hz    1000Hz    1200Hz    (silence)
         *          └─100ms─┘└─100ms─┘└─100ms─┘
         *
         * Logic:
         * - Wait for note duration to elapse (100ms)
         * - Play next note
         * - Increment step
         * - If step >= 3, animation complete
         **************************************************************************/

        case ANIM_BULLSEYE:
            // Check if time for next note
            if (now - anim_last_update >= DURATION_BULLSEYE_NOTE) {
                anim_last_update = now;  // Reset timer for next note

                // Play note based on current step
                switch(anim_step) {
                    case 0: tone(BUZZER_PIN, FREQ_BULLSEYE_1, DURATION_BULLSEYE_NOTE); break;
                    case 1: tone(BUZZER_PIN, FREQ_BULLSEYE_2, DURATION_BULLSEYE_NOTE); break;
                    case 2: tone(BUZZER_PIN, FREQ_BULLSEYE_3, DURATION_BULLSEYE_NOTE); break;
                }

                anim_step++;  // Advance to next note

                // Check if sequence complete
                if (anim_step >= 3) {
                    anim_state = ANIM_IDLE;  // Return to idle state
                    return true;  // Signal completion
                }
            }
            break;

        /**************************************************************************
         * CELEBRATION ANIMATION - Parallel Buzzer + LED Wave
         *
         * BUZZER SEQUENCE (5 notes):
         *   Note 0: C5 (523 Hz) for 150ms
         *   Note 1: E5 (659 Hz) for 150ms
         *   Note 2: G5 (784 Hz) for 150ms
         *   Note 3: C6 (1047 Hz) for 150ms
         *   Note 4: E6 (1319 Hz) for 300ms (finale)
         *   Total: ~900ms
         *
         * LED SEQUENCE (wave effect):
         *   Light LEDs 0→1→2→3→4→5→6→7, then repeat
         *   Each LED lit for 40ms
         *   3 complete sweeps (3 × 8 LEDs × 40ms = 960ms)
         *
         * PARALLEL TIMING:
         * Buzzer and LEDs use separate timers (anim_last_update, led_last_update).
         * They advance independently:
         *
         *   Time:     0ms   40ms  80ms  120ms 160ms 200ms ...
         *   Buzzer:   C5    (C5)  (C5)  (C5)  E5    (E5)  ...
         *             └────150ms────┘    └────150ms────┘
         *   LEDs:     [0]   [1]   [2]   [3]   [4]   [5]  ...
         *             └40ms┘└40ms┘└40ms┘└40ms┘└40ms┘
         *
         * COMPLETION:
         * Animation complete when BOTH sequences finish:
         * - anim_step >= 5 (all notes played)
         * - led_sweep >= 3 (all sweeps complete)
         **************************************************************************/

        case ANIM_CELEBRATION: {
            // Buzzer sequence (5 notes with varying durations)
            const uint16_t freqs[] = {523, 659, 784, 1047, 1319};  // C5, E5, G5, C6, E6
            const uint16_t durations[] = {150, 150, 150, 150, 300};  // Last note longer

            // Check if time for next note
            // First note plays immediately (anim_step == 0), others wait for duration
            if (anim_step < 5 && now - anim_last_update >= (anim_step == 0 ? 0 : durations[anim_step-1] + 50)) {
                tone(BUZZER_PIN, freqs[anim_step], durations[anim_step]);
                anim_last_update = now;
                anim_step++;
            }

            // LED sweep animation (parallel, independent timing)
            if (now - led_last_update >= CELEBRATION_LED_DELAY) {
                led_last_update = now;

                if (led_sweep < CELEBRATION_SWEEPS) {
                    // Turn off previous LED
                    led_set(led_pos, false);

                    // Advance position
                    led_pos++;

                    // Check if sweep complete (reached end of LED strip)
                    if (led_pos >= NUM_LEDS) {
                        led_pos = 0;       // Wrap to start
                        led_sweep++;       // Increment sweep counter
                    }

                    // Turn on LED at new position (if still sweeping)
                    if (led_sweep < CELEBRATION_SWEEPS) {
                        led_set(led_pos, true);
                    } else {
                        // Final sweep complete, clear all LEDs
                        led_clear_all();
                    }
                }
            }

            // Check if BOTH animations complete
            if (anim_step >= 5 && led_sweep >= CELEBRATION_SWEEPS) {
                anim_state = ANIM_IDLE;  // Return to idle
                led_sweep = 0;           // Reset for next time
                led_pos = 0;
                return true;  // Signal completion
            }
            break;
        }

        /**************************************************************************
         * GAME_OVER ANIMATION - Descending Tones + LED Flash
         *
         * BUZZER SEQUENCE (3 notes, "sad trombone"):
         *   Note 0: 400 Hz for 200ms
         *   Note 1: 300 Hz for 200ms
         *   Note 2: 200 Hz for 200ms
         *   Total: 600ms
         *
         * LED SEQUENCE (synchronised flash):
         *   All 8 LEDs flash on/off together
         *   5 complete cycles (on+off = 1 cycle)
         *   150ms per state (150ms on, 150ms off)
         *   Total: 5 cycles × 300ms = 1500ms
         *
         * Timeline:
         *   Time:     0ms   150ms 300ms 450ms 600ms 750ms 900ms ...
         *   Buzzer:   400Hz (400) 300Hz (300) 200Hz (200) (off)
         *             └─200ms─┘   └─200ms─┘   └─200ms─┘
         *   LEDs:     ON    OFF   ON    OFF   ON    OFF   ON   ...
         *             └150ms┘150ms└150ms┘150ms└150ms┘150ms└150ms
         *   Flash:    1st   1st   2nd   2nd   3rd   3rd   4th  ...
         *
         * COMPLETION:
         * Animation complete when flash_count >= 5 (5 on/off cycles)
         **************************************************************************/

        case ANIM_GAME_OVER:
            // Buzzer sequence (descending tones)
            if (now - anim_last_update >= DURATION_GAME_OVER_NOTE) {
                anim_last_update = now;

                if (anim_step < 3) {
                    // Play note based on step
                    switch(anim_step) {
                        case 0: tone(BUZZER_PIN, FREQ_GAME_OVER_1, DURATION_GAME_OVER_NOTE); break;
                        case 1: tone(BUZZER_PIN, FREQ_GAME_OVER_2, DURATION_GAME_OVER_NOTE); break;
                        case 2: tone(BUZZER_PIN, FREQ_GAME_OVER_3, DURATION_GAME_OVER_NOTE); break;
                    }
                    anim_step++;
                }
            }

            // LED flash animation (parallel, independent timing)
            if (now - led_last_update >= GAME_OVER_LED_FLASH_DURATION) {
                led_last_update = now;

                // Toggle flash state (on → off → on → ...)
                flash_state = !flash_state;

                if (flash_state) {
                    // Flash ON: Light all LEDs
                    for (uint8_t i = 0; i < NUM_LEDS; i++) {
                        led_set(i, true);
                    }
                    flash_count++;  // Increment on rising edge (counts complete cycles)
                } else {
                    // Flash OFF: Clear all LEDs
                    led_clear_all();
                }

                // Check if flash sequence complete
                if (flash_count >= GAME_OVER_LED_FLASH_COUNT) {
                    anim_state = ANIM_IDLE;  // Return to idle
                    flash_count = 0;         // Reset for next time
                    led_clear_all();         // Ensure LEDs off
                    return true;  // Signal completion
                }
            }
            break;

        case ANIM_IDLE:
        default:
            return true;  // Nothing to do
    }

    return false;  // Still animating
}

/**
 * animation_start_bullseye - Initialise bullseye animation
 *
 * Called from game.cpp when player hits bullseye zone.
 * Sets up state for 3-note ascending sequence.
 */
void animation_start_bullseye(void) {
    anim_state = ANIM_BULLSEYE;     // Set animation type
    anim_step = 0;                  // Start at first note
    anim_last_update = millis();    // Record start time
}

/**
 * animation_start_celebration - Initialise celebration animation
 *
 * Called from game.cpp when new high score achieved.
 * Sets up state for parallel buzzer melody + LED wave.
 */
void animation_start_celebration(void) {
    anim_state = ANIM_CELEBRATION;  // Set animation type
    anim_step = 0;                  // Start at first note
    led_sweep = 0;                  // Start at first sweep
    led_pos = 0;                    // Start at first LED
    anim_last_update = millis();    // Record start time (buzzer)
    led_last_update = millis();     // Record start time (LEDs)
}

/**
 * animation_start_game_over - Initialise game over animation
 *
 * Called from game.cpp when player misses (no high score).
 * Sets up state for parallel descending tones + LED flash.
 */
void animation_start_game_over(void) {
    anim_state = ANIM_GAME_OVER;    // Set animation type
    anim_step = 0;                  // Start at first note
    flash_count = 0;                // No flashes yet
    flash_state = false;            // Start with LEDs off
    anim_last_update = millis();    // Record start time (buzzer)
    led_last_update = millis();     // Record start time (LEDs)
}

/**
 * animation_is_playing - Check if any animation is active
 * @return: true if animation playing, false if idle
 *
 * Used by game state machine to wait for animations:
 *   if (!animation_is_playing()) {
 *       game_transition_to(STATE_ATTRACT);  // Safe to transition
 *   }
 */
bool animation_is_playing(void) {
    return anim_state != ANIM_IDLE;
}

/******************************************************************************
 * SECTION 3: LCD DISPLAY - I2C Character Display
 *
 * HARDWARE: 16×2 character LCD with I2C backpack
 *
 * I2C (Inter-Integrated Circuit) Protocol:
 * - 2-wire serial protocol (SDA = data, SCL = clock)
 * - Master-slave architecture (Arduino = master, LCD = slave)
 * - Each slave has unique 7-bit address (our LCD = 0x27 or 0x3F)
 * - Multiple devices can share same 2 wires (I2C bus)
 *
 * I2C Communication Architecture:
 *
 *        Arduino Uno (Master)
 *             │
 *             ├─ A4 (SDA) ──────┬────► LCD Display (Slave 0x27)
 *             │                 │
 *             ├─ A5 (SCL) ──────┘      (Could add more devices here)
 *             │
 *             └─ GND ────────────────► Common ground
 *
 * Pull-up Resistors:
 * Both SDA and SCL need pull-up resistors to 5V (typically 4.7kΩ).
 * Most I2C LCD modules have these built-in on the backpack PCB.
 *
 * WHY I2C INSTEAD OF PARALLEL?
 *
 * Parallel LCD (traditional):
 * - Requires 6+ pins (RS, E, D4, D5, D6, D7)
 * - Fast updates (~100μs per character)
 * - Simple protocol (direct GPIO bit manipulation)
 *
 * I2C LCD (what we use):
 * - Requires only 2 pins (SDA, SCL)
 * - Slower updates (~3-4ms for full screen)
 * - More complex protocol (handled by library)
 *
 * For our game: Pin savings >> speed. We update display infrequently
 * (only on state changes and score updates), so 3ms delay is acceptable.
 *
 * DISPLAY COORDINATES:
 *
 * 16 columns × 2 rows (0-indexed):
 *
 *   Column: 0123456789ABCDEF
 *   Row 0:  [Press to Play!]
 *   Row 1:  [HiScore: 100   ]
 *
 * lcd.setCursor(column, row) positions cursor before print
 ******************************************************************************/

/**
 * display_show_attract - Show attract mode screen
 * @param high_score: Current high score to display
 *
 * Screen layout:
 *   ┌────────────────┐
 *   │Press to Play!  │
 *   │HiScore: 100    │
 *   └────────────────┘
 */
void display_show_attract(uint16_t high_score) {
    lcd.clear();              // Clear entire display (removes old content)
    lcd.setCursor(0, 0);      // Position: column 0, row 0 (top-left)
    lcd.print("Press to Play!");
    lcd.setCursor(0, 1);      // Position: column 0, row 1 (bottom-left)
    lcd.print("HiScore: ");
    lcd.print(high_score);    // Print number (right-justified by default)
}

/**
 * display_show_game - Show active game screen
 * @param score: Current game score
 * @param high_score: High score
 *
 * Screen layout:
 *   ┌────────────────┐
 *   │Score:   45     │
 *   │HiScore: 100    │
 *   └────────────────┘
 *
 * FLICKER REDUCTION TECHNIQUE:
 *
 * We DON'T call lcd.clear() here. Instead, we overwrite just the numbers.
 * This prevents screen flicker during gameplay.
 *
 * Without technique (flickers):
 *   lcd.clear();           // Entire screen goes blank for ~3ms
 *   lcd.print("Score:");   // Text reappears
 *   lcd.print(score);
 *   // Visible flicker every update!
 *
 * With technique (smooth):
 *   lcd.setCursor(8, 0);   // Jump to number position
 *   lcd.print(score);      // Overwrite just the number
 *   lcd.print("    ");     // Clear trailing digits (e.g., 100 → 99)
 *   // No flicker, smooth update
 *
 * Trailing spaces: If score decreases (100 → 99), old digit remains unless
 * we overwrite with spaces. lcd.print("    ") clears 4 character positions.
 */
void display_show_game(uint16_t score, uint16_t high_score) {
    // Update score (row 0)
    lcd.setCursor(0, 0);
    lcd.print("Score:   ");   // Label + spacing
    lcd.print(score);
    lcd.print("    ");        // Clear trailing digits (in case score decreased)

    // Update high score (row 1)
    lcd.setCursor(0, 1);
    lcd.print("HiScore: ");
    lcd.print(high_score);
    lcd.print("    ");        // Clear trailing digits
}

/**
 * display_show_celebration - Show new high score screen
 * @param score: New high score value
 *
 * Screen layout:
 *   ┌────────────────┐
 *   │NEW HIGH SCORE! │
 *   │Score: 150      │
 *   └────────────────┘
 */
void display_show_celebration(uint16_t score) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NEW HIGH SCORE!");
    lcd.setCursor(0, 1);
    lcd.print("Score: ");
    lcd.print(score);
}

/**
 * display_clear - Clear display (blank screen)
 *
 * Currently unused but provided for completeness.
 */
void display_clear(void) {
    lcd.clear();
}

/******************************************************************************
 * SECTION 4: EEPROM PERSISTENCE - Non-Volatile Storage
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * EEPROM OVERVIEW
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * EEPROM = Electrically Erasable Programmable Read-Only Memory
 *
 * Arduino Uno has 1KB (1024 bytes) of EEPROM. It's separate from RAM and Flash:
 *
 * ┌────────────┬──────────┬──────────┬─────────────┬─────────────┐
 * │ Type       │ Size     │ Speed    │ Persistence │ Endurance   │
 * ├────────────┼──────────┼──────────┼─────────────┼─────────────┤
 * │ RAM        │ 2 KB     │ 62.5 ns  │ Lost on RST │ Unlimited   │
 * │ Flash      │ 32 KB    │ Fast (R) │ Permanent   │ 10K writes  │
 * │ EEPROM     │ 1 KB     │ 3.3 ms   │ Permanent   │ 100K writes │
 * └────────────┴──────────┴──────────┴─────────────┴─────────────┘
 *
 * KEY CHARACTERISTICS:
 *
 * 1. NON-VOLATILE: Data survives power loss, reset, firmware updates
 *    Perfect for: Settings, high scores, calibration data
 *
 * 2. SLOW WRITES: ~3.3ms per byte (1000× slower than RAM)
 *    Implication: Don't write frequently (once per game, not per frame)
 *
 * 3. LIMITED ENDURANCE: ~100,000 write cycles per byte
 *    Implication: Use EEPROM.update() instead of write() (only writes if changed)
 *
 * 4. UNINITIALISED: Contains random garbage on first boot
 *    Implication: MUST validate data before using (magic byte + checksum)
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * OUR DATA STRUCTURE (4 bytes at address 0)
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Address 0: Score low byte  (bits 0-7)
 * Address 1: Score high byte (bits 8-15)
 * Address 2: Magic byte      (0xA5 = "data valid" marker)
 * Address 3: Checksum        (XOR of bytes 0-2)
 *
 * Example (high score = 305 = 0x0131):
 *
 *   Address │ Value │ Meaning
 *   ────────┼───────┼─────────────────────────────
 *   0       │ 0x31  │ Low byte (305 & 0xFF = 0x31)
 *   1       │ 0x01  │ High byte (305 >> 8 = 0x01)
 *   2       │ 0xA5  │ Magic byte (validation layer 1)
 *   3       │ 0x95  │ Checksum (0x31 ^ 0x01 ^ 0xA5 = 0x95)
 *
 * WHY TWO LAYERS OF VALIDATION?
 *
 * Layer 1: Magic Byte (0xA5)
 * - Detects uninitialised EEPROM (first boot)
 * - Random garbage unlikely to be 0xA5 at byte 2
 * - Fast check (single byte comparison)
 *
 * Layer 2: Checksum (XOR)
 * - Detects data corruption (bit flips, partial writes)
 * - XOR is fast, simple, and good enough for small data
 * - Catches single-bit errors reliably
 *
 * WHY 0xA5 FOR MAGIC BYTE?
 *
 * 0xA5 = 10100101 in binary (alternating bits)
 * - Unlikely pattern in random data
 * - Easy to spot in hex dumps
 * - Common choice in embedded systems (you'll see it often)
 *
 * CHECKSUM ALGORITHM (XOR):
 *
 * XOR (exclusive OR) has useful properties for checksums:
 * - A ^ A = 0 (any value XOR'd with itself = 0)
 * - A ^ 0 = A (XOR with 0 preserves value)
 * - Commutative: A ^ B = B ^ A (order doesn't matter)
 *
 * To generate checksum:
 *   checksum = byte0 ^ byte1 ^ byte2
 *
 * To validate:
 *   expected = byte0 ^ byte1 ^ byte2
 *   if (checksum == expected) → data valid
 *   else → data corrupted
 *
 * Example (score = 305):
 *   checksum = 0x31 ^ 0x01 ^ 0xA5
 *            = 0x30 ^ 0xA5  (0x31 ^ 0x01 = 0x30)
 *            = 0x95
 *
 * If byte 0 gets corrupted (0x31 → 0x32):
 *   expected = 0x32 ^ 0x01 ^ 0xA5 = 0x96
 *   stored checksum = 0x95
 *   0x96 ≠ 0x95 → corruption detected!
 *
 * EEPROM.update() vs EEPROM.write():
 *
 * EEPROM.write(addr, val): Writes unconditionally (wears EEPROM)
 * EEPROM.update(addr, val): Only writes if value differs (preserves endurance)
 *
 * Example scenario:
 *   High score is 100. Player scores 100 again.
 *   EEPROM.write(): Writes all 4 bytes (4 write cycles wasted)
 *   EEPROM.update(): Compares first, sees no change, writes nothing (0 wear)
 *
 * At 100,000 write endurance, saving 100 high scores per day:
 *   With write(): 100,000 / 100 = 1000 days (~3 years) lifespan
 *   With update(): Effectively unlimited (only writes on actual changes)
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * CODE IMPLEMENTATION BELOW
 * ═══════════════════════════════════════════════════════════════════════════
 ******************************************************************************/

/**
 * eeprom_read_high_score - Load high score from EEPROM
 * @return: High score value (0-65535), or 0 if data invalid/uninitialised
 *
 * VALIDATION SEQUENCE:
 * 1. Read all 4 bytes from EEPROM
 * 2. Check magic byte (byte 2) == 0xA5
 *    - If not: EEPROM never initialised (first boot) → return 0
 * 3. Calculate expected checksum: byte0 ^ byte1 ^ byte2
 * 4. Compare to stored checksum (byte 3)
 *    - If mismatch: Data corrupted → return 0
 * 5. Reconstruct 16-bit score: low_byte | (high_byte << 8)
 * 6. Return score
 *
 * DEFENSIVE PROGRAMMING:
 * We return 0 on ANY validation failure. This is safer than returning
 * corrupted data. User sees high score of 0 (expected on first boot) rather
 * than random garbage (confusing).
 */
uint16_t eeprom_read_high_score(void) {
    // Read all 4 bytes from EEPROM
    uint8_t low_byte = EEPROM.read(EEPROM_HIGH_SCORE_ADDR);      // Address 0
    uint8_t high_byte = EEPROM.read(EEPROM_HIGH_SCORE_ADDR + 1); // Address 1
    uint8_t magic = EEPROM.read(EEPROM_HIGH_SCORE_ADDR + 2);     // Address 2
    uint8_t checksum = EEPROM.read(EEPROM_HIGH_SCORE_ADDR + 3);  // Address 3

    // VALIDATION LAYER 1: Magic byte check
    // If byte 2 ≠ 0xA5, EEPROM was never initialised (contains random garbage)
    if (magic != EEPROM_MAGIC_BYTE) {
        return 0;  // Uninitialised, return default score
    }

    // VALIDATION LAYER 2: Checksum verification
    // Calculate expected checksum from data bytes
    uint8_t expected_checksum = low_byte ^ high_byte ^ magic;

    // Compare expected vs stored checksum
    if (checksum != expected_checksum) {
        return 0;  // Corrupted data (bit flip, partial write), return default
    }

    // Data valid! Reconstruct 16-bit score from two 8-bit bytes
    // Example: low_byte = 0x31, high_byte = 0x01
    //   score = 0x31 | (0x01 << 8)
    //         = 0x31 | 0x0100
    //         = 0x0131
    //         = 305 (decimal)
    uint16_t score = (uint16_t)low_byte | ((uint16_t)high_byte << 8);

    return score;
}

/**
 * eeprom_write_high_score - Save high score to EEPROM
 * @param score: Score value to save (0-65535)
 *
 * WRITE SEQUENCE:
 * 1. Split 16-bit score into two 8-bit bytes
 * 2. Calculate checksum: low ^ high ^ 0xA5
 * 3. Write all 4 bytes using EEPROM.update() (wear levelling)
 *
 * BYTE SPLITTING:
 *
 * Example: score = 305 (0x0131 in hex, 0000000100110001 in binary)
 *
 * Low byte (bits 0-7):
 *   score & 0xFF = 0x0131 & 0x00FF = 0x0031 = 0x31 (49 decimal)
 *
 * High byte (bits 8-15):
 *   (score >> 8) & 0xFF = (0x0131 >> 8) & 0xFF
 *                       = 0x0001 & 0xFF
 *                       = 0x01 (1 decimal)
 *
 * Checksum:
 *   0x31 ^ 0x01 ^ 0xA5 = 0x95
 *
 * EEPROM.update() BEHAVIOUR:
 *
 * For each byte, update() checks if new value differs from current EEPROM value:
 * - If different: Writes new value (1 write cycle)
 * - If same: Skips write (0 write cycles, preserves endurance)
 *
 * This is crucial for longevity. If player achieves same high score multiple
 * times, we waste zero write cycles.
 */
void eeprom_write_high_score(uint16_t score) {
    // Split 16-bit score into two 8-bit bytes
    uint8_t low_byte = score & 0xFF;         // Extract bits 0-7
    uint8_t high_byte = (score >> 8) & 0xFF; // Extract bits 8-15

    // Calculate checksum (XOR of all data bytes including magic)
    uint8_t checksum = low_byte ^ high_byte ^ EEPROM_MAGIC_BYTE;

    // Write all 4 bytes using update() for wear levelling
    // update() only writes if value differs from current EEPROM value
    EEPROM.update(EEPROM_HIGH_SCORE_ADDR,     low_byte);           // Address 0
    EEPROM.update(EEPROM_HIGH_SCORE_ADDR + 1, high_byte);          // Address 1
    EEPROM.update(EEPROM_HIGH_SCORE_ADDR + 2, EEPROM_MAGIC_BYTE);  // Address 2
    EEPROM.update(EEPROM_HIGH_SCORE_ADDR + 3, checksum);           // Address 3

    // Total execution time: ~3.3ms per byte written (max 13.2ms if all 4 bytes change)
    // Typical time: ~0ms (no bytes changed) to ~6.6ms (score + checksum changed)
}
