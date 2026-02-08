#include <Arduino.h>
#include "hardware.h"
#include "game.h"

void setup() {
    hardware_init();
    game_init();
}

void loop() {
    game_update();
}
