// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef INFILTRATR_TEMPORAL_RESOURCE_FORMAT_H
#define INFILTRATR_TEMPORAL_RESOURCE_FORMAT_H

/*
 * Private ICU text adapter compiled into each independent resource.
 * The chronology is provider-owned; this header owns only date-part mapping,
 * bounded UTF-8 output copying, and the temporary ICU string's lifetime.
 */
#include <infiltratr/temporal_calendar.h>
#include "icu-calendar.h"

#include <glib.h>
#include <stdbool.h>
#include <string.h>

static inline CalendarDatePart
resource_format_part(InfiltratrTemporalCalendarDatePart part)
{
    switch (part) {
        case INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_DAY:
            return CALENDAR_DATE_PART_DAY;
        case INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_MONTH:
            return CALENDAR_DATE_PART_MONTH;
        case INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_YEAR:
            return CALENDAR_DATE_PART_YEAR;
        case INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_SHORT:
            return CALENDAR_DATE_PART_SHORT;
        case INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_FULL:
            return CALENDAR_DATE_PART_FULL;
        default:
            return CALENDAR_DATE_PART_INVALID;
    }
}

static inline bool resource_format_icu(
    CalendarCalendarMode mode, const char *keyword, int64_t jdn,
    InfiltratrTemporalCalendarDatePart part,
    char *buffer, size_t capacity, size_t *length)
{
    const CalendarDatePart date_part = resource_format_part(part);
    gchar *text;
    size_t size;

    if (buffer == NULL || capacity == 0U || length == NULL ||
        date_part == CALENDAR_DATE_PART_INVALID)
        return false;

    text = calendar_icu_format(mode, keyword, jdn, date_part);
    if (text == NULL || text[0] == '\0') {
        g_free(text);
        return false;
    }
    size = strlen(text);
    if (size >= capacity) {
        g_free(text);
        return false;
    }
    memcpy(buffer, text, size + 1U);
    *length = size;
    g_free(text);
    return true;
}

#endif
