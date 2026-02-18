#ifndef ACTIVITY_REPOSITORY_TEST_H
#define ACTIVITY_REPOSITORY_TEST_H

#include "config.h"

/**
 * @file activity_repository_test.h
 * @brief Test-only utilities for the activity repository mock.
 *
 * Include this in test builds to load fixture data into the mock repository.
 * Do NOT include in production code.
 */

void activity_repository_mock_set(const Activity* arr, uint8_t count);

#endif // ACTIVITY_REPOSITORY_TEST_H
