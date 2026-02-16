#ifndef MOOD_REGISTRY_H
#define MOOD_REGISTRY_H

#include "config.h"

typedef struct {
    MoodCategory category;
    const char*  name;
    const char*  fallback_audio;
    uint8_t      test_uid[7];   // Wokwi simulation UID
} MoodDefinition;

// Returns the definition for the given mood, or NULL if mood is out of range.
const MoodDefinition* mood_registry_get(MoodCategory mood);

// Returns pointer to the full array (NUM_MOODS entries).
const MoodDefinition* mood_registry_all();

// Returns the display name for the given mood (e.g. "Happy").
const char* mood_registry_name(MoodCategory mood);

// Returns the fallback audio path for the given mood.
const char* mood_registry_fallback_audio(MoodCategory mood);

#endif // MOOD_REGISTRY_H
