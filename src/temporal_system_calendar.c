// SPDX-License-Identifier: GPL-3.0-or-later
/* Normal Gregorian presentation/navigation delegated to the host C runtime.
 * This is a platform adapter, not a copy of any optional chronology engine. */
#define _DEFAULT_SOURCE 1
#define _POSIX_C_SOURCE 200809L
#include "temporal_system_calendar.h"
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static bool system_date(int64_t jdn, struct tm *date)
{
    int64_t days;
    int64_t seconds;
    time_t instant;
    if (jdn < INT64_MIN + INT64_C(2440588)) return false;
    days = jdn - INT64_C(2440588);
    if (days > INT64_MAX / 86400 || days < INT64_MIN / 86400) return false;
    seconds = days * 86400;
    instant = (time_t)seconds;
    if ((int64_t)instant != seconds) return false;
#if defined(_WIN32)
    return gmtime_s(date, &instant) == 0;
#else
    return gmtime_r(&instant, date) != NULL;
#endif
}

static bool system_jdn(struct tm *date, int64_t *jdn)
{
    time_t instant;
    int64_t seconds;
    int64_t days;
    errno = 0;
    date->tm_isdst = 0;
#if defined(_WIN32)
    instant = (time_t)_mkgmtime64(date);
#else
    instant = timegm(date);
#endif
    if (instant == (time_t)-1 && errno != 0) return false;
    seconds = (int64_t)instant;
    days = seconds / 86400;
    if (seconds % 86400 < 0) --days;
    *jdn = days + INT64_C(2440588);
    return true;
}

static bool system_fields(const InfiltratrTemporalCalendarProvider *provider,
                          int64_t jdn, InfiltratrTemporalCalendarFields *fields)
{
    struct tm date;
    int64_t year;
    (void)provider;
    if (fields == NULL || !system_date(jdn, &date)) return false;
    year = (int64_t)date.tm_year + 1900;
    *fields = (InfiltratrTemporalCalendarFields){
        year >= 1 ? year : 1 - year,
        date.tm_mon + 1, date.tm_mday, year >= 1 ? 1 : 0, false
    };
    return true;
}

static bool system_format(const InfiltratrTemporalCalendarProvider *provider,
                          int64_t jdn, InfiltratrTemporalCalendarDatePart part,
                          char *buffer, size_t capacity, size_t *length)
{
    struct tm date;
    char text[INFILTRATR_TEMPORAL_CALENDAR_TEXT_CAPACITY];
    size_t size;
    (void)provider;
    if (buffer == NULL || length == NULL || !system_date(jdn, &date)) return false;
    switch (part) {
    case INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_DAY:
        if (snprintf(text, sizeof(text), "%d", date.tm_mday) < 1) return false;
        size = strlen(text);
        break;
    case INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_MONTH:
        size = strftime(text, sizeof(text), "%B", &date); break;
    case INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_YEAR:
        size = strftime(text, sizeof(text), "%Y", &date); break;
    case INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_SHORT:
        /* Keep the full year while using only the normal host date service. */
        size = strftime(text, sizeof(text), "%Y-%m-%d", &date); break;
    case INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_FULL:
        size = strftime(text, sizeof(text), "%A, %d %B %Y", &date); break;
    default: return false;
    }
    if (size == 0U || size >= capacity) return false;
    memcpy(buffer, text, size + 1U);
    *length = size;
    return true;
}

static bool system_period_start(const InfiltratrTemporalCalendarProvider *provider,
                                int64_t jdn, int64_t *result)
{
    struct tm date;
    (void)provider;
    if (result == NULL || !system_date(jdn, &date)) return false;
    date.tm_mday = 1;
    return system_jdn(&date, result);
}

static bool system_navigate(int64_t jdn, int32_t amount, bool years, int64_t *result)
{
    struct tm date;
    struct tm last;
    int64_t coordinate;
    int day;
    int64_t target;
    if (result == NULL || !system_date(jdn, &date)) return false;
    day = date.tm_mday;
    coordinate = (int64_t)(years ? date.tm_year : date.tm_mon) + amount;
    if (coordinate < INT_MIN || coordinate > INT_MAX) return false;
    if (years) date.tm_year = (int)coordinate;
    else date.tm_mon = (int)coordinate;
    date.tm_mday = 1;
    if (!system_jdn(&date, &target) || !system_date(target, &date)) return false;
    last = date;
    if (last.tm_mon == INT_MAX) return false;
    ++last.tm_mon;
    last.tm_mday = 0;
    if (!system_jdn(&last, &target) || !system_date(target, &last)) return false;
    date.tm_mday = day > last.tm_mday ? last.tm_mday : day;
    return system_jdn(&date, result);
}

static bool system_periods(const InfiltratrTemporalCalendarProvider *provider,
                           int64_t jdn, int32_t amount, int64_t *result)
{
    (void)provider;
    return system_navigate(jdn, amount, false, result);
}

static bool system_years(const InfiltratrTemporalCalendarProvider *provider,
                         int64_t jdn, int32_t amount, int64_t *result)
{
    (void)provider;
    return system_navigate(jdn, amount, true, result);
}

const InfiltratrTemporalCalendarProvider *
infiltratr_temporal_system_calendar_provider(void)
{
    static const InfiltratrTemporalCalendarProvider provider = {
        sizeof(provider), INFILTRATR_TEMPORAL_CALENDAR_PROVIDER_ABI,
        "gregorian", system_fields, system_format, system_period_start,
        system_periods, system_years
    };
    return &provider;
}
