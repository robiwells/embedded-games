/**
 * @file test_main.cpp
 * @brief Unit tests for nfc_validate_uid() and nfc_get_mood_name() pure functions
 */

#include <unity.h>
#include <stdint.h>
#include <string.h>

// Include mock (provides nfc_validate_uid and nfc_get_mood_name implementations)
#include "../../test/mocks/nfc_handler_mock.cpp"

// Include headers for types
#include "../../include/nfc_handler.h"
#include "../../include/config.h"

// Test UIDs — copied from mapping table in nfc_handler_mock.cpp
static const uint8_t uid_happy[]     = {0x04, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6};
static const uint8_t uid_sad[]       = {0x04, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0xA1};
static const uint8_t uid_calm[]      = {0x04, 0xC3, 0xD4, 0xE5, 0xF6, 0xA1, 0xB2};
static const uint8_t uid_energetic[] = {0x04, 0xD4, 0xE5, 0xF6, 0xA1, 0xB2, 0xC3};
static const uint8_t uid_anxious[]   = {0x04, 0xE5, 0xF6, 0xA1, 0xB2, 0xC3, 0xD4};
static const uint8_t uid_angry[]     = {0x04, 0xF6, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5};
static const uint8_t uid_invalid[]   = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void setUp(void) {}
void tearDown(void) {}

// =============================================================================
// nfc_validate_uid() tests
// =============================================================================

void test_validate_uid_happy(void) {
    TEST_ASSERT_EQUAL(MOOD_HAPPY, nfc_validate_uid(uid_happy));
}

void test_validate_uid_sad(void) {
    TEST_ASSERT_EQUAL(MOOD_SAD, nfc_validate_uid(uid_sad));
}

void test_validate_uid_calm(void) {
    TEST_ASSERT_EQUAL(MOOD_CALM, nfc_validate_uid(uid_calm));
}

void test_validate_uid_energetic(void) {
    TEST_ASSERT_EQUAL(MOOD_ENERGETIC, nfc_validate_uid(uid_energetic));
}

void test_validate_uid_anxious(void) {
    TEST_ASSERT_EQUAL(MOOD_ANXIOUS, nfc_validate_uid(uid_anxious));
}

void test_validate_uid_angry(void) {
    TEST_ASSERT_EQUAL(MOOD_ANGRY, nfc_validate_uid(uid_angry));
}

void test_validate_uid_unknown(void) {
    TEST_ASSERT_EQUAL(MOOD_UNKNOWN, nfc_validate_uid(uid_invalid));
}

// =============================================================================
// nfc_get_mood_name() tests
// =============================================================================

void test_get_mood_name_happy(void) {
    TEST_ASSERT_EQUAL_STRING("Happy", nfc_get_mood_name(MOOD_HAPPY));
}

void test_get_mood_name_sad(void) {
    TEST_ASSERT_EQUAL_STRING("Sad", nfc_get_mood_name(MOOD_SAD));
}

void test_get_mood_name_calm(void) {
    TEST_ASSERT_EQUAL_STRING("Calm", nfc_get_mood_name(MOOD_CALM));
}

void test_get_mood_name_energetic(void) {
    TEST_ASSERT_EQUAL_STRING("Energetic", nfc_get_mood_name(MOOD_ENERGETIC));
}

void test_get_mood_name_anxious(void) {
    TEST_ASSERT_EQUAL_STRING("Anxious", nfc_get_mood_name(MOOD_ANXIOUS));
}

void test_get_mood_name_angry(void) {
    TEST_ASSERT_EQUAL_STRING("Angry", nfc_get_mood_name(MOOD_ANGRY));
}

void test_get_mood_name_unknown(void) {
    TEST_ASSERT_EQUAL_STRING("Unknown", nfc_get_mood_name((MoodCategory)255));
}

// =============================================================================
// Test Runner
// =============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_validate_uid_happy);
    RUN_TEST(test_validate_uid_sad);
    RUN_TEST(test_validate_uid_calm);
    RUN_TEST(test_validate_uid_energetic);
    RUN_TEST(test_validate_uid_anxious);
    RUN_TEST(test_validate_uid_angry);
    RUN_TEST(test_validate_uid_unknown);

    RUN_TEST(test_get_mood_name_happy);
    RUN_TEST(test_get_mood_name_sad);
    RUN_TEST(test_get_mood_name_calm);
    RUN_TEST(test_get_mood_name_energetic);
    RUN_TEST(test_get_mood_name_anxious);
    RUN_TEST(test_get_mood_name_angry);
    RUN_TEST(test_get_mood_name_unknown);

    return UNITY_END();
}
