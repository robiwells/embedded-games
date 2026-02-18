#include "led_controller.h"
#include "config.h"
#include "platform_hal.h"
#include "event_bus.h"
#include <math.h>

static LedAnimationState current_animation = LED_IDLE;
static PowerMode current_power_mode = POWER_MODE_ECO;
static uint32_t animation_start = 0;

void led_init() {
    HAL_led_begin();
    led_set_brightness(POWER_MODE_ECO);
    HAL_led_clear();
    HAL_led_show();
    animation_start = HAL_millis();
    HAL_log_println("LED controller initialised");
}

static void on_state_entered(const Event* event) {
    GameState new_state = event->payload.state_transition.new_state;
    switch (new_state) {
        case STATE_IDLE:
            led_set_animation(LED_IDLE);
            led_set_brightness(POWER_MODE_ECO);
            break;
        case STATE_NFC_DETECTED:
            led_set_animation(LED_DETECTED);
            led_set_brightness(POWER_MODE_NORMAL);
            break;
        case STATE_PLAYING_ACTIVITY:
            led_set_animation(LED_BREATHING);
            break;
        case STATE_ACTIVITY_COMPLETE:
            led_set_animation(LED_SPARKLE);
            break;
        case STATE_ERROR:
            led_set_animation(LED_ERROR);
            led_set_brightness(POWER_MODE_ECO);
            break;
        case STATE_LOW_BATTERY:
            led_set_animation(LED_ERROR);
            led_set_brightness(POWER_MODE_CRITICAL);
            break;
        default:
            break;
    }
}

void led_controller_init() {
    event_bus_subscribe(STATE_ENTERED, on_state_entered);
}

void led_set_brightness(PowerMode mode) {
    current_power_mode = mode;
    switch (mode) {
        case POWER_MODE_NORMAL:
            HAL_led_set_brightness(LED_BRIGHTNESS_ACTIVE);
            break;
        case POWER_MODE_ECO:
            HAL_led_set_brightness(LED_BRIGHTNESS_IDLE);
            break;
        case POWER_MODE_CRITICAL:
            HAL_led_set_brightness(LED_BRIGHTNESS_LOW_BATTERY);
            break;
    }
}

void led_set_animation(LedAnimationState animation) {
    current_animation = animation;
    animation_start = HAL_millis();
}

void led_update() {
    uint32_t elapsed = HAL_millis() - animation_start;

    switch (current_animation) {
        case LED_IDLE: {
            // Slow white pulse (4-second cycle)
            float brightness = (sin((elapsed / 2000.0) * 3.14159265359) + 1.0) / 2.0;
            uint8_t val = (uint8_t)(brightness * 255);
            for (int i = 0; i < NUM_LEDS; i++) {
                HAL_led_set_pixel_color(i, HAL_led_color(val, val, val));
            }
            break;
        }

        case LED_DETECTED: {
            // Quick green flash (500ms total)
            if (elapsed < 250) {
                for (int i = 0; i < NUM_LEDS; i++) {
                    HAL_led_set_pixel_color(i, HAL_led_color(0, 255, 0));
                }
            } else {
                for (int i = 0; i < NUM_LEDS; i++) {
                    HAL_led_set_pixel_color(i, HAL_led_color(0, 0, 0));
                }
            }
            break;
        }

        case LED_BREATHING: {
            // Breathing blue (2-second cycle)
            float brightness = (sin((elapsed / 1000.0) * 3.14159265359) + 1.0) / 2.0;
            uint8_t val = (uint8_t)(brightness * 255);
            for (int i = 0; i < NUM_LEDS; i++) {
                HAL_led_set_pixel_color(i, HAL_led_color(0, 0, val));
            }
            break;
        }

        case LED_SPARKLE: {
            // Random sparkle (2 seconds)
            if (elapsed < 2000 && elapsed % 100 < 50) {
                // Note: Using simple modulo for random in tests, hardware uses random()
                int led = (elapsed / 100) % NUM_LEDS;
                HAL_led_set_pixel_color(led, HAL_led_color(255, 255, 255));
            } else {
                HAL_led_clear();
            }
            break;
        }

        case LED_ERROR: {
            // Slow red pulse (3-second cycle)
            float brightness = (sin((elapsed / 1500.0) * 3.14159265359) + 1.0) / 2.0;
            uint8_t val = (uint8_t)(brightness * 128); // Dimmer for errors
            for (int i = 0; i < NUM_LEDS; i++) {
                HAL_led_set_pixel_color(i, HAL_led_color(val, 0, 0));
            }
            break;
        }
    }

    HAL_led_show();
}

void led_test_sequence() {
    HAL_log_println("\n=== LED Test: All animations ===");

    // Test each animation for 3 seconds
    LedAnimationState tests[] = {LED_IDLE, LED_DETECTED, LED_BREATHING, LED_SPARKLE, LED_ERROR};
    const char* names[] = {"IDLE", "DETECTED", "BREATHING", "SPARKLE", "ERROR"};

    for (int i = 0; i < 5; i++) {
        HAL_log_print("Testing: ");
        HAL_log_println(names[i]);
        led_set_animation(tests[i]);
        uint32_t start = HAL_millis();
        while (HAL_millis() - start < 3000) {
            led_update();
            HAL_delay(10);
        }
    }

    HAL_log_println("LED Test complete\n");
}
