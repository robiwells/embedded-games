/**
 * @file platform_hal_fake.cpp
 * @brief Fake hardware implementation for unit testing
 *
 * Provides controllable test doubles for all hardware functions:
 * - Fake time (set/advance manually)
 * - Log capture (inspect what was logged)
 * - LED state capture (verify colours/brightness)
 * - GPIO state capture (verify pin states)
 */

#include "platform_hal.h"
#include <string.h>
#include <stdlib.h>

// ========= Fake State =========

#define MAX_LEDS 16
#define LOG_BUFFER_SIZE 256

static unsigned long fake_time_ms = 0;
static char fake_log_buffer[LOG_BUFFER_SIZE] = {0};
static uint32_t fake_led_colors[MAX_LEDS] = {0};
static uint8_t fake_led_brightness = 255;
static uint8_t fake_gpio_state[256] = {0}; // Pin states (0=LOW, 1=HIGH)

// Audio state
static bool fake_audio_running = false;
static char fake_audio_path[128] = {0};

// ========= Test Utilities =========

void fake_reset(void) {
    fake_time_ms = 0;
    memset(fake_log_buffer, 0, LOG_BUFFER_SIZE);
    memset(fake_led_colors, 0, sizeof(fake_led_colors));
    fake_led_brightness = 255;
    memset(fake_gpio_state, 0, sizeof(fake_gpio_state));
    fake_audio_running = false;
    memset(fake_audio_path, 0, sizeof(fake_audio_path));
}

void fake_set_millis(unsigned long ms) {
    fake_time_ms = ms;
}

void fake_advance_time(unsigned long delta) {
    fake_time_ms += delta;
}

const char* fake_get_last_log(void) {
    return fake_log_buffer;
}

uint32_t fake_get_led_color(uint16_t n) {
    if (n < MAX_LEDS) {
        return fake_led_colors[n];
    }
    return 0;
}

uint8_t fake_get_led_brightness(void) {
    return fake_led_brightness;
}

void fake_set_audio_running(bool running) {
    fake_audio_running = running;
}

const char* fake_get_last_audio_path(void) {
    return fake_audio_path;
}

// ========= Time Functions =========

static unsigned long fake_millis(void) {
    return fake_time_ms;
}

static unsigned long fake_micros(void) {
    // Convert milliseconds to microseconds
    return fake_time_ms * 1000;
}

static void fake_delay(unsigned long ms) {
    // Advance time instead of blocking
    fake_time_ms += ms;
}

// ========= Logging Functions =========

static void fake_log_print(const char* msg) {
    if (msg) {
        // Append to buffer (basic implementation, can overflow)
        size_t len = strlen(fake_log_buffer);
        size_t remaining = LOG_BUFFER_SIZE - len - 1;
        strncat(fake_log_buffer, msg, remaining);
    }
}

static void fake_log_println(const char* msg) {
    fake_log_print(msg);
    fake_log_print("\n");
}

// ========= LED Functions =========

static void fake_led_begin(void) {
    // No-op (already initialised)
}

static void fake_led_show(void) {
    // No-op (changes already applied to fake_led_colors)
}

static void fake_led_clear(void) {
    memset(fake_led_colors, 0, sizeof(fake_led_colors));
}

static void fake_led_set_pixel_color(uint16_t n, uint32_t color) {
    if (n < MAX_LEDS) {
        fake_led_colors[n] = color;
    }
}

static void fake_led_set_brightness(uint8_t brightness) {
    fake_led_brightness = brightness;
}

static uint32_t fake_led_color(uint8_t r, uint8_t g, uint8_t b) {
    // Pack RGB into 32-bit value (0x00RRGGBB)
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

// ========= GPIO Functions =========

static void fake_pin_mode(uint8_t pin, uint8_t mode) {
    // No-op (mode not stored)
}

static void fake_digital_write(uint8_t pin, uint8_t val) {
    if (pin < 256) {
        fake_gpio_state[pin] = val;
    }
}

static int fake_digital_read(uint8_t pin) {
    if (pin < 256) {
        return fake_gpio_state[pin];
    }
    return 0;
}

// ========= Watchdog Functions =========

static void fake_watchdog_reset(void) {
    // No-op (no watchdog in tests)
}

// ========= Audio Functions =========

static bool fake_audio_play(const char* path) {
    if (path) {
        strncpy(fake_audio_path, path, sizeof(fake_audio_path) - 1);
    }
    fake_audio_running = true;
    return true;
}

static void fake_audio_stop(void) {
    fake_audio_running = false;
}

static bool fake_audio_is_running(void) {
    return fake_audio_running;
}

static void fake_audio_loop(void) {
    // No-op
}

// ========= Platform HAL Instance =========

/**
 * @brief Fake hardware HAL for unit testing
 *
 * Set as global HAL in test setUp: platform_hal = &platform_fake;
 */
PlatformHAL platform_fake = {
    // Time
    .millis = fake_millis,
    .micros = fake_micros,
    .delay = fake_delay,

    // Logging
    .log_print = fake_log_print,
    .log_println = fake_log_println,

    // LED
    .led_begin = fake_led_begin,
    .led_show = fake_led_show,
    .led_clear = fake_led_clear,
    .led_set_pixel_color = fake_led_set_pixel_color,
    .led_set_brightness = fake_led_set_brightness,
    .led_color = fake_led_color,

    // GPIO
    .pin_mode = fake_pin_mode,
    .digital_write = fake_digital_write,
    .digital_read = fake_digital_read,

    // Watchdog
    .watchdog_reset = fake_watchdog_reset,

    // Audio
    .audio_play = fake_audio_play,
    .audio_stop = fake_audio_stop,
    .audio_is_running = fake_audio_is_running,
    .audio_loop = fake_audio_loop
};

// Global HAL pointer (defined here for tests)
PlatformHAL* platform_hal = nullptr;
