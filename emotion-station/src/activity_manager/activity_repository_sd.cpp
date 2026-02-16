/**
 * @file activity_repository_sd.cpp
 * @brief Activity repository implementation — loads from SD card JSON (esp32dev)
 */

#include "activity_repository.h"
#include "platform_hal.h"
#include <SD.h>
#include <ArduinoJson.h>
#include <string.h>

static StaticJsonDocument<8192> s_json_doc;

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

static bool sd_load(Activity* out, uint8_t max, uint8_t* out_count) {
    *out_count = 0;
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

    randomSeed(analogRead(34));

    for (JsonObject obj : arr) {
        if (*out_count >= max) break;

        MoodCategory mood = mood_from_string(obj["mood"] | "");
        if (mood == MOOD_UNKNOWN) continue;

        Activity* a = &out[(*out_count)++];
        a->id               = obj["id"]               | 0;
        a->mood             = mood;
        a->duration_seconds = obj["duration_seconds"] | 0;

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

        // Skip activities unreachable at any time of day
        bool has_time = false;
        for (int t = 0; t < 4; t++) { if (a->time_flags[t]) { has_time = true; break; } }
        if (!has_time) {
            char wbuf[64];
            snprintf(wbuf, sizeof(wbuf), "[ACTIVITY] WARNING: id=%u '%s' has no time_flags, skipping",
                     a->id, a->name);
            HAL_log_println(wbuf);
            (*out_count)--;
        }
    }

    char buf[48];
    snprintf(buf, sizeof(buf), "[ACTIVITY] Loaded %u activities", *out_count);
    HAL_log_println(buf);
    return true;
}

static const ActivityRepository s_sd_repository = { sd_load };
const ActivityRepository* activity_repository_sd = &s_sd_repository;
