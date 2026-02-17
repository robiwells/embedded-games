#include "activity_manager.h"
#include "activity_repository.h"
#include "time_service.h"
#include "platform_hal.h"
#include <string.h>
#include <stdio.h>

// Global repository pointer — defined here for hardware/wokwi builds.
// The native test env overrides this in activity_repository_mock.cpp.
#ifndef UNIT_TEST
const ActivityRepository* activity_repository = nullptr;
#endif

static Activity activities[MAX_ACTIVITIES];
static uint8_t activity_count = 0;

#ifdef WOKWI_SIMULATION
void activity_set_sim_time(uint8_t hour) {
    time_service_set_mock_hour(hour);
}
#endif

// ---------------------------------------------------------------------------
// History tracking — avoid recent repeats per mood
// ---------------------------------------------------------------------------

#define HISTORY_SIZE 5

typedef struct {
    uint8_t recent_ids[NUM_MOODS][HISTORY_SIZE];
    uint8_t history_index[NUM_MOODS];
} ActivityHistory;

static ActivityHistory activity_history = {{{0}}, {0}};

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static bool activity_is_recent(MoodCategory mood, uint8_t id) {
    for (uint8_t i = 0; i < HISTORY_SIZE; i++) {
        if (activity_history.recent_ids[mood][i] == id && id != 0) {
            return true;
        }
    }
    return false;
}

static void activity_add_to_history(MoodCategory mood, uint8_t id) {
    uint8_t idx = activity_history.history_index[mood];
    activity_history.recent_ids[mood][idx] = id;
    activity_history.history_index[mood] = (idx + 1) % HISTORY_SIZE;
}

// ---------------------------------------------------------------------------
// Initialisation — delegates to repository
// ---------------------------------------------------------------------------

bool activity_manager_init() {
    if (!activity_repository || !activity_repository->load) {
        HAL_log_println("[ACTIVITY] ERROR: no repository set");
        return false;
    }
    return activity_repository->load(activities, MAX_ACTIVITIES, &activity_count);
}

// ---------------------------------------------------------------------------
// Public API (shared between all environments)
// ---------------------------------------------------------------------------

TimeOfDay activity_get_time_of_day() {
    return time_service_get_time_of_day();
}

const char* activity_get_time_name(TimeOfDay time) {
    switch (time) {
        case TIME_MORNING:   return "Morning";
        case TIME_AFTERNOON: return "Afternoon";
        case TIME_EVENING:   return "Evening";
        case TIME_BEDTIME:   return "Bedtime";
        default:             return "Unknown";
    }
}

// ---------------------------------------------------------------------------
// Pipeline stage helpers
// ---------------------------------------------------------------------------

typedef struct {
    MoodCategory mood;
    TimeOfDay    time;
} SelectionContext;

typedef uint8_t (*StageFn)(const uint8_t* in,  uint8_t in_count,
                                  uint8_t* out, uint8_t out_max,
                            const SelectionContext* ctx);

static uint8_t stage_filter_by_mood(const uint8_t* in, uint8_t in_count,
                                          uint8_t* out, uint8_t out_max,
                                    const SelectionContext* ctx) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < in_count && count < out_max; i++) {
        if (activities[in[i]].mood == ctx->mood) {
            out[count++] = in[i];
        }
    }
    return count;
}

static uint8_t stage_filter_by_time(const uint8_t* in, uint8_t in_count,
                                           uint8_t* out, uint8_t out_max,
                                     const SelectionContext* ctx) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < in_count && count < out_max; i++) {
        if (activities[in[i]].time_flags[ctx->time]) {
            out[count++] = in[i];
        }
    }
    return count;
}

static uint8_t stage_exclude_recent(const uint8_t* in, uint8_t in_count,
                                           uint8_t* out, uint8_t out_max,
                                     const SelectionContext* ctx) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < in_count && count < out_max; i++) {
        if (!activity_is_recent(ctx->mood, activities[in[i]].id)) {
            out[count++] = in[i];
        }
    }
    if (count == 0) {
        char buf[64];
        snprintf(buf, sizeof(buf),
                 "[ACTIVITY] Stage 3: history cleared for mood %u (all %u recently played)",
                 ctx->mood, in_count);
        HAL_log_println(buf);
        for (uint8_t i = 0; i < HISTORY_SIZE; i++) {
            activity_history.recent_ids[ctx->mood][i] = 0;
        }
        activity_history.history_index[ctx->mood] = 0;
    }
    return count;
}

typedef struct {
    StageFn fn;
    bool    required;
} PipelineStage;

static const PipelineStage pipeline[] = {
    { stage_filter_by_mood,    true  },
    { stage_filter_by_time,    false },
    { stage_exclude_recent,    false },
};
static const uint8_t NUM_PIPELINE_STAGES =
    (uint8_t)(sizeof(pipeline) / sizeof(pipeline[0]));

static Activity* stage_random_pick(uint8_t* pool, uint8_t count) {
    if (count == 0) return nullptr;
    uint8_t pick = (uint8_t)(random(count));
    return &activities[pool[pick]];
}

Activity* activity_select(MoodCategory mood, TimeOfDay time) {
    char buf[80];
    SelectionContext ctx = { mood, time };

    uint8_t buf_a[MAX_ACTIVITIES];
    uint8_t buf_b[MAX_ACTIVITIES];
    for (uint8_t i = 0; i < activity_count; i++) buf_a[i] = i;
    uint8_t* current   = buf_a;
    uint8_t* scratch   = buf_b;
    uint8_t  pool_size = activity_count;

    for (uint8_t s = 0; s < NUM_PIPELINE_STAGES; s++) {
        uint8_t out_count = pipeline[s].fn(current, pool_size, scratch, MAX_ACTIVITIES, &ctx);
        snprintf(buf, sizeof(buf), "[ACTIVITY] Stage %u: %u candidates", s + 1, out_count);
        HAL_log_println(buf);

        if (out_count == 0) {
            if (pipeline[s].required) {
                HAL_log_println("[ACTIVITY] Pipeline FAIL: required stage found no candidates");
                return nullptr;
            }
            HAL_log_println("[ACTIVITY] Stage fallback: keeping previous pool");
        } else {
            uint8_t* tmp = current; current = scratch; scratch = tmp;
            pool_size = out_count;
        }
    }

    if (pool_size == 1) {
        Activity* selected = &activities[current[0]];
        snprintf(buf, sizeof(buf),
                 "[ACTIVITY] WARNING: only 1 activity — no variety possible: '%s'",
                 selected->name);
        HAL_log_println(buf);
        return selected;
    }

    static uint16_t selection_count = 0;
    if (++selection_count % 10 == 0) {
#if !defined(UNIT_TEST)
        randomSeed((uint32_t)(analogRead(BATTERY_ADC_PIN)) ^ HAL_millis());
#endif
    }
    Activity* selected = stage_random_pick(current, pool_size);
    if (selected) {
        activity_add_to_history(mood, selected->id);
        snprintf(buf, sizeof(buf), "[ACTIVITY] Selected '%s' (id=%u)",
                 selected->name, selected->id);
        HAL_log_println(buf);
    }
    return selected;
}

uint8_t activity_get_count() {
    return activity_count;
}

void activity_history_clear() {
    for (uint8_t m = 0; m < NUM_MOODS; m++) {
        for (uint8_t i = 0; i < HISTORY_SIZE; i++) {
            activity_history.recent_ids[m][i] = 0;
        }
        activity_history.history_index[m] = 0;
    }
}

void activity_test_load() {
    HAL_log_println("[ACTIVITY] Test load:");
    for (uint8_t i = 0; i < activity_count; i++) {
        char buf[96];
        snprintf(buf, sizeof(buf), "  [%u] mood=%u name=%s path=%s",
                 activities[i].id, activities[i].mood,
                 activities[i].name, activities[i].file_path);
        HAL_log_println(buf);
    }
}
