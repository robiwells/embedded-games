#include "time_service.h"
#include "platform_hal.h"

#ifdef WOKWI_SIMULATION

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

#else // Real hardware

#include <time.h>

void time_service_init() {
    // Seed ESP32 RTC with compile-time timestamp so time-of-day filtering works
    // without network or an external RTC module. Time resets on each power cycle.
    struct tm timeinfo = {};
    int year, month, day, hour, min, sec;
    const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char mon_str[4] = {};
    sscanf(__DATE__, "%3s %d %d", mon_str, &day, &year);
    month = ((strstr(months, mon_str) - months) / 3) + 1;
    sscanf(__TIME__, "%d:%d:%d", &hour, &min, &sec);

    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon  = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min  = min;
    timeinfo.tm_sec  = sec;

    time_t t = mktime(&timeinfo);
    struct timeval now_tv = { .tv_sec = t };
    settimeofday(&now_tv, NULL);

    HAL_log_print("RTC: Set to ");
    HAL_log_print(asctime(&timeinfo));
}

void time_service_set(int year, int month, int day, int hour, int min, int sec) {
    struct tm timeinfo = {};
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon  = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min  = min;
    timeinfo.tm_sec  = sec;
    time_t t = mktime(&timeinfo);
    struct timeval now_tv = { .tv_sec = t };
    settimeofday(&now_tv, NULL);
    HAL_log_print("RTC: Time set to ");
    HAL_log_print(asctime(&timeinfo));
}

uint8_t time_service_get_hour() {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    char buf[48];
    snprintf(buf, sizeof(buf), "[TIME] RTC: %04d-%02d-%02d hour=%02d",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
             timeinfo.tm_mday, timeinfo.tm_hour);
    HAL_log_println(buf);
    return (uint8_t)timeinfo.tm_hour;
}

#endif // WOKWI_SIMULATION

TimeOfDay time_service_get_time_of_day() {
    uint8_t hour = time_service_get_hour();

#ifdef WOKWI_SIMULATION
    HAL_log_print("ActivityMgr: Simulated hour: ");
    HAL_log_println(String(hour).c_str());
#endif

    if (hour >= 6  && hour < 12) return TIME_MORNING;
    if (hour >= 12 && hour < 17) return TIME_AFTERNOON;
    if (hour >= 17 && hour < 21) return TIME_EVENING;
    return TIME_BEDTIME;
}
