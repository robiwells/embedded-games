#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <stdint.h>

typedef enum {
    LED_IDLE,           // Slow white pulse
    LED_DETECTED,       // Quick green flash
    LED_BREATHING,      // Mood-specific colour breathing
    LED_SPARKLE,        // Completion sparkle
    LED_ERROR           // Slow red pulse
} LedAnimationState;

typedef enum {
    POWER_MODE_NORMAL,     // Full brightness (100%)
    POWER_MODE_ECO,        // 50% brightness
    POWER_MODE_CRITICAL    // 25% brightness
} PowerMode;

void led_init();
void led_update();
void led_set_animation(LedAnimationState animation);
void led_set_brightness(PowerMode mode);
void led_test_sequence();

#endif
