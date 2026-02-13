/**
 * @file platform_hal.h
 * @brief Hardware Abstraction Layer for platform-independent testing
 *
 * This HAL enables unit testing by abstracting Arduino/ESP32 hardware functions
 * into runtime-injectable function pointers. Zero overhead on hardware (optimised away).
 */

#ifndef PLATFORM_HAL_H
#define PLATFORM_HAL_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Function pointer struct for hardware abstraction
 *
 * Each member is a function pointer to a hardware operation.
 * Real implementation calls Arduino functions, fake implementation
 * provides controllable test doubles.
 */
typedef struct {
    // ========= Time Functions =========
    unsigned long (*millis)(void);      ///< Get milliseconds since boot
    unsigned long (*micros)(void);      ///< Get microseconds since boot
    void (*delay)(unsigned long ms);    ///< Blocking delay (avoid in production)

    // ========= Logging Functions =========
    void (*log_print)(const char* msg); ///< Print without newline
    void (*log_println)(const char* msg); ///< Print with newline

    // ========= LED Functions (NeoPixel abstraction) =========
    void (*led_begin)(void);            ///< Initialise LED hardware
    void (*led_show)(void);             ///< Update LED strip with buffered colours
    void (*led_clear)(void);            ///< Clear all LEDs (set to black)
    void (*led_set_pixel_color)(uint16_t n, uint32_t color); ///< Set single LED colour (0xRRGGBB)
    void (*led_set_brightness)(uint8_t brightness); ///< Set global brightness (0-255)
    uint32_t (*led_color)(uint8_t r, uint8_t g, uint8_t b); ///< Pack RGB into 32-bit colour

    // ========= GPIO Functions =========
    void (*pin_mode)(uint8_t pin, uint8_t mode); ///< Configure pin as input/output
    void (*digital_write)(uint8_t pin, uint8_t val); ///< Write digital pin (HIGH/LOW)
    int (*digital_read)(uint8_t pin); ///< Read digital pin state

    // ========= Watchdog Functions =========
    void (*watchdog_reset)(void);       ///< Reset watchdog timer

    // ========= Audio Functions =========
    void (*audio_play)(const char* path);   ///< Start audio playback from SD
    void (*audio_stop)(void);               ///< Stop current playback
    bool (*audio_is_running)(void);         ///< True if audio currently playing
    void (*audio_loop)(void);               ///< Must be called every loop iteration
} PlatformHAL;

/**
 * @brief Global HAL instance (set at runtime in main.cpp)
 *
 * Production code sets this to &platform_real in setup().
 * Test code sets this to &platform_fake before calling game_init().
 */
extern PlatformHAL* platform_hal;

/**
 * @brief Convenience macros for calling HAL functions
 *
 * Usage: HAL_millis() instead of platform_hal->millis()
 * Makes code more readable and easier to refactor.
 */
#define HAL_millis() platform_hal->millis()
#define HAL_micros() platform_hal->micros()
#define HAL_delay(ms) platform_hal->delay(ms)
#define HAL_log_print(msg) platform_hal->log_print(msg)
#define HAL_log_println(msg) platform_hal->log_println(msg)
#define HAL_led_begin() platform_hal->led_begin()
#define HAL_led_show() platform_hal->led_show()
#define HAL_led_clear() platform_hal->led_clear()
#define HAL_led_set_pixel_color(n, color) platform_hal->led_set_pixel_color(n, color)
#define HAL_led_set_brightness(brightness) platform_hal->led_set_brightness(brightness)
#define HAL_led_color(r, g, b) platform_hal->led_color(r, g, b)
#define HAL_pin_mode(pin, mode) platform_hal->pin_mode(pin, mode)
#define HAL_digital_write(pin, val) platform_hal->digital_write(pin, val)
#define HAL_digital_read(pin) platform_hal->digital_read(pin)
#define HAL_watchdog_reset() platform_hal->watchdog_reset()
#define HAL_audio_play(path) platform_hal->audio_play(path)
#define HAL_audio_stop() platform_hal->audio_stop()
#define HAL_audio_is_running() platform_hal->audio_is_running()
#define HAL_audio_loop() platform_hal->audio_loop()

// ========= Platform Implementations =========

/**
 * @brief Real hardware implementation (wraps Arduino functions)
 *
 * Defined in src/platform_hal_real.cpp.
 * Set as global HAL in main.cpp setup().
 */
extern PlatformHAL platform_real;

/**
 * @brief Fake implementation for unit testing
 *
 * Defined in test/mocks/platform_hal_fake.cpp.
 * Provides controllable time, log capture, LED state inspection.
 */
extern PlatformHAL platform_fake;

// ========= Test Utilities (only available in unit tests) =========

#ifdef UNIT_TEST
/**
 * @brief Reset fake HAL state (call in test setUp)
 */
void fake_reset(void);

/**
 * @brief Set fake time to absolute value
 * @param ms Time in milliseconds
 */
void fake_set_millis(unsigned long ms);

/**
 * @brief Advance fake time by delta
 * @param delta Time to advance in milliseconds
 */
void fake_advance_time(unsigned long delta);

/**
 * @brief Get last logged message
 * @return Pointer to last log string (or NULL if none)
 */
const char* fake_get_last_log(void);

/**
 * @brief Get last LED animation colour for pixel
 * @param n Pixel index
 * @return 32-bit colour value (0xRRGGBB)
 */
uint32_t fake_get_led_color(uint16_t n);

/**
 * @brief Get current LED brightness
 * @return Brightness value (0-255)
 */
uint8_t fake_get_led_brightness(void);

/**
 * @brief Simulate audio playing or finished
 * @param running True if audio should appear to be playing
 */
void fake_set_audio_running(bool running);

/**
 * @brief Get the last audio path passed to audio_play
 * @return Pointer to path string (or empty string if none)
 */
const char* fake_get_last_audio_path(void);
#endif // UNIT_TEST

#endif // PLATFORM_HAL_H
