#include <Arduino.h>
#include <avr/wdt.h>
#include "game.h"
#include "hardware.h"

// ============================================================================
// Main Entry Points
// ============================================================================

void setup() {
    // Disable watchdog timer during setup
    wdt_disable();

    // Initialise hardware abstraction layer
    hardware_init();

    // Initialise button interrupt (MUST be called after hardware_init)
    button_init_interrupt();

    // Initialise game state machine
    game_init();

    // Enable watchdog timer (4 second timeout)
    wdt_enable(WDTO_4S);
}

void loop() {
    // Reset watchdog timer
    wdt_reset();

    // Update game state machine
    game_update();
}
