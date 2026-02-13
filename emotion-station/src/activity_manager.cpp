#include "activity_manager.h"
#include "platform_hal.h"
#include <string.h>

#ifndef WOKWI_SIMULATION
#include <SD.h>
#include <ArduinoJson.h>
#endif

static Activity activities[MAX_ACTIVITIES];
static uint8_t activity_count = 0;

#ifdef WOKWI_SIMULATION
static uint8_t s_sim_time_offset_hours = 12;

void activity_set_sim_time(uint8_t hour) {
    // Adjust offset so that (millis/60000 + offset) % 24 == hour
    uint32_t elapsed_hours = (HAL_millis() / 60000UL) % 24;
    s_sim_time_offset_hours = (uint8_t)((hour + 24 - elapsed_hours) % 24);
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

#ifdef WOKWI_SIMULATION

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
        for (int t = 0; t < 4; t++) a->time_flags[t] = mock[i].time_flags[t];
    }

    HAL_log_println("[ACTIVITY] Mock activities loaded");
    return true;
}

#else // Real hardware

// ---------------------------------------------------------------------------
// Real hardware — parse activities.json from SD card
// ---------------------------------------------------------------------------

bool activity_manager_init() {
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

    StaticJsonDocument<8192> doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) {
        HAL_log_println("[ACTIVITY] ERROR: JSON parse failed");
        return false;
    }

    JsonArray arr = doc["activities"].as<JsonArray>();
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

        JsonArray times = obj["time_of_day"].as<JsonArray>();
        for (int t = 0; t < 4; t++) a->time_flags[t] = false;
        for (const char* ts : times) {
            if (strcmp(ts, "morning")   == 0) a->time_flags[TIME_MORNING]   = true;
            if (strcmp(ts, "afternoon") == 0) a->time_flags[TIME_AFTERNOON] = true;
            if (strcmp(ts, "evening")   == 0) a->time_flags[TIME_EVENING]   = true;
            if (strcmp(ts, "bedtime")   == 0) a->time_flags[TIME_BEDTIME]   = true;
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
#ifdef WOKWI_SIMULATION
    // 1 real minute = 1 simulated hour, wraps at 24
    uint32_t simulated_hour = ((HAL_millis() / 60000UL) + s_sim_time_offset_hours) % 24;
    if (simulated_hour >= 6  && simulated_hour < 12) return TIME_MORNING;
    if (simulated_hour >= 12 && simulated_hour < 17) return TIME_AFTERNOON;
    if (simulated_hour >= 17 && simulated_hour < 21) return TIME_EVENING;
    return TIME_BEDTIME;
#else
    return TIME_AFTERNOON; // Hardcoded until Phase 6.5 adds RTC
#endif
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

Activity* activity_select(MoodCategory mood, TimeOfDay time) {
    // Stage 1: filter by mood
    uint8_t mood_candidates[MAX_ACTIVITIES];
    uint8_t mood_count = 0;
    for (uint8_t i = 0; i < activity_count; i++) {
        if (activities[i].mood == mood) {
            mood_candidates[mood_count++] = i;
        }
    }
    if (mood_count == 0) {
        HAL_log_println("[ACTIVITY] Stage 1 FAIL: no activities for mood");
        return nullptr;
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "[ACTIVITY] Stage 1: %u mood candidates", mood_count);
    HAL_log_println(buf);

    // Stage 2: filter by time — fall back to mood candidates if none match
    uint8_t time_candidates[MAX_ACTIVITIES];
    uint8_t time_count = 0;
    for (uint8_t i = 0; i < mood_count; i++) {
        if (activities[mood_candidates[i]].time_flags[time]) {
            time_candidates[time_count++] = mood_candidates[i];
        }
    }
    uint8_t* pool      = time_count > 0 ? time_candidates : mood_candidates;
    uint8_t  pool_size = time_count > 0 ? time_count      : mood_count;
    snprintf(buf, sizeof(buf), "[ACTIVITY] Stage 2: %u time candidates (time=%s)",
             time_count, activity_get_time_name(time));
    HAL_log_println(buf);
    if (time_count == 0) {
        HAL_log_println("[ACTIVITY] Stage 2: no time match, using mood pool");
    }

    // Early return when only 1 candidate — history would fill and clear every play
    if (pool_size <= 1) {
        Activity* selected = &activities[pool[0]];
        snprintf(buf, sizeof(buf), "[ACTIVITY] Only 1 candidate, skipping history: '%s'", selected->name);
        HAL_log_println(buf);
        return selected;
    }

    // Stage 3: remove recently played
    uint8_t fresh_candidates[MAX_ACTIVITIES];
    uint8_t fresh_count = 0;
    for (uint8_t i = 0; i < pool_size; i++) {
        if (!activity_is_recent(mood, activities[pool[i]].id)) {
            fresh_candidates[fresh_count++] = pool[i];
        }
    }
    if (fresh_count == 0) {
        // All candidates played recently — clear history and retry
        HAL_log_println("[ACTIVITY] Stage 3: history full, clearing for this mood");
        for (uint8_t i = 0; i < HISTORY_SIZE; i++) {
            activity_history.recent_ids[mood][i] = 0;
        }
        activity_history.history_index[mood] = 0;
        fresh_count = pool_size;
        for (uint8_t i = 0; i < pool_size; i++) {
            fresh_candidates[i] = pool[i];
        }
    }
    snprintf(buf, sizeof(buf), "[ACTIVITY] Stage 3: %u fresh candidates", fresh_count);
    HAL_log_println(buf);

    // Stage 4: random pick
    uint8_t pick = (uint8_t)(random(fresh_count));
    Activity* selected = &activities[fresh_candidates[pick]];
    activity_add_to_history(mood, selected->id);
    snprintf(buf, sizeof(buf), "[ACTIVITY] Stage 4: selected '%s' (id=%u)",
             selected->name, selected->id);
    HAL_log_println(buf);
    return selected;
}

uint8_t activity_get_count() {
    return activity_count;
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
