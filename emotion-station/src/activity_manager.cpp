#include "activity_manager.h"
#include "platform_hal.h"
#include <string.h>

#ifndef WOKWI_SIMULATION
#include <SD.h>
#include <ArduinoJson.h>
#endif

static Activity activities[MAX_ACTIVITIES];
static uint8_t activity_count = 0;

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

    const struct {
        uint8_t       id;
        MoodCategory  mood;
        const char*   name;
        const char*   file_path;
        uint16_t      duration_seconds;
        const char*   type;
    } mock[] = {
        { 1, MOOD_HAPPY,     "Sunshine Dance",   "/audio/happy/25_sunshine_dance.mp3",   120, "movement"  },
        { 2, MOOD_SAD,       "Rainbow Breath",   "/audio/sad/17_rainbow_breath.mp3",     180, "breathing" },
        { 3, MOOD_CALM,      "Slow Breathing",   "/audio/calm/41_slow_breathing.mp3",    240, "breathing" },
        { 4, MOOD_ENERGETIC, "Movement One",     "/audio/energetic/movement_1.mp3",       90, "movement"  },
        { 5, MOOD_ANXIOUS,   "Bubble Breath",    "/audio/anxious/9_bubble_breath.mp3",   180, "breathing" },
        { 6, MOOD_ANGRY,     "Dragon Breath",    "/audio/angry/1_dragon_breath.mp3",     120, "breathing" },
    };

    activity_count = 0;
    for (uint8_t i = 0; i < 6; i++) {
        Activity* a = &activities[activity_count++];
        a->id               = mock[i].id;
        a->mood             = mock[i].mood;
        a->duration_seconds = mock[i].duration_seconds;
        strncpy(a->name,      mock[i].name,      ACTIVITY_NAME_LENGTH - 1);
        strncpy(a->file_path, mock[i].file_path, ACTIVITY_PATH_LENGTH - 1);
        strncpy(a->type,      mock[i].type,       ACTIVITY_TYPE_LENGTH - 1);
        a->name[ACTIVITY_NAME_LENGTH - 1]      = '\0';
        a->file_path[ACTIVITY_PATH_LENGTH - 1] = '\0';
        a->type[ACTIVITY_TYPE_LENGTH - 1]      = '\0';
        for (int t = 0; t < 4; t++) a->time_flags[t] = true;
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

Activity* activity_select(MoodCategory mood, TimeOfDay time) {
    for (uint8_t i = 0; i < activity_count; i++) {
        if (activities[i].mood == mood && activities[i].time_flags[time]) {
            return &activities[i];
        }
    }
    // Fallback: match mood regardless of time
    for (uint8_t i = 0; i < activity_count; i++) {
        if (activities[i].mood == mood) {
            return &activities[i];
        }
    }
    return nullptr;
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
