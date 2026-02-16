/**
 * @file activity_repository_wokwi.cpp
 * @brief Activity repository implementation — hardcoded mock data (wokwi env)
 */

#include "activity_repository.h"
#include "platform_hal.h"
#include <string.h>

static bool wokwi_load(Activity* out, uint8_t max, uint8_t* out_count) {
    HAL_log_println("[ACTIVITY] Wokwi simulation: loading mock activities");

    static const struct {
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

    *out_count = 0;
    uint8_t n = (uint8_t)(sizeof(mock) / sizeof(mock[0]));
    for (uint8_t i = 0; i < n && *out_count < max; i++) {
        Activity* a = &out[(*out_count)++];
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

static const ActivityRepository s_wokwi_repository = { wokwi_load };
const ActivityRepository* activity_repository_wokwi = &s_wokwi_repository;
