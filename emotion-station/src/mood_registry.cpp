#include "mood_registry.h"

static const MoodDefinition moods[NUM_MOODS] = {
    {
        MOOD_HAPPY,
        "Happy",
        "/audio/happy/25_sunshine_dance.mp3",
        {0x04, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6}
    },
    {
        MOOD_SAD,
        "Sad",
        "/audio/sad/17_rainbow_breath.mp3",
        {0x04, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0xA1}
    },
    {
        MOOD_CALM,
        "Calm",
        "/audio/calm/41_slow_breathing.mp3",
        {0x04, 0xC3, 0xD4, 0xE5, 0xF6, 0xA1, 0xB2}
    },
    {
        MOOD_ENERGETIC,
        "Energetic",
        "/audio/energetic/movement_1.mp3",
        {0x04, 0xD4, 0xE5, 0xF6, 0xA1, 0xB2, 0xC3}
    },
    {
        MOOD_ANXIOUS,
        "Anxious",
        "/audio/anxious/9_bubble_breath.mp3",
        {0x04, 0xE5, 0xF6, 0xA1, 0xB2, 0xC3, 0xD4}
    },
    {
        MOOD_ANGRY,
        "Angry",
        "/audio/angry/1_dragon_breath.mp3",
        {0x04, 0xF6, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5}
    },
};

const MoodDefinition* mood_registry_get(MoodCategory mood) {
    if (mood >= NUM_MOODS) return nullptr;
    return &moods[mood];
}

const MoodDefinition* mood_registry_all() {
    return moods;
}

const char* mood_registry_name(MoodCategory mood) {
    if (mood >= NUM_MOODS) return "Unknown";
    return moods[mood].name;
}

const char* mood_registry_fallback_audio(MoodCategory mood) {
    if (mood >= NUM_MOODS) return "";
    return moods[mood].fallback_audio;
}
