/**
 * @file time_service_wokwi.cpp
 * @brief Wokwi simulation time service — replaces time_service.cpp in wokwi env
 */

#include "time_service.h"
#include "platform_hal.h"
#include <stdio.h>

static uint8_t s_sim_time_offset_hours = 12;

void time_service_init() {
    // No RTC hardware in Wokwi — time is driven by millis() simulation
}

void time_service_set_mock_hour(uint8_t hour) {
    uint32_t elapsed_hours = (HAL_millis() / 60000UL) % 24;
    s_sim_time_offset_hours = (uint8_t)((hour + 24 - elapsed_hours) % 24);
}

uint8_t time_service_get_hour() {
    return (uint8_t)(((HAL_millis() / 60000UL) + s_sim_time_offset_hours) % 24);
}

TimeOfDay time_service_get_time_of_day() {
    uint8_t hour = time_service_get_hour();
    char hour_str[4];
    snprintf(hour_str, sizeof(hour_str), "%u", (unsigned)hour);
    HAL_log_print("ActivityMgr: Simulated hour: ");
    HAL_log_println(hour_str);
    if (hour >= 6  && hour < 12) return TIME_MORNING;
    if (hour >= 12 && hour < 17) return TIME_AFTERNOON;
    if (hour >= 17 && hour < 21) return TIME_EVENING;
    return TIME_BEDTIME;
}
