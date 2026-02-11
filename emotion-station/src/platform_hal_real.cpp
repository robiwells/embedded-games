/**
 * @file platform_hal_real.cpp
 * @brief Real hardware implementation of Platform HAL (wraps Arduino/ESP32 functions)
 *
 * This implementation provides zero-overhead access to Arduino functions
 * through function pointers. The compiler optimises these away with -O2.
 */

#include "platform_hal.h"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <esp_task_wdt.h>
#include "config.h"

// Global NeoPixel instance (managed by LED controller)
static Adafruit_NeoPixel* g_pixels = nullptr;

// ========= Time Functions =========

static unsigned long real_millis(void) {
    return millis();
}

static unsigned long real_micros(void) {
    return micros();
}

static void real_delay(unsigned long ms) {
    delay(ms);
}

// ========= Logging Functions =========

static void real_log_print(const char* msg) {
    Serial.print(msg);
}

static void real_log_println(const char* msg) {
    Serial.println(msg);
}

// ========= LED Functions =========

static void real_led_begin(void) {
    if (g_pixels == nullptr) {
        g_pixels = new Adafruit_NeoPixel(NUM_LEDS, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);
    }
    g_pixels->begin();
}

static void real_led_show(void) {
    if (g_pixels) {
        g_pixels->show();
    }
}

static void real_led_clear(void) {
    if (g_pixels) {
        g_pixels->clear();
    }
}

static void real_led_set_pixel_color(uint16_t n, uint32_t color) {
    if (g_pixels) {
        g_pixels->setPixelColor(n, color);
    }
}

static void real_led_set_brightness(uint8_t brightness) {
    if (g_pixels) {
        g_pixels->setBrightness(brightness);
    }
}

static uint32_t real_led_color(uint8_t r, uint8_t g, uint8_t b) {
    if (g_pixels) {
        return g_pixels->Color(r, g, b);
    }
    return 0;
}

// ========= GPIO Functions =========

static void real_pin_mode(uint8_t pin, uint8_t mode) {
    pinMode(pin, mode);
}

static void real_digital_write(uint8_t pin, uint8_t val) {
    digitalWrite(pin, val);
}

static int real_digital_read(uint8_t pin) {
    return digitalRead(pin);
}

// ========= Watchdog Functions =========

static void real_watchdog_reset(void) {
    esp_task_wdt_reset();
}

// ========= Platform HAL Instance =========

/**
 * @brief Real hardware HAL implementation
 *
 * Set as global HAL in main.cpp: platform_hal = &platform_real;
 */
PlatformHAL platform_real = {
    // Time
    .millis = real_millis,
    .micros = real_micros,
    .delay = real_delay,

    // Logging
    .log_print = real_log_print,
    .log_println = real_log_println,

    // LED
    .led_begin = real_led_begin,
    .led_show = real_led_show,
    .led_clear = real_led_clear,
    .led_set_pixel_color = real_led_set_pixel_color,
    .led_set_brightness = real_led_set_brightness,
    .led_color = real_led_color,

    // GPIO
    .pin_mode = real_pin_mode,
    .digital_write = real_digital_write,
    .digital_read = real_digital_read,

    // Watchdog
    .watchdog_reset = real_watchdog_reset
};

// Global HAL pointer (set in main.cpp)
PlatformHAL* platform_hal = nullptr;
