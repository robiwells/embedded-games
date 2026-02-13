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
