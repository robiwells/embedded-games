#include "activity_repository.h"

// Default global repository pointer — set to the appropriate implementation
// before calling activity_manager_init() (see main.cpp).
// In test builds this file is not compiled; activity_repository_mock.cpp
// provides the definition instead.
const ActivityRepository* activity_repository = nullptr;
