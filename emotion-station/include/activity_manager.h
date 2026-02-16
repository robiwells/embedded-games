#ifndef ACTIVITY_MANAGER_H
#define ACTIVITY_MANAGER_H

#include "config.h"

bool        activity_manager_init();
Activity*   activity_select(MoodCategory mood, TimeOfDay time);
uint8_t     activity_get_count();
void        activity_test_load();
TimeOfDay   activity_get_time_of_day();
const char* activity_get_time_name(TimeOfDay time);

#ifdef WOKWI_SIMULATION
void activity_set_sim_time(uint8_t hour);
#endif

#ifdef UNIT_TEST
// Test-only injection API — loads a fake activity library for unit tests
void activity_test_inject(const Activity* arr, uint8_t count);
void activity_history_clear();
#endif

#endif
