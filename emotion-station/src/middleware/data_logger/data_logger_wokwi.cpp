/**
 * @file data_logger_wokwi.cpp
 * @brief Wokwi simulation data logger — replaces data_logger.cpp in wokwi env
 */

#include "data_logger.h"
#include "mood_registry.h"
#include "platform_hal.h"
#include <stdio.h>
#include <string.h>

static const char* time_name(TimeOfDay t) {
    switch (t) {
        case TIME_MORNING:   return "morning";
        case TIME_AFTERNOON: return "afternoon";
        case TIME_EVENING:   return "evening";
        case TIME_BEDTIME:   return "bedtime";
        default:             return "unknown";
    }
}

#define MOCK_LOG_ROWS 10
#define MOCK_LOG_COL  128

static char mock_log[MOCK_LOG_ROWS][MOCK_LOG_COL];
static int  mock_log_count = 0;

bool logger_init() {
    mock_log_count = 0;
    for (int i = 0; i < MOCK_LOG_ROWS; i++) {
        mock_log[i][0] = '\0';
    }
    HAL_log_println("[LOGGER] Wokwi simulation: mock logger ready");
    return true;
}

bool logger_log_session(const SessionLog* session) {
    if (!session) {
        return false;
    }

    char row[MOCK_LOG_COL];
    snprintf(row, sizeof(row), "%lu,%s,%u,%s,%u,%s,%s",
        (unsigned long)session->timestamp,
        mood_registry_name(session->mood),
        (unsigned)session->activity_id,
        session->activity_name,
        (unsigned)session->duration_seconds,
        session->completed ? "true" : "false",
        time_name(session->time_of_day)
    );

    if (mock_log_count < MOCK_LOG_ROWS) {
        strncpy(mock_log[mock_log_count], row, MOCK_LOG_COL - 1);
        mock_log[mock_log_count][MOCK_LOG_COL - 1] = '\0';
        mock_log_count++;
    }

    HAL_log_print("[LOGGER] Logged session: ");
    HAL_log_println(row);
    return true;
}

void logger_test() {
    SessionLog test_session;
    test_session.timestamp        = 42;
    test_session.mood             = MOOD_HAPPY;
    test_session.activity_id      = 1;
    strncpy(test_session.activity_name, "test_activity", ACTIVITY_NAME_LENGTH - 1);
    test_session.activity_name[ACTIVITY_NAME_LENGTH - 1] = '\0';
    test_session.duration_seconds = 30;
    test_session.completed        = true;
    test_session.time_of_day      = TIME_MORNING;

    HAL_log_println("[LOGGER] Running test...");
    logger_log_session(&test_session);

    HAL_log_println("[LOGGER] Stored rows:");
    for (int i = 0; i < mock_log_count; i++) {
        HAL_log_println(mock_log[i]);
    }
}
