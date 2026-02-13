#ifndef ACTIVITY_MANAGER_H
#define ACTIVITY_MANAGER_H

#include "config.h"

bool      activity_manager_init();
Activity* activity_select(MoodCategory mood, TimeOfDay time);
uint8_t   activity_get_count();
void      activity_test_load();

#endif
