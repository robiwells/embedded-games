/******************************************************************************
 * MAIN.CPP - Arduino Firmware Entry Point
 *
 * TUTORIAL: Understanding the Arduino Program Lifecycle
 *
 * If you're coming from desktop/web programming, Arduino code looks strange.
 * There's no main() function! Instead, you write two special functions:
 *
 * 1. setup() - Called ONCE when Arduino powers on or resets
 * 2. loop() - Called REPEATEDLY forever after setup() completes
 *
 * COMPARISON TO DESKTOP PROGRAMMING:
 *
 * Desktop application:
 *   int main() {
 *       initialise();
 *       while (running) {
 *           process_input();
 *           update_logic();
 *           render();
 *       }
 *       cleanup();
 *       return 0;
 *   }
 *
 * Arduino equivalent:
 *   void setup() {
 *       initialise();  // setup() is your initialisation
 *   }
 *   void loop() {
 *       process_input();  // loop() is your while(running) body
 *       update_logic();
 *       render();
 *   }
 *   // No cleanup() - Arduino runs forever!
 *   // No return statement - embedded systems don't "exit"
 *
 * WHY THIS PATTERN?
 *
 * "Bare metal" programming: Arduino firmware runs directly on hardware with
 * no operating system. The Arduino framework provides a hidden main() that:
 * 1. Initialises hardware (timers, serial, etc.)
 * 2. Calls your setup()
 * 3. Calls your loop() in an infinite while(1) loop
 *
 * You write setup() and loop(), Arduino handles the rest.
 *
 * LEARNING OBJECTIVES:
 * - Understand setup/loop lifecycle (one-time vs repeated execution)
 * - Learn about watchdog timers as safety mechanisms
 * - Recognise the critical importance of non-blocking code
 *
 * Related files:
 * - hardware.cpp: hardware_init() initialises all pins and peripherals
 * - game.cpp: game_init() sets up game state, game_update() runs game logic
 ******************************************************************************/

#include <Arduino.h>
#include <avr/wdt.h>   // Watchdog Timer library for AVR microcontrollers
#include "hardware.h"
#include "game.h"

/******************************************************************************
 * setup() - One-Time Initialisation
 *
 * Called exactly ONCE when:
 * - Arduino is powered on
 * - Reset button is pressed
 * - New firmware is uploaded
 *
 * RESPONSIBILITIES:
 * - Configure hardware (pin modes, peripheral initialisation)
 * - Set initial variable values
 * - Load persistent data (EEPROM)
 * - Enable safety mechanisms (watchdog timer)
 *
 * IMPORTANT: setup() must complete quickly (within seconds). If setup() takes
 * too long, the watchdog timer will reset the Arduino, causing an infinite
 * reset loop. Our setup() completes in milliseconds - no problem.
 *
 * COMMON BEGINNER MISTAKE:
 *   void setup() {
 *       Serial.begin(9600);
 *       delay(10000);  // ❌ BAD! 10 second delay here is unnecessary
 *       pinMode(13, OUTPUT);
 *   }
 *
 * Keep setup() lean - only essential initialisation.
 ******************************************************************************/

void setup() {
    // Initialise all hardware peripherals (LEDs, button, buzzer, LCD, I2C)
    // See hardware.cpp:hardware_init() for pin configuration details
    hardware_init();

    // Initialise game state machine and load high score from EEPROM
    // See game.cpp:game_init() for state machine setup
    game_init();

    /**************************************************************************
     * WATCHDOG TIMER - Critical Safety Mechanism
     *
     * WHAT IS A WATCHDOG TIMER?
     *
     * A watchdog timer (WDT) is a hardware timer that automatically resets
     * the microcontroller if not "fed" (reset) regularly. It's a failsafe
     * against software crashes or infinite loops.
     *
     * HOW IT WORKS:
     * 1. Enable WDT with timeout (we use 4 seconds)
     * 2. WDT counts down from 4000ms to 0
     * 3. Your code must call wdt_reset() before timer reaches 0
     * 4. If timer expires (reaches 0) → Arduino RESETS (like pressing reset button)
     * 5. wdt_reset() resets timer back to 4000ms
     *
     * ANALOGY:
     * You have a guard dog that will attack unless you pet it every 4 seconds.
     * wdt_reset() = petting the dog. Forget to pet = reset (attack).
     *
     * WHY DO WE NEED THIS?
     *
     * Embedded systems often run unattended (in products, installations, etc.).
     * If software crashes or hangs, there's no human to press the reset button.
     * WDT provides automatic recovery:
     *
     * Example failure scenarios WDT protects against:
     * - while(1) {} infinite loop (forgot break condition)
     * - delay(10000) in wrong place (blocks too long)
     * - I2C device hangs (library gets stuck waiting)
     * - Memory corruption causes crash
     *
     * WDTO_4S TIMEOUT CHOICE:
     *
     * We chose 4 seconds because:
     * - Our loop() executes every ~1-50ms (very fast)
     * - Longest blocking operation: EEPROM write (~3.3ms)
     * - All animations are non-blocking (return immediately)
     * - 4 seconds gives HUGE safety margin
     *
     * AVR Watchdog timeout options:
     * WDTO_15MS, WDTO_30MS, WDTO_60MS, WDTO_120MS, WDTO_250MS,
     * WDTO_500MS, WDTO_1S, WDTO_2S, WDTO_4S, WDTO_8S
     *
     * CRITICAL REQUIREMENT:
     * Every code path in loop() MUST execute faster than the WDT timeout.
     *
     * COMMON BEGINNER MISTAKE:
     *   void loop() {
     *       delay(5000);  // ❌ FATAL! 5 seconds > 4 second WDT timeout
     *       wdt_reset();  // Never reached - Arduino resets at 4s mark
     *   }
     *
     * Our code is safe because:
     * - We NEVER use blocking delay() calls in loop()
     * - All timing uses millis() timestamps (non-blocking)
     * - See hardware.cpp:animation_update() for non-blocking patterns
     **************************************************************************/

    wdt_enable(WDTO_4S);  // Enable 4-second watchdog timer
}

/******************************************************************************
 * loop() - Main Program Loop (Runs Forever)
 *
 * Called REPEATEDLY in an infinite loop after setup() completes.
 * The hidden Arduino main() looks like:
 *
 *   int main(void) {
 *       init();         // Arduino hardware init
 *       setup();        // Your setup()
 *       for (;;) {      // Infinite loop (same as while(1))
 *           loop();     // Your loop()
 *       }
 *       return 0;       // Never reached
 *   }
 *
 * CRITICAL REQUIREMENTS FOR loop():
 *
 * 1. MUST EXECUTE QUICKLY (< 4 seconds for our WDT timeout)
 *    - Our loop() completes in 1-50ms depending on game state
 *
 * 2. MUST NOT BLOCK (no delay(), no while(condition) waits)
 *    - Use millis() timestamps instead of delay()
 *    - See game.cpp:update_chase_position() for timing pattern
 *
 * 3. MUST RESET WATCHDOG TIMER EVERY ITERATION
 *    - We call wdt_reset() at the end of every loop()
 *
 * EXECUTION FLOW:
 *
 * loop() iteration 1:  game_update() → wdt_reset() → return
 * loop() iteration 2:  game_update() → wdt_reset() → return
 * loop() iteration 3:  game_update() → wdt_reset() → return
 * ... repeats forever at ~1-50ms per iteration
 *
 * INSIDE game_update():
 * - Updates animation state (non-blocking)
 * - Calls current game state's update function
 * - Checks button input
 * - Updates LED positions
 * - All using non-blocking patterns!
 *
 * WHY SO SIMPLE?
 *
 * This is a key embedded pattern: THIN MAIN LOOP. The loop() function is
 * deliberately minimal. All the real work happens in:
 * - game.cpp: State machine logic
 * - hardware.cpp: Hardware control and animations
 *
 * Benefits:
 * - Easy to understand program flow
 * - Watchdog timer safety is obvious (wdt_reset() right there)
 * - Hardware updates (animation_update) happen every frame
 * - Clean separation of concerns
 *
 * PERFORMANCE NOTE:
 *
 * You might think calling functions every millisecond is wasteful.
 * "Shouldn't we sleep when there's nothing to do?"
 *
 * On Arduino Uno (16 MHz):
 * - Function call overhead: ~1-2 microseconds
 * - Our entire loop(): ~100-500 microseconds
 * - Power savings from sleep: negligible for USB-powered device
 *
 * For battery-powered devices, we'd add sleep modes. For our game, the
 * responsiveness of running full-speed is more important.
 ******************************************************************************/

void loop() {
    // Update game state machine and all animations
    // This function:
    // 1. Calls animation_update() to advance any playing animations
    // 2. Calls the current game state's update() function
    // 3. Returns immediately (non-blocking)
    // See game.cpp:game_update() for implementation
    game_update();

    // Feed the watchdog timer - reset countdown to 4000ms
    // If we forget this call, Arduino resets after 4 seconds
    // The wdt_reset() macro is defined in <avr/wdt.h>
    wdt_reset();
}
