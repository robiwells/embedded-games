/**
 * @file activity_repository_mock.cpp
 * @brief Injectable test repository for unit tests
 */

#include "activity_repository.h"
#include <string.h>

static const Activity* mock_arr = nullptr;
static uint8_t         mock_count = 0;

void activity_repository_mock_set(const Activity* arr, uint8_t count) {
    mock_arr   = arr;
    mock_count = count;
}

static bool mock_load(Activity* out, uint8_t max, uint8_t* out_count) {
    *out_count = 0;
    if (!mock_arr) return true;
    uint8_t n = mock_count < max ? mock_count : max;
    for (uint8_t i = 0; i < n; i++) {
        out[i] = mock_arr[i];
    }
    *out_count = n;
    return true;
}

static const ActivityRepository s_mock_repository = { mock_load };
const ActivityRepository* activity_repository_mock = &s_mock_repository;

// Global pointer defined here for native test env (only one translation unit sets it)
const ActivityRepository* activity_repository = &s_mock_repository;
