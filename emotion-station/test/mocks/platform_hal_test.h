#ifndef PLATFORM_HAL_TEST_H
#define PLATFORM_HAL_TEST_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file platform_hal_test.h
 * @brief Test-only utilities for the fake HAL implementation.
 *
 * Include this alongside platform_hal_fake.cpp in test builds.
 * Do NOT include in production code.
 */

void fake_reset(void);
void fake_set_millis(unsigned long ms);
void fake_advance_time(unsigned long delta);
const char* fake_get_last_log(void);
uint32_t fake_get_led_color(uint16_t n);
uint8_t fake_get_led_brightness(void);
void fake_set_audio_running(bool running);
const char* fake_get_last_audio_path(void);

#endif // PLATFORM_HAL_TEST_H
