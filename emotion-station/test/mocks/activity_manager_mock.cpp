#include "activity_manager.h"

static Activity mock_activity = {
    1, MOOD_HAPPY, "Test Activity", "/audio/happy/test.mp3", 120, {true, true, true, true}, "breathing"
};

bool activity_manager_init() { return true; }

Activity* activity_select(MoodCategory mood, TimeOfDay time) {
    mock_activity.mood = mood;
    return &mock_activity;
}

uint8_t activity_get_count() { return 1; }

void activity_test_load() {}

TimeOfDay activity_get_time_of_day() { return TIME_AFTERNOON; }

#ifdef WOKWI_SIMULATION
void activity_set_sim_time(uint8_t hour) { (void)hour; }
#endif

const char* activity_get_time_name(TimeOfDay time) {
    switch (time) {
        case TIME_MORNING:   return "Morning";
        case TIME_AFTERNOON: return "Afternoon";
        case TIME_EVENING:   return "Evening";
        case TIME_BEDTIME:   return "Bedtime";
        default:             return "Unknown";
    }
}
