#include "led_controller.h"
#include "config.h"
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel pixels(NUM_LEDS, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);

static LedAnimationState current_animation = LED_IDLE;
static PowerMode current_power_mode = POWER_MODE_ECO;
static uint32_t animation_start = 0;

void led_init() {
    pixels.begin();
    led_set_brightness(POWER_MODE_ECO);
    pixels.clear();
    pixels.show();
    animation_start = millis();
    Serial.println("LED controller initialised");
}

void led_set_brightness(PowerMode mode) {
    current_power_mode = mode;
    switch (mode) {
        case POWER_MODE_NORMAL:
            pixels.setBrightness(LED_BRIGHTNESS_ACTIVE);
            break;
        case POWER_MODE_ECO:
            pixels.setBrightness(LED_BRIGHTNESS_IDLE);
            break;
        case POWER_MODE_CRITICAL:
            pixels.setBrightness(LED_BRIGHTNESS_LOW_BATTERY);
            break;
    }
}

void led_set_animation(LedAnimationState animation) {
    current_animation = animation;
    animation_start = millis();
}

void led_update() {
    uint32_t elapsed = millis() - animation_start;

    switch (current_animation) {
        case LED_IDLE: {
            // Slow white pulse (4-second cycle)
            float brightness = (sin((elapsed / 2000.0) * PI) + 1.0) / 2.0;
            uint8_t val = (uint8_t)(brightness * 255);
            for (int i = 0; i < NUM_LEDS; i++) {
                pixels.setPixelColor(i, pixels.Color(val, val, val));
            }
            break;
        }

        case LED_DETECTED: {
            // Quick green flash (500ms total)
            if (elapsed < 250) {
                for (int i = 0; i < NUM_LEDS; i++) {
                    pixels.setPixelColor(i, pixels.Color(0, 255, 0));
                }
            } else {
                for (int i = 0; i < NUM_LEDS; i++) {
                    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
                }
            }
            break;
        }

        case LED_BREATHING: {
            // Breathing blue (2-second cycle)
            float brightness = (sin((elapsed / 1000.0) * PI) + 1.0) / 2.0;
            uint8_t val = (uint8_t)(brightness * 255);
            for (int i = 0; i < NUM_LEDS; i++) {
                pixels.setPixelColor(i, pixels.Color(0, 0, val));
            }
            break;
        }

        case LED_SPARKLE: {
            // Random sparkle (2 seconds)
            if (elapsed < 2000 && elapsed % 100 < 50) {
                int led = random(NUM_LEDS);
                pixels.setPixelColor(led, pixels.Color(255, 255, 255));
            } else {
                pixels.clear();
            }
            break;
        }

        case LED_ERROR: {
            // Slow red pulse (3-second cycle)
            float brightness = (sin((elapsed / 1500.0) * PI) + 1.0) / 2.0;
            uint8_t val = (uint8_t)(brightness * 128); // Dimmer for errors
            for (int i = 0; i < NUM_LEDS; i++) {
                pixels.setPixelColor(i, pixels.Color(val, 0, 0));
            }
            break;
        }
    }

    pixels.show();
}

void led_test_sequence() {
    Serial.println("\n=== LED Test: All animations ===");

    // Test each animation for 3 seconds
    LedAnimationState tests[] = {LED_IDLE, LED_DETECTED, LED_BREATHING, LED_SPARKLE, LED_ERROR};
    const char* names[] = {"IDLE", "DETECTED", "BREATHING", "SPARKLE", "ERROR"};

    for (int i = 0; i < 5; i++) {
        Serial.print("Testing: ");
        Serial.println(names[i]);
        led_set_animation(tests[i]);
        uint32_t start = millis();
        while (millis() - start < 3000) {
            led_update();
            delay(10);
        }
    }

    Serial.println("LED Test complete\n");
}
