#include <Arduino.h>
#include <avr/wdt.h>
#include "hardware.h"
#include "game.h"

void setup() {
    hardware_init();
    game_init();

    // Enable watchdog timer (4 second timeout)
    wdt_enable(WDTO_4S);
}

void loop() {
    game_update();
    wdt_reset();  // Feed the watchdog
}
