// SPDX-License-Identifier: GPL-3.0-or-later
/* Consumer ABI invariants only; provider repositories own model accuracy. */
#include <infiltratr/temporal_calendar.h>
#include <infiltratr/temporal_clock_resource.h>
#include <infiltratr/temporal_runtime.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(bool condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "Published provider ABI failure: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

static void identity(const InfiltratrTemporalResourceMetadata *metadata,
                     const char *id, const char *name,
                     InfiltratrTemporalResourceKind kind)
{
    require(metadata != NULL, "missing module metadata");
    require(metadata->abi_version == INFILTRATR_TEMPORAL_RESOURCE_METADATA_ABI,
            "metadata ABI version");
    require(metadata->kind == kind, "metadata kind");
    require(strcmp(metadata->id, id) == 0, "metadata ID");
    require(strcmp(metadata->name, name) == 0, "canonical resource name");
}

static void calendar_probe(const char *id, const char *path,
                           const char *name, bool installed)
{
    InfiltratrTemporalCalendarModule *module =
        infiltratr_temporal_calendar_module_open(path);
    InfiltratrTemporalCalendarFields fields;
    const int64_t day = INT64_C(2451545);
    int64_t result = 0;
    require(module != NULL, path);
    identity(infiltratr_temporal_calendar_module_metadata(module), id, name,
             INFILTRATR_TEMPORAL_RESOURCE_CALENDAR);
    require(infiltratr_temporal_calendar_fields_from_jdn(module, day, &fields), "calendar fields");
    require(infiltratr_temporal_calendar_period_start(module, day, &result) && result <= day, "period start");
    require(infiltratr_temporal_calendar_add_periods(module, day, 0, &result) && result == day, "zero-period identity");
    require(infiltratr_temporal_calendar_add_years(module, day, 0, &result) && result == day, "zero-year identity");
    for (int part = INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_DAY;
         part <= INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_FULL; ++part) {
        char text[1024] = {0};
        size_t length = 0U;
        require(infiltratr_temporal_calendar_format(module, day,
            (InfiltratrTemporalCalendarDatePart)part, text, sizeof(text), &length), "calendar formatting");
        require(length > 0U && length == strlen(text), "calendar formatted length");
    }
    infiltratr_temporal_calendar_module_free(module);
    if (installed) {
        module = infiltratr_temporal_calendar_module_open_id(id);
        require(module != NULL, "installed calendar lookup");
        require(infiltratr_temporal_installed_calendar_find(id) != NULL, "installed calendar catalogue");
        infiltratr_temporal_calendar_module_free(module);
    }
}

static void clock_probe(const char *id, const char *path,
                        const char *name, bool installed)
{
    InfiltratrTemporalClockModule *module =
        infiltratr_temporal_clock_module_open(path);
    require(module != NULL, path);
    const InfiltratrTemporalResourceMetadata *metadata =
        infiltratr_temporal_clock_module_metadata(module);
    identity(metadata, id, name, INFILTRATR_TEMPORAL_RESOURCE_CLOCK);
    for (int seconds = 0; seconds <= (metadata->supports_seconds ? 1 : 0); ++seconds) {
        char text[1024] = {0};
        size_t length = 0U;
        uint32_t delay = 0U;
        require(infiltratr_temporal_clock_format(module, "en_US", INT64_C(1704067200000000),
            39600, seconds != 0, false, -37.8, 144.9, text, sizeof(text), &length), "clock formatting");
        require(length > 0U && length == strlen(text), "clock formatted length");
        require(infiltratr_temporal_clock_next_tick(module, INT64_C(1704067200000000),
            39600, seconds != 0, -37.8, 144.9, &delay) && delay > 0U, "clock scheduling");
    }
    infiltratr_temporal_clock_module_free(module);
    if (installed) {
        module = infiltratr_temporal_clock_module_open_id(id);
        require(module != NULL, "installed clock lookup");
        require(infiltratr_temporal_installed_clock_mode_find(id) != NULL, "installed clock catalogue");
        infiltratr_temporal_clock_module_free(module);
    }
}

int main(int argc, char **argv)
{
    unsigned calendars = 0U, clocks = 0U;
    require(argc >= 6 && (argc - 2) % 4 == 0, "expected mode and provider records");
    const bool installed = strcmp(argv[1], "--installed") == 0;
    require(installed || strcmp(argv[1], "--direct") == 0, "invalid probe mode");
    for (int index = 2; index < argc; index += 4) {
        if (strcmp(argv[index], "calendar") == 0) {
            calendar_probe(argv[index + 1], argv[index + 2], argv[index + 3], installed);
            ++calendars;
        } else {
            require(strcmp(argv[index], "clock") == 0, "invalid provider kind");
            clock_probe(argv[index + 1], argv[index + 2], argv[index + 3], installed);
            ++clocks;
        }
    }
    require(calendars == 31U && clocks == 23U, "expected complete independent provider set");
    puts("Common consumer ABI: all 31 calendar and 23 clock providers passed");
    return EXIT_SUCCESS;
}
