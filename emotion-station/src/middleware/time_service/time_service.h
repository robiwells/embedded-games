#ifndef TIME_SERVICE_H
#define TIME_SERVICE_H

#include "config.h"

// Initialise the time service — seeds ESP32 RTC from compile-time timestamp.
// No-op on Wokwi (simulated time driven by millis).
void time_service_init();

// Returns the current time bucket based on RTC hour (or simulated hour on Wokwi).
TimeOfDay time_service_get_time_of_day();

// Returns the raw hour (0-23).
uint8_t time_service_get_hour();

// Sets the RTC to the specified date/time (real hardware only).
void time_service_set(int year, int month, int day, int hour, int min, int sec);

// Adjusts the simulated time offset so the current hour appears as the given value.
void time_service_set_mock_hour(uint8_t hour);

#endif // TIME_SERVICE_H
