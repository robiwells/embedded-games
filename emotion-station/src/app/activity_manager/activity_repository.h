#ifndef ACTIVITY_REPOSITORY_H
#define ACTIVITY_REPOSITORY_H

#include "config.h"

typedef struct {
    bool (*load)(Activity* out, uint8_t max, uint8_t* out_count);
} ActivityRepository;

// Global pointer — set before activity_manager_init() is called.
extern const ActivityRepository* activity_repository;

// Concrete implementations (link the appropriate one per build env)
extern const ActivityRepository* activity_repository_sd;
extern const ActivityRepository* activity_repository_wokwi;
extern const ActivityRepository* activity_repository_mock;

#ifdef UNIT_TEST
// Test helper — load arr into the mock repository
void activity_repository_mock_set(const Activity* arr, uint8_t count);
#endif

#endif // ACTIVITY_REPOSITORY_H
