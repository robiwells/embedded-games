#include "session_manager.h"
#include "data_logger.h"
#include "platform_hal.h"
#include "event_bus.h"
#include <string.h>

typedef struct {
    const Activity* activity;
    MoodCategory    mood;
    TimeOfDay       time_of_day;
} SessionStartPayload;

typedef struct {
    bool completed;
} SessionEndPayload;

static SessionLog s_session;
static uint32_t   s_start_time = 0;
static bool       s_active = false;

void session_begin(MoodCategory mood, const Activity* activity, TimeOfDay time_of_day) {
    s_start_time = HAL_millis();
    s_session.timestamp        = HAL_millis() / 1000;
    s_session.mood             = mood;
    s_session.time_of_day      = time_of_day;
    s_session.duration_seconds = 0;
    s_session.completed        = false;

    if (activity) {
        s_session.activity_id = activity->id;
        strncpy(s_session.activity_name, activity->name, ACTIVITY_NAME_LENGTH - 1);
        s_session.activity_name[ACTIVITY_NAME_LENGTH - 1] = '\0';
    } else {
        s_session.activity_id = 0;
        s_session.activity_name[0] = '\0';
    }

    s_active = true;
}

void session_end(bool completed) {
    if (!s_active) {
        return;
    }
    s_session.completed        = completed;
    s_session.duration_seconds = (HAL_millis() - s_start_time) / 1000;
    logger_log_session(&s_session);
    s_active = false;
}

bool session_is_active() {
    return s_active;
}

static void on_session_started(const Event* event) {
    const SessionStartPayload* p = (const SessionStartPayload*)event->payload.raw;
    session_begin(p->mood, p->activity, p->time_of_day);
}

static void on_session_completed(const Event* event) {
    const SessionEndPayload* p = (const SessionEndPayload*)event->payload.raw;
    if (s_active) {
        session_end(p->completed);
    }
}

void session_manager_init() {
    event_bus_subscribe(SESSION_STARTED,   on_session_started);
    event_bus_subscribe(SESSION_COMPLETED, on_session_completed);
}
