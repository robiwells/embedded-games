#include "activity_manager.h"
#include "time_service.h"
#include "platform_hal.h"
#include <string.h>
#include <stdio.h>

#if !defined(WOKWI_SIMULATION) && !defined(UNIT_TEST)
#include <SD.h>
#include <ArduinoJson.h>
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

static void csv_sanitise(char* field) {
    if (field[0] == '=' || field[0] == '+' || field[0] == '-' || field[0] == '@')
        field[0] = '_';
}

static MoodCategory mood_from_string(const char* s) {
    if (strcmp(s, "happy")     == 0) return MOOD_HAPPY;
    if (strcmp(s, "sad")       == 0) return MOOD_SAD;
    if (strcmp(s, "calm")      == 0) return MOOD_CALM;
    if (strcmp(s, "energetic") == 0) return MOOD_ENERGETIC;
    if (strcmp(s, "anxious")   == 0) return MOOD_ANXIOUS;
    if (strcmp(s, "angry")     == 0) return MOOD_ANGRY;
    return MOOD_UNKNOWN;
}

// ---------------------------------------------------------------------------
// Wokwi simulation — hardcoded activities (SD not supported in Wokwi)
// ---------------------------------------------------------------------------

#ifdef UNIT_TEST

bool activity_manager_init() {
    // Stub for unit tests — use activity_test_inject() to load activities
    return true;
}

#elif defined(WOKWI_SIMULATION)

bool activity_manager_init() {
    HAL_log_println("[ACTIVITY] Wokwi simulation: loading mock activities");

    // time_flags: [morning, afternoon, evening, bedtime]
    const struct {
        uint8_t      id;
        MoodCategory mood;
        const char*  name;
        const char*  file_path;
        uint16_t     duration_seconds;
        const char*  type;
        bool         time_flags[4];
    } mock[] = {
        {  1, MOOD_HAPPY,     "Sunshine Dance",    "/audio/happy/25_sunshine_dance.mp3",  120, "movement",      {true,  true,  true,  false} },
        {  2, MOOD_HAPPY,     "Joy Jump",          "/audio/happy/joy_jump.mp3",            90, "movement",      {true,  true,  false, false} },
        {  3, MOOD_HAPPY,     "Giggle Breath",     "/audio/happy/giggle_breath.mp3",      150, "breathing",     {false, false, true,  true } },
        {  4, MOOD_SAD,       "Rainbow Breath",    "/audio/sad/17_rainbow_breath.mp3",    180, "breathing",     {true,  true,  true,  true } },
        {  5, MOOD_SAD,       "Cosy Cloud",        "/audio/sad/cosy_cloud.mp3",           200, "visualisation", {false, false, true,  true } },
        {  6, MOOD_SAD,       "Sunshine Hug",      "/audio/sad/sunshine_hug.mp3",         160, "visualisation", {true,  true,  false, false} },
        {  7, MOOD_CALM,      "Slow Breathing",    "/audio/calm/41_slow_breathing.mp3",   240, "breathing",     {true,  true,  true,  true } },
        {  8, MOOD_CALM,      "Peaceful Pond",     "/audio/calm/peaceful_pond.mp3",       300, "visualisation", {false, false, true,  true } },
        {  9, MOOD_CALM,      "Gentle Stretch",    "/audio/calm/gentle_stretch.mp3",      180, "movement",      {true,  true,  false, false} },
        { 10, MOOD_ENERGETIC, "Movement One",      "/audio/energetic/movement_1.mp3",      90, "movement",      {true,  true,  false, false} },
        { 11, MOOD_ENERGETIC, "Star Jump Fun",     "/audio/energetic/star_jump_fun.mp3",  120, "movement",      {true,  true,  false, false} },
        { 12, MOOD_ENERGETIC, "Calm Down Breath",  "/audio/energetic/calm_down_breath.mp3",180,"breathing",     {false, false, true,  true } },
        { 13, MOOD_ANXIOUS,   "Bubble Breath",     "/audio/anxious/9_bubble_breath.mp3",  180, "breathing",     {true,  true,  true,  true } },
        { 14, MOOD_ANXIOUS,   "Safe Place",        "/audio/anxious/safe_place.mp3",       240, "visualisation", {false, false, true,  true } },
        { 15, MOOD_ANXIOUS,   "Five Senses",       "/audio/anxious/five_senses.mp3",      200, "mindfulness",   {true,  true,  false, false} },
        { 16, MOOD_ANGRY,     "Dragon Breath",     "/audio/angry/1_dragon_breath.mp3",    120, "breathing",     {true,  true,  true,  false} },
        { 17, MOOD_ANGRY,     "Volcano Stomp",     "/audio/angry/volcano_stomp.mp3",       90, "movement",      {true,  true,  false, false} },
        { 18, MOOD_ANGRY,     "Cool Down Cloud",   "/audio/angry/cool_down_cloud.mp3",    180, "visualisation", {false, false, true,  true } },
    };

    activity_count = 0;
    for (uint8_t i = 0; i < 18; i++) {
        Activity* a = &activities[activity_count++];
        a->id               = mock[i].id;
        a->mood             = mock[i].mood;
        a->duration_seconds = mock[i].duration_seconds;
        strncpy(a->name,      mock[i].name,      ACTIVITY_NAME_LENGTH - 1);
        strncpy(a->file_path, mock[i].file_path, ACTIVITY_PATH_LENGTH - 1);
        strncpy(a->type,      mock[i].type,      ACTIVITY_TYPE_LENGTH - 1);
        a->name[ACTIVITY_NAME_LENGTH - 1]      = '\0';
        a->file_path[ACTIVITY_PATH_LENGTH - 1] = '\0';
        a->type[ACTIVITY_TYPE_LENGTH - 1]      = '\0';
        csv_sanitise(a->name);
        for (int t = 0; t < 4; t++) a->time_flags[t] = mock[i].time_flags[t];
    }

    HAL_log_println("[ACTIVITY] Mock activities loaded");
    return true;
}

#else // Real hardware

// ---------------------------------------------------------------------------
// Real hardware — parse activities.json from SD card
// ---------------------------------------------------------------------------

static StaticJsonDocument<8192> s_json_doc;

bool activity_manager_init() {
    s_json_doc.clear();
    HAL_log_println("[ACTIVITY] Initialising SD card");

    if (!SD.begin(SD_CS_PIN)) {
        HAL_log_println("[ACTIVITY] ERROR: SD card mount failed");
        return false;
    }
    HAL_log_println("[ACTIVITY] SD card mounted");

    File f = SD.open("/activities.json");
    if (!f) {
        HAL_log_println("[ACTIVITY] ERROR: /activities.json not found");
        return false;
    }

    DeserializationError err = deserializeJson(s_json_doc, f);
    f.close();

    if (err) {
        HAL_log_println("[ACTIVITY] ERROR: JSON parse failed");
        return false;
    }

    JsonArray arr = s_json_doc["activities"].as<JsonArray>();
    if (arr.isNull()) {
        HAL_log_println("[ACTIVITY] ERROR: no 'activities' array in JSON");
        return false;
    }

    activity_count = 0;
    randomSeed(analogRead(34));

    for (JsonObject obj : arr) {
        if (activity_count >= MAX_ACTIVITIES) break;

        MoodCategory mood = mood_from_string(obj["mood"] | "");
        if (mood == MOOD_UNKNOWN) continue;

        Activity* a = &activities[activity_count++];
        a->id               = obj["id"]                | 0;
        a->mood             = mood;
        a->duration_seconds = obj["duration_seconds"]  | 0;

        strncpy(a->name,      obj["name"]      | "", ACTIVITY_NAME_LENGTH - 1);
        strncpy(a->file_path, obj["file_path"] | "", ACTIVITY_PATH_LENGTH - 1);
        strncpy(a->type,      obj["type"]      | "", ACTIVITY_TYPE_LENGTH - 1);
        a->name[ACTIVITY_NAME_LENGTH - 1]      = '\0';
        a->file_path[ACTIVITY_PATH_LENGTH - 1] = '\0';
        a->type[ACTIVITY_TYPE_LENGTH - 1]      = '\0';
        csv_sanitise(a->name);

        JsonArray times = obj["time_of_day"].as<JsonArray>();
        for (int t = 0; t < 4; t++) a->time_flags[t] = false;
        for (const char* ts : times) {
            if (strcmp(ts, "morning")   == 0) a->time_flags[TIME_MORNING]   = true;
            if (strcmp(ts, "afternoon") == 0) a->time_flags[TIME_AFTERNOON] = true;
            if (strcmp(ts, "evening")   == 0) a->time_flags[TIME_EVENING]   = true;
            if (strcmp(ts, "bedtime")   == 0) a->time_flags[TIME_BEDTIME]   = true;
        }

        // Skip activities that are unreachable at any time of day
        bool has_time = false;
        for (int t = 0; t < 4; t++) { if (a->time_flags[t]) { has_time = true; break; } }
        if (!has_time) {
            char wbuf[64];
            snprintf(wbuf, sizeof(wbuf), "[ACTIVITY] WARNING: id=%u '%s' has no time_flags, skipping", a->id, a->name);
            HAL_log_println(wbuf);
            activity_count--;
        }
    }

    char buf[48];
    snprintf(buf, sizeof(buf), "[ACTIVITY] Loaded %u activities", activity_count);
    HAL_log_println(buf);
    return true;
}

#endif // WOKWI_SIMULATION

// ---------------------------------------------------------------------------
// Public API (shared between simulation and real hardware)
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
// Pipeline stage helpers
// ---------------------------------------------------------------------------

// Context passed to every stage so each has access to mood and time-of-day
typedef struct {
    MoodCategory mood;
    TimeOfDay    time;
} SelectionContext;

// Uniform stage signature: filter in[] → out[], return new count.
// Returning 0 signals "no candidates" — the orchestrator falls back to in[].
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
        // Return 0 — orchestrator will fall back to the previous pool
    }
    return count;
}

// Pipeline stage descriptor — pairs a stage function with its failure mode:
//   required = true  → empty result is a hard failure; return nullptr immediately
//   required = false → empty result falls back to the previous pool (soft filter)
typedef struct {
    StageFn fn;
    bool    required;
} PipelineStage;

// Ordered pipeline — add, remove or reorder stages here
static const PipelineStage pipeline[] = {
    { stage_filter_by_mood,    true  },   // hard: must match mood
    { stage_filter_by_time,    false },   // soft: fall back to mood pool
    { stage_exclude_recent,    false },   // soft: fall back to time/mood pool
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

    // Seed the pool with every activity index
    uint8_t buf_a[MAX_ACTIVITIES];
    uint8_t buf_b[MAX_ACTIVITIES];
    for (uint8_t i = 0; i < activity_count; i++) buf_a[i] = i;
    uint8_t* current    = buf_a;
    uint8_t* scratch    = buf_b;
    uint8_t  pool_size  = activity_count;

    // Run each stage
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
            // current and pool_size remain unchanged
        } else {
            // Swap buffers — scratch becomes current for the next stage
            uint8_t* tmp = current; current = scratch; scratch = tmp;
            pool_size = out_count;
        }
    }

    // Single candidate — skip history to avoid thrashing
    if (pool_size == 1) {
        Activity* selected = &activities[current[0]];
        snprintf(buf, sizeof(buf),
                 "[ACTIVITY] WARNING: only 1 activity — no variety possible: '%s'",
                 selected->name);
        HAL_log_println(buf);
        return selected;
    }

    // Terminal stage: random pick — reseed every 10 selections for better entropy
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

#ifdef UNIT_TEST
void activity_test_inject(const Activity* arr, uint8_t count) {
    activity_count = count < MAX_ACTIVITIES ? count : MAX_ACTIVITIES;
    for (uint8_t i = 0; i < activity_count; i++) {
        activities[i] = arr[i];
    }
}

void activity_history_clear() {
    for (uint8_t m = 0; m < NUM_MOODS; m++) {
        for (uint8_t i = 0; i < HISTORY_SIZE; i++) {
            activity_history.recent_ids[m][i] = 0;
        }
        activity_history.history_index[m] = 0;
    }
}
#endif

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
