// SPDX-License-Identifier: GPL-3.0-or-later
#include "infiltratr/temporal_calendar.h"
#include "../src/temporal_module_internal.h"

#include <stdio.h>
#include <string.h>

#ifndef TEMPORAL_CALENDAR_FIXTURE_PATH
#error TEMPORAL_CALENDAR_FIXTURE_PATH must name the calendar provider fixture
#endif

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "calendar provider check failed at line %d\n", __LINE__); \
    return 1; } } while (0)

int main(void)
{
    InfiltratrTemporalCalendarModule *module;
    InfiltratrTemporalCalendarFields fields = { 0 };
    const InfiltratrTemporalResourceMetadata *metadata;
    const InfiltratrTemporalCalendarProvider *provider;
    char text[64] = "keep";
    size_t length = 99U;
    int64_t result = 0;

    CHECK(infiltratr_temporal_calendar_module_open(NULL) == NULL);
    CHECK(infiltratr_temporal_calendar_module_open("") == NULL);
    CHECK(infiltratr_temporal_calendar_module_open("/definitely/missing/calendar.so") == NULL);
    CHECK(infiltratr_temporal_calendar_module_open_id(NULL) == NULL);
    CHECK(infiltratr_temporal_calendar_module_open_id("Bad ID") == NULL);

    module = infiltratr_temporal_calendar_module_open(
        TEMPORAL_CALENDAR_FIXTURE_PATH);
    CHECK(module != NULL);

    metadata = infiltratr_temporal_calendar_module_metadata(module);
    provider = infiltratr_temporal_calendar_module_provider(module);
    CHECK(metadata != NULL && provider != NULL);
    CHECK(strcmp(metadata->id, "fixture-calendar") == 0);
    CHECK(strcmp(provider->calendar_id, "fixture-calendar") == 0);
    /* Direct module loading and catalogue scanning share one ABI validator. */
    CHECK(temporal_calendar_provider_valid(metadata, provider));
    {
        InfiltratrTemporalResourceMetadata wrong_metadata = *metadata;
        InfiltratrTemporalCalendarProvider incomplete = *provider;
        wrong_metadata.id = "not-fixture-calendar";
        CHECK(!temporal_calendar_provider_valid(&wrong_metadata, provider));
        incomplete.add_years = NULL;
        CHECK(!temporal_calendar_provider_valid(metadata, &incomplete));
    }

    CHECK(infiltratr_temporal_calendar_fields_from_jdn(
        module, INT64_C(2461331), &fields));
    CHECK(fields.year == INT64_C(5787));
    CHECK(fields.month == 1 && fields.day == 6);

    CHECK(infiltratr_temporal_calendar_format(
        module, INT64_C(2461331),
        INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_SHORT,
        text, sizeof(text), &length));
    CHECK(strcmp(text, "6 Fixture 5787") == 0);
    CHECK(length == strlen(text));

    strcpy(text, "keep");
    length = 99U;
    CHECK(!infiltratr_temporal_calendar_format(
        module, INT64_C(2461331),
        INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_INVALID,
        text, sizeof(text), &length));
    CHECK(strcmp(text, "keep") == 0 && length == 99U);

    CHECK(infiltratr_temporal_calendar_period_start(
        module, INT64_C(2461331), &result));
    CHECK(result == INT64_C(2461326));
    CHECK(infiltratr_temporal_calendar_add_periods(
        module, INT64_C(2461331), 2, &result));
    CHECK(result == INT64_C(2461391));
    CHECK(infiltratr_temporal_calendar_add_years(
        module, INT64_C(2461331), 1, &result));
    CHECK(result == INT64_C(2461696));

    infiltratr_temporal_calendar_module_free(module);
    infiltratr_temporal_calendar_module_free(NULL);
    /* The normal OS calendar is usable without any resource directory. */
    module = infiltratr_temporal_calendar_module_open_system();
    CHECK(module != NULL);
    CHECK(infiltratr_temporal_calendar_fields_from_jdn(module, 2440588, &fields));
    CHECK(fields.year == 1970 && fields.month == 1 && fields.day == 1);
    CHECK(infiltratr_temporal_calendar_format(module, 2440588,
        INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_SHORT,
        text, sizeof(text), &length));
    CHECK(strcmp(text, "1970-01-01") == 0);
    CHECK(infiltratr_temporal_calendar_add_periods(module, 2440618, 1, &result));
    CHECK(result == 2440646); /* Host month-end clamp: Jan 31 -> Feb 28. */
    CHECK(infiltratr_temporal_calendar_add_years(module, 2458909, 1, &result));
    CHECK(result == 2459274); /* Host leap-day clamp: 2020 -> 2021. */
    infiltratr_temporal_calendar_module_free(module);
    return 0;
}
