// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 1993-2026 Shannon Smith

#ifndef CALENDAR_CALENDAR_HELPERS_H
#define CALENDAR_CALENDAR_HELPERS_H

#include "calendar-engine-types.h"
#include "julian-day.h"

/*
 * Arithmetic inverse-year estimators are only meaningful across the supported
 * finite civil-date domain. This 200-million-day span includes every historic
 * implementation period without approaching int64 saturation in year/epoch
 * products. ICU's signed millisecond horizon is the corresponding bound.
 */
static inline gboolean calendar_corrected_year_jdn_supported(gint64 jdn)
{
    return jdn >= CALENDAR_UNIX_EPOCH_JDN - G_GINT64_CONSTANT(100000000) &&
           jdn <= CALENDAR_UNIX_EPOCH_JDN + G_GINT64_CONSTANT(100000000);
}

G_BEGIN_DECLS

gint64 calendar_count_multiples_inclusive(gint64 first,
                                               gint64 last,
                                               gint64 divisor);
gchar *calendar_format_named_date(
    const CalendarCalendarFields *fields,
    CalendarDatePart part,
    const gchar *const *months,
    gsize month_count,
    const gchar *year_suffix);

G_END_DECLS

#endif
