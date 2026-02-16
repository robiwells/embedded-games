/**
 * @file time_service_mock.cpp
 * @brief Stub time service for unit testing
 */

#include "time_service.h"

static TimeOfDay s_time_of_day = TIME_AFTERNOON;
static uint8_t   s_hour        = 12;

void time_service_init() {}

TimeOfDay time_service_get_time_of_day() {
    return s_time_of_day;
}

uint8_t time_service_get_hour() {
    return s_hour;
}

#ifdef WOKWI_SIMULATION
void time_service_set_mock_hour(uint8_t hour) {
    s_hour = hour;
    if (hour >= 6  && hour < 12) s_time_of_day = TIME_MORNING;
    else if (hour >= 12 && hour < 17) s_time_of_day = TIME_AFTERNOON;
    else if (hour >= 17 && hour < 21) s_time_of_day = TIME_EVENING;
    else s_time_of_day = TIME_BEDTIME;
}
#endif
