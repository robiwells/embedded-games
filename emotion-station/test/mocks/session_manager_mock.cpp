/**
 * @file session_manager_mock.cpp
 * @brief Stub session manager for unit testing
 */

#include "session_manager.h"

static bool s_active = false;

void session_begin(MoodCategory mood, const Activity* activity, TimeOfDay time_of_day) {
    (void)mood;
    (void)activity;
    (void)time_of_day;
    s_active = true;
}

void session_end(bool completed) {
    (void)completed;
    s_active = false;
}

bool session_is_active() {
    return s_active;
}
