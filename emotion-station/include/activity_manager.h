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

#endif
