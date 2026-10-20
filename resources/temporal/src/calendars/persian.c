// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 1993-2026 Shannon Smith

/* Native Persian (Solar Hijri arithmetic) calendar resource. */

#include <infiltratr/arithmetic.h>
#include <infiltratr/core.h>
#include <infiltratr/temporal_calendar.h>

#include <glib.h>
#include <stdint.h>
#include <string.h>

#include "calendar-arithmetic-internal.h"
#include "calendar-types.h"
#include "icu-calendar.h"
#include "resource-format.h"
#include "integer-math.h"

#if defined(_WIN32)
#define CALENDAR_RESOURCE_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
#define CALENDAR_RESOURCE_EXPORT __attribute__((visibility("default")))
#else
#define CALENDAR_RESOURCE_EXPORT
#endif

static bool
provider_fields(const InfiltratrTemporalCalendarProvider *provider,
                int64_t jdn,
                InfiltratrTemporalCalendarFields *fields)
{
    CalendarCalendarFields native_fields;

    (void)provider;
    if (fields == NULL)
        return false;

    if (!calendar_arithmetic_persian_fields_from_jdn(jdn, &native_fields) ||
        calendar_arithmetic_persian_fields_to_jdn(
            native_fields.year, native_fields.month, native_fields.day) != jdn)
        return false;

    *fields = (InfiltratrTemporalCalendarFields){
        .year = native_fields.year,
        .month = native_fields.month,
        .day = native_fields.day,
        .auxiliary = native_fields.auxiliary,
        .special = native_fields.special
    };
    return true;
}

static int64_t
fields_to_jdn(const InfiltratrTemporalCalendarFields *fields)
{
    return calendar_arithmetic_persian_fields_to_jdn(
        fields->year, fields->month, fields->day);
}

static bool
provider_format(const InfiltratrTemporalCalendarProvider *provider,
                int64_t jdn,
                InfiltratrTemporalCalendarDatePart part,
                char *buffer,
                size_t capacity,
                size_t *length)
{
    InfiltratrTemporalCalendarFields checked;
    if (!provider_fields(provider, jdn, &checked))
        return false;
    return resource_format_icu(
        CALENDAR_CALENDAR_MODE_PERSIAN, "persian", jdn, part, buffer, capacity, length);
}

static bool
provider_period_start(const InfiltratrTemporalCalendarProvider *provider,
                      int64_t jdn,
                      int64_t *result)
{
    InfiltratrTemporalCalendarFields fields;

    if (result == NULL || !provider_fields(provider, jdn, &fields))
        return false;
    fields.day = 1;
    const int64_t candidate = fields_to_jdn(&fields);
    InfiltratrTemporalCalendarFields checked;
    if (!provider_fields(provider, candidate, &checked))
        return false;
    *result = candidate;
    return true;
}

static bool
provider_add_periods(const InfiltratrTemporalCalendarProvider *provider,
                     int64_t jdn,
                     int32_t amount,
                     int64_t *result)
{
    InfiltratrTemporalCalendarFields fields;
    int64_t serial;
    int64_t year;

    if (result == NULL || !provider_fields(provider, jdn, &fields))
        return false;

    serial = infiltratr_i64_add_saturating(
        infiltratr_i64_add_saturating(
            infiltratr_i64_multiply_saturating(fields.year, 12),
            fields.month - 1),
        amount);
    year = calendar_floor_divide(serial, 12);
    fields.year = year;
    fields.month = (int32_t)calendar_positive_modulo(serial, 12) + 1;
    fields.day = MIN(
        fields.day,
        calendar_arithmetic_persian_month_length(year, fields.month));
    const int64_t candidate = fields_to_jdn(&fields);
    InfiltratrTemporalCalendarFields checked;
    if (!provider_fields(provider, candidate, &checked))
        return false;
    *result = candidate;
    return true;
}

static bool
provider_add_years(const InfiltratrTemporalCalendarProvider *provider,
                   int64_t jdn,
                   int32_t amount,
                   int64_t *result)
{
    InfiltratrTemporalCalendarFields fields;

    if (result == NULL || !provider_fields(provider, jdn, &fields))
        return false;

    fields.year = infiltratr_i64_add_saturating(fields.year, amount);
    fields.day = MIN(
        fields.day,
        calendar_arithmetic_persian_month_length(fields.year, fields.month));
    const int64_t candidate = fields_to_jdn(&fields);
    InfiltratrTemporalCalendarFields checked;
    if (!provider_fields(provider, candidate, &checked))
        return false;
    *result = candidate;
    return true;
}

static const InfiltratrTemporalResourceMetadata persian_metadata = {
    .struct_size = sizeof(InfiltratrTemporalResourceMetadata),
    .abi_version = INFILTRATR_TEMPORAL_RESOURCE_METADATA_ABI,
    .kind = INFILTRATR_TEMPORAL_RESOURCE_CALENDAR,
    .id = "persian",
    .name = INFILTRATR_TEMPORAL_DISPLAY_NAME,
    .supports_seconds = false,
    .requires_latitude = false,
    .requires_longitude = false
};

static const InfiltratrTemporalCalendarProvider persian_provider = {
    .struct_size = sizeof(InfiltratrTemporalCalendarProvider),
    .abi_version = INFILTRATR_TEMPORAL_CALENDAR_PROVIDER_ABI,
    .calendar_id = "persian",
    .fields_from_jdn = provider_fields,
    .format = provider_format,
    .period_start = provider_period_start,
    .add_periods = provider_add_periods,
    .add_years = provider_add_years
};

CALENDAR_RESOURCE_EXPORT const InfiltratrTemporalResourceMetadata *
infiltratr_temporal_resource_metadata_v1(void);
CALENDAR_RESOURCE_EXPORT const InfiltratrTemporalCalendarProvider *
infiltratr_temporal_calendar_provider_v1(void);

CALENDAR_RESOURCE_EXPORT const InfiltratrTemporalResourceMetadata *
infiltratr_temporal_resource_metadata_v1(void)
{
    return &persian_metadata;
}

CALENDAR_RESOURCE_EXPORT const InfiltratrTemporalCalendarProvider *
infiltratr_temporal_calendar_provider_v1(void)
{
    return &persian_provider;
}
