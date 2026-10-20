// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 1993-2026 Shannon Smith

/*
 * Native Hebrew calendar resource.
 *
 * The chronology and its presentation travel as one system resource. The
 * module exports Common's generic temporal metadata plus Common's executable
 * calendar-provider ABI; it has no dependency on Calendar's private registry
 * ABI. Any Common consumer can therefore discover and execute this installed
 * capability from /System/Resources/Calendars.
 */

#include <infiltratr/arithmetic.h>
#include <infiltratr/core.h>
#include <infiltratr/temporal_calendar.h>

#include <glib.h>
#include "resource-format.h"

#include <stdint.h>
#include <string.h>

#if defined(_WIN32)
#define CALENDAR_RESOURCE_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
#define CALENDAR_RESOURCE_EXPORT __attribute__((visibility("default")))
#else
#define CALENDAR_RESOURCE_EXPORT
#endif

#define HEBREW_UNIX_EPOCH_JDN INT64_C(2440588)

enum
{
    HEBREW_HOUR_PARTS = 1080,
    HEBREW_DAY_PARTS = 24 * HEBREW_HOUR_PARTS,
    HEBREW_MONTH_FRACT = 12 * HEBREW_HOUR_PARTS + 793,
    HEBREW_MONTH_PARTS = 29 * HEBREW_DAY_PARTS + HEBREW_MONTH_FRACT,
    HEBREW_BAHARAD = 11 * HEBREW_HOUR_PARTS + 204,
    HEBREW_EPOCH_OFFSET = 347998
};

static const gint hebrew_month_length_table[13][3] = {
    { 30, 30, 30 }, { 29, 29, 30 }, { 29, 30, 30 },
    { 29, 29, 29 }, { 30, 30, 30 }, { 30, 30, 30 },
    { 29, 29, 29 }, { 30, 30, 30 }, { 29, 29, 29 },
    { 30, 30, 30 }, { 29, 29, 29 }, { 30, 30, 30 },
    { 29, 29, 29 }
};

static const gint hebrew_month_start[14][3] = {
    { 0, 0, 0 }, { 30, 30, 30 }, { 59, 59, 60 },
    { 88, 89, 90 }, { 117, 118, 119 }, { 147, 148, 149 },
    { 147, 148, 149 }, { 176, 177, 178 }, { 206, 207, 208 },
    { 235, 236, 237 }, { 265, 266, 267 }, { 294, 295, 296 },
    { 324, 325, 326 }, { 353, 354, 355 }
};

static const gint hebrew_leap_month_start[14][3] = {
    { 0, 0, 0 }, { 30, 30, 30 }, { 59, 59, 60 },
    { 88, 89, 90 }, { 117, 118, 119 }, { 147, 148, 149 },
    { 177, 178, 179 }, { 206, 207, 208 }, { 236, 237, 238 },
    { 265, 266, 267 }, { 295, 296, 297 }, { 324, 325, 326 },
    { 354, 355, 356 }, { 383, 384, 385 }
};


static int64_t
floor_divide(int64_t value,
             int64_t divisor)
{
    int64_t quotient = 0;

    return infiltratr_i64_floor_divmod(value, divisor, &quotient, NULL) ?
        quotient : 0;
}

static int64_t
positive_modulo(int64_t value,
                int64_t modulus)
{
    int64_t remainder = 0;

    return infiltratr_i64_floor_divmod(value, modulus, NULL, &remainder) ?
        remainder : 0;
}

static gboolean
hebrew_is_leap(int64_t year)
{
    return positive_modulo(
        infiltratr_i64_add_saturating(
            infiltratr_i64_multiply_saturating(year, 12), 17),
        19) >= 12;
}

static int64_t
hebrew_start_of_year(int64_t year)
{
    const int64_t months = floor_divide(
        infiltratr_i64_subtract_saturating(
            infiltratr_i64_multiply_saturating(235, year), 234),
        19);
    const int64_t total_fraction = infiltratr_i64_add_saturating(
        infiltratr_i64_multiply_saturating(months, HEBREW_MONTH_FRACT),
        HEBREW_BAHARAD);
    const int64_t fraction_days = floor_divide(
        total_fraction, HEBREW_DAY_PARTS);
    int64_t day = infiltratr_i64_add_saturating(
        infiltratr_i64_multiply_saturating(months, 29),
        fraction_days);
    const int64_t fraction = infiltratr_i64_subtract_saturating(
        total_fraction,
        infiltratr_i64_multiply_saturating(
            fraction_days, HEBREW_DAY_PARTS));
    gint weekday = (gint)positive_modulo(day, 7);

    if (weekday == 2 || weekday == 4 || weekday == 6 ||
        (weekday == 0 &&
         fraction >= 21 * HEBREW_HOUR_PARTS + 589 &&
         hebrew_is_leap(infiltratr_i64_subtract_saturating(year, 1))))
    {
        day = infiltratr_i64_add_saturating(day, 1);
    }
    else if (weekday == 1 &&
             fraction >= 15 * HEBREW_HOUR_PARTS + 204 &&
             !hebrew_is_leap(year))
    {
        day = infiltratr_i64_add_saturating(day, 2);
    }

    return day;
}

static gint
hebrew_year_type(int64_t year)
{
    int64_t length = infiltratr_i64_subtract_saturating(
        hebrew_start_of_year(infiltratr_i64_add_saturating(year, 1)),
        hebrew_start_of_year(year));

    if (length > 380)
        length = infiltratr_i64_subtract_saturating(length, 30);

    if (length == 353)
        return 0;
    if (length == 355)
        return 2;
    return 1;
}

static gint
hebrew_month_length(int64_t year,
                    gint month)
{
    const gint month_index = month - 1;

    if (month < 1 || month > 13)
        return 0;
    if (month_index == 5 && !hebrew_is_leap(year))
        return 0;
    if (month_index == 1 || month_index == 2)
        return hebrew_month_length_table[month_index][hebrew_year_type(year)];
    return hebrew_month_length_table[month_index][0];
}

static gint
hebrew_month_start_offset(int64_t year,
                          gint month)
{
    const gint month_index = month - 1;
    const gint type = hebrew_year_type(year);

    if (month < 1 || month > 13)
        return 0;
    return hebrew_is_leap(year) ?
        hebrew_leap_month_start[month_index][type] :
        hebrew_month_start[month_index][type];
}

static int64_t
hebrew_to_jdn(int64_t year,
              gint month,
              gint day)
{
    int64_t result = infiltratr_i64_add_saturating(
        hebrew_start_of_year(year), HEBREW_EPOCH_OFFSET);

    result = infiltratr_i64_add_saturating(
        result, hebrew_month_start_offset(year, month));
    return infiltratr_i64_add_saturating(result, day - 1);
}

static void
hebrew_from_jdn(int64_t jdn,
                InfiltratrTemporalCalendarFields *fields)
{
    const int64_t d = infiltratr_i64_subtract_saturating(
        jdn, HEBREW_EPOCH_OFFSET - 1);
    const int64_t approximate_months = floor_divide(
        infiltratr_i64_multiply_saturating(d, HEBREW_DAY_PARTS),
        HEBREW_MONTH_PARTS);
    int64_t year = infiltratr_i64_add_saturating(
        floor_divide(
            infiltratr_i64_add_saturating(
                infiltratr_i64_multiply_saturating(19, approximate_months),
                234),
            235),
        1);
    int64_t day_of_year = infiltratr_i64_subtract_saturating(
        d, hebrew_start_of_year(year));
    gint month_index = 0;
    gint type;
    gboolean leap;

    while (day_of_year < 1)
    {
        year = infiltratr_i64_subtract_saturating(year, 1);
        day_of_year = infiltratr_i64_subtract_saturating(
            d, hebrew_start_of_year(year));
    }

    type = hebrew_year_type(year);
    leap = hebrew_is_leap(year);
    while (month_index < 14 &&
           day_of_year >
               (leap ? hebrew_leap_month_start[month_index][type] :
                       hebrew_month_start[month_index][type]))
    {
        month_index++;
    }
    if (month_index > 0)
        month_index--;

    *fields = (InfiltratrTemporalCalendarFields){
        .year = year,
        .month = month_index + 1,
        .day = (int32_t)infiltratr_i64_subtract_saturating(
            day_of_year,
            leap ? hebrew_leap_month_start[month_index][type] :
                   hebrew_month_start[month_index][type]),
        .auxiliary = 0,
        .special = false
    };
}

static void
hebrew_add_months_to_fields(InfiltratrTemporalCalendarFields *fields,
                            int32_t amount)
{
    int64_t year = fields->year;
    gint month = fields->month - 1;
    int32_t remaining = amount;

    if (remaining >= 235 || remaining <= -235)
    {
        const int32_t cycles = remaining / 235;
        year = infiltratr_i64_add_saturating(
            year, infiltratr_i64_multiply_saturating(cycles, 19));
        remaining -= cycles * 235;
    }

    while (remaining > 0)
    {
        month++;
        if (month > 12)
        {
            month = 0;
            year = infiltratr_i64_add_saturating(year, 1);
        }
        if (month == 5 && !hebrew_is_leap(year))
            month++;
        remaining--;
    }

    while (remaining < 0)
    {
        month--;
        if (month < 0)
        {
            year = infiltratr_i64_subtract_saturating(year, 1);
            month = 12;
        }
        if (month == 5 && !hebrew_is_leap(year))
            month--;
        remaining++;
    }

    fields->year = year;
    fields->month = month + 1;
    fields->day = MIN(fields->day, hebrew_month_length(year, fields->month));
}

/*
 * ICU presentation supports only a finite day span, and molad inversions
 * cannot safely approximate arbitrary int64 JDNs. All five ABI operations
 * must honor this domain before running Hebrew year correction arithmetic.
 */
static bool hebrew_fields_checked(int64_t jdn,
                                  InfiltratrTemporalCalendarFields *fields)
{
    if (fields == NULL ||
        jdn < HEBREW_UNIX_EPOCH_JDN - INT64_C(100000000) ||
        jdn > HEBREW_UNIX_EPOCH_JDN + INT64_C(99999999))
        return false;
    hebrew_from_jdn(jdn, fields);
    return fields->month >= 1 && fields->month <= 13 &&
           fields->day >= 1 &&
           fields->day <= hebrew_month_length(fields->year, fields->month) &&
           hebrew_to_jdn(fields->year, fields->month, fields->day) == jdn;
}

static bool
provider_fields(const InfiltratrTemporalCalendarProvider *provider,
                int64_t jdn,
                InfiltratrTemporalCalendarFields *fields)
{
    (void)provider;
    return hebrew_fields_checked(jdn, fields);
}

static bool
provider_format(const InfiltratrTemporalCalendarProvider *provider,
                int64_t jdn,
                InfiltratrTemporalCalendarDatePart part,
                char *buffer, size_t capacity, size_t *length)
{
    InfiltratrTemporalCalendarFields checked;

    (void)provider;
    if (buffer == NULL || capacity == 0U || length == NULL ||
        !hebrew_fields_checked(jdn, &checked))
        return false;

    /* The Hebrew arithmetic stays local; ICU conversion and text ownership
     * use the same private helper as the other independently built modules. */
    return resource_format_icu(
        CALENDAR_CALENDAR_MODE_HEBREW, "hebrew", jdn, part,
        buffer, capacity, length);
}

static bool
provider_period_start(const InfiltratrTemporalCalendarProvider *provider,
                      int64_t jdn,
                      int64_t *result)
{
    InfiltratrTemporalCalendarFields fields;

    (void)provider;
    if (result == NULL || !hebrew_fields_checked(jdn, &fields))
        return false;
    *result = hebrew_to_jdn(fields.year, fields.month, 1);
    return true;
}

static bool
provider_add_periods(const InfiltratrTemporalCalendarProvider *provider,
                     int64_t jdn,
                     int32_t amount,
                     int64_t *result)
{
    InfiltratrTemporalCalendarFields fields;

    (void)provider;
    if (result == NULL || !hebrew_fields_checked(jdn, &fields))
        return false;
    hebrew_add_months_to_fields(&fields, amount);
    const int64_t candidate =
        hebrew_to_jdn(fields.year, fields.month, fields.day);
    if (candidate < HEBREW_UNIX_EPOCH_JDN - INT64_C(100000000) ||
        candidate > HEBREW_UNIX_EPOCH_JDN + INT64_C(100000000))
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

    (void)provider;
    if (result == NULL || !hebrew_fields_checked(jdn, &fields))
        return false;
    fields.year = infiltratr_i64_add_saturating(fields.year, amount);
    if (fields.month == 6 && !hebrew_is_leap(fields.year))
        fields.month = 7;
    fields.day = MIN(fields.day,
                     hebrew_month_length(fields.year, fields.month));
    const int64_t candidate =
        hebrew_to_jdn(fields.year, fields.month, fields.day);
    if (candidate < HEBREW_UNIX_EPOCH_JDN - INT64_C(100000000) ||
        candidate > HEBREW_UNIX_EPOCH_JDN + INT64_C(100000000))
        return false;
    *result = candidate;
    return true;
}

static const InfiltratrTemporalResourceMetadata hebrew_metadata = {
    .struct_size = sizeof(InfiltratrTemporalResourceMetadata),
    .abi_version = INFILTRATR_TEMPORAL_RESOURCE_METADATA_ABI,
    .kind = INFILTRATR_TEMPORAL_RESOURCE_CALENDAR,
    .id = "hebrew",
    .name = INFILTRATR_TEMPORAL_DISPLAY_NAME,
    .supports_seconds = false,
    .requires_latitude = false,
    .requires_longitude = false
};

static const InfiltratrTemporalCalendarProvider hebrew_provider = {
    .struct_size = sizeof(InfiltratrTemporalCalendarProvider),
    .abi_version = INFILTRATR_TEMPORAL_CALENDAR_PROVIDER_ABI,
    .calendar_id = "hebrew",
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
    return &hebrew_metadata;
}

CALENDAR_RESOURCE_EXPORT const InfiltratrTemporalCalendarProvider *
infiltratr_temporal_calendar_provider_v1(void)
{
    return &hebrew_provider;
}
