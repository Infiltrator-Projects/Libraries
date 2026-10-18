// SPDX-License-Identifier: GPL-3.0-or-later
#include "infiltratr/temporal_clock_resource.h"
#include <stdio.h>
#include <string.h>

#ifndef TEMPORAL_CLOCK_FIXTURE_PATH
#error TEMPORAL_CLOCK_FIXTURE_PATH is required
#endif

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "clock resource ABI test failed at line %d\n", __LINE__); \
    return 1; } } while (0)


/* All eighteen extracted clock IDs must fail closed without a system module.
 * Unmigrated IDs retain their legacy rendering during the migration. */
static int test_system_presentation(void)
{
    static const char *const migrated[] = {
        "decimal", "internet", "unix", "hexadecimal", "binary",
        "julian", "modified-julian", "chinese-ke", "mean-solar",
        "sidereal", "chinese-time", "solar", "italian-hours",
        "babylonian-hours", "babylonian-ancient",
        "indian-ghati", "standard-12", "standard-24", "roman-temporal",
        "japanese-temporal", "japanese-temporal-early", "nuremberg-hours",
        "nuremberg-solar"
    };
    InfiltratrTemporalClockPresentation *presentation;
    char buffer[64] = "unchanged";
    size_t length = 777U;
    uint32_t delay = 999U;
    size_t i;

    CHECK(infiltratr_temporal_clock_presentation_new(NULL) == NULL);
    CHECK(infiltratr_temporal_clock_presentation_new("Bad ID") == NULL);
    CHECK(infiltratr_temporal_clock_presentation_new("standard") == NULL);
    CHECK(infiltratr_temporal_clock_presentation_new("absent-clock") == NULL);

    for (i = 0U; i < sizeof(migrated) / sizeof(migrated[0]); ++i) {
        InfiltratrTemporalClockModule *installed =
            infiltratr_temporal_clock_module_open_id(migrated[i]);
        presentation = infiltratr_temporal_clock_presentation_new(migrated[i]);
        CHECK((installed == NULL) == (presentation == NULL));
        if (installed != NULL) {
            char expected[64] = {0};
            size_t expected_length = 0U;
            CHECK(infiltratr_temporal_clock_format(
                installed, "en", INT64_C(43200000000), 0,
                true, false, 0.0, 0.0, expected, sizeof(expected),
                &expected_length));
            CHECK(infiltratr_temporal_clock_presentation_format(
                presentation, "en", INT64_C(43200000000), 0,
                true, false, true, 0.0, 0.0, buffer, sizeof(buffer),
                &length));
            CHECK(length == expected_length && strcmp(buffer, expected) == 0);
            CHECK(infiltratr_temporal_clock_presentation_next_tick(
                presentation, INT64_C(43200000000), 0, true,
                0.0, 0.0, &delay));
            CHECK(delay > 0U);
        }
        infiltratr_temporal_clock_presentation_free(presentation);
        infiltratr_temporal_clock_module_free(installed);
    }
    infiltratr_temporal_clock_presentation_free(NULL);
    return 0;
}

int main(void)
{
    InfiltratrTemporalClockModule *module;
    const InfiltratrTemporalResourceMetadata *metadata;
    char buffer[32] = "unchanged";
    size_t length = 777U;
    uint32_t delay = 999U;

    CHECK(infiltratr_temporal_clock_module_open(NULL) == NULL);
    CHECK(infiltratr_temporal_clock_module_open("/missing/clock.so") == NULL);
    CHECK(infiltratr_temporal_clock_module_open_id(NULL) == NULL);
    CHECK(infiltratr_temporal_clock_module_open_id("Bad ID") == NULL);
    module = infiltratr_temporal_clock_module_open(TEMPORAL_CLOCK_FIXTURE_PATH);
    CHECK(module != NULL);
    metadata = infiltratr_temporal_clock_module_metadata(module);
    CHECK(metadata != NULL && strcmp(metadata->id, "fixture-clock") == 0);
    CHECK(infiltratr_temporal_clock_format(module, "en", INT64_C(1234567),
        0, true, false, 0.0, 0.0, buffer, sizeof(buffer), &length));
    CHECK(strcmp(buffer, "42:99") == 0 && length == 5U);
    strcpy(buffer, "unchanged");
    length = 777U;
    CHECK(!infiltratr_temporal_clock_format(module, "en", INT64_C(1234567),
        0, false, false, 0.0, 0.0, buffer, 2U, &length));
    CHECK(strcmp(buffer, "unchanged") == 0 && length == 777U);
    CHECK(infiltratr_temporal_clock_next_tick(module, INT64_C(1234567),
        0, true, 0.0, 0.0, &delay));
    CHECK(delay == 42U);
    CHECK(!infiltratr_temporal_clock_next_tick(module, INT64_C(1234568),
        0, true, 0.0, 0.0, &delay));
    CHECK(delay == 42U);
    infiltratr_temporal_clock_module_free(module);
    infiltratr_temporal_clock_module_free(NULL);
    CHECK(test_system_presentation() == 0);
    return 0;
}
