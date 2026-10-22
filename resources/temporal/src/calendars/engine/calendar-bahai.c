// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 1993-2026 Shannon Smith

/*
 * Badíʿ calendar conversion.
 *
 * Years 1-171 B.E. retain the historically used Western civil convention:
 * Naw-Rúz is 21 March. From 172 B.E. (2015) onward this resource implements
 * the unified rule announced by the Universal House of Justice: Tehran is the
 * reference location, the northern vernal equinox is calculated
 * astronomically, and Naw-Rúz is the Tehran civil date whose sunset follows
 * that equinox (or the following date when the equinox is after sunset).
 *
 * Authority for the modern rule is the Universal House of Justice message of
 * 10 July 2014, which designates Tehran as the reference location and the
 * northern vernal equinox as the basis for Naw-Rúz.
 *
 * Numerically, the equinox is the March-equinox polynomial and periodic
 * correction from Jean Meeus, Astronomical Algorithms (2nd ed., ch. 27).
 * TT-to-UTC conversion uses the Espenak/Meeus NASA GSFC ΔT polynomial pieces
 * relevant to this module's modern-year range. The supported astronomical
 * range is 1000-3000 CE; dates outside it use the historical 21-March
 * continuation rather than pretending greater precision.
 *
 * Tehran is represented by fixed reference coordinates and a +03:30 standard
 * civil offset. That fixed offset is an explicit reproducibility policy rather
 * than a claim that historical Iranian timezone/DST law is part of the Badíʿ
 * astronomical rule.
 */

#include "calendar-bahai.h"

#include "julian-day.h"
#include "time-astronomy.h"

#include <math.h>

#define floor_divide calendar_floor_divide
#define gregorian_to_jdn calendar_gregorian_to_jdn
#define jdn_to_gregorian calendar_jdn_to_gregorian

#define JULIAN_DATE_UNIX_EPOCH 2440587.5L
#define SECONDS_PER_DAY 86400.0L
#define MICROSECONDS_PER_DAY ((gint64)86400 * G_USEC_PER_SEC)
#define TEHRAN_LATITUDE 35.6892
#define TEHRAN_LONGITUDE 51.3890
#define TEHRAN_UTC_OFFSET_SECONDS 12600
#define MODERN_BAHAI_FIRST_YEAR ((gint64)172)
#define BAHAI_TO_GREGORIAN_YEAR_OFFSET ((gint64)1843)

typedef struct
{
    long double amplitude;
    long double phase;
    long double rate;
} EquinoxPeriodicTerm;

static const EquinoxPeriodicTerm equinox_terms[] = {
    {485.0L, 324.96L, 1934.136L}, {203.0L, 337.23L, 32964.467L},
    {199.0L, 342.08L, 20.186L}, {182.0L, 27.85L, 445267.112L},
    {156.0L, 73.14L, 45036.886L}, {136.0L, 171.52L, 22518.443L},
    {77.0L, 222.54L, 65928.934L}, {74.0L, 296.72L, 3034.906L},
    {70.0L, 243.58L, 9037.513L}, {58.0L, 119.81L, 33718.147L},
    {52.0L, 297.17L, 150.678L}, {50.0L, 21.02L, 2281.226L},
    {45.0L, 247.54L, 29929.562L}, {44.0L, 325.15L, 31555.956L},
    {29.0L, 60.93L, 4443.417L}, {18.0L, 155.12L, 67555.328L},
    {17.0L, 288.79L, 4562.452L}, {16.0L, 198.04L, 62894.029L},
    {14.0L, 199.76L, 31436.921L}, {12.0L, 95.39L, 14577.848L},
    {12.0L, 287.11L, 31931.756L}, {12.0L, 320.81L, 34777.259L},
    {9.0L, 227.73L, 1222.114L}, {8.0L, 15.45L, 16859.074L}
};

static long double
degrees_to_radians(long double degrees)
{
    return degrees * G_PI / 180.0L;
}

static long double
march_equinox_jde_tt(gint year)
{
    const long double y = ((long double)year - 2000.0L) / 1000.0L;
    const long double jde0 =
        2451623.80984L + 365242.37404L * y + 0.05169L * y * y -
        0.00411L * y * y * y - 0.00057L * y * y * y * y;
    const long double t = (jde0 - 2451545.0L) / 36525.0L;
    const long double w = degrees_to_radians(35999.373L * t - 2.47L);
    const long double delta_lambda =
        1.0L + 0.0334L * cosl(w) + 0.0007L * cosl(2.0L * w);
    long double correction = 0.0L;
    gsize index;

    for (index = 0; index < G_N_ELEMENTS(equinox_terms); index++)
    {
        correction += equinox_terms[index].amplitude *
            cosl(degrees_to_radians(
                equinox_terms[index].phase + equinox_terms[index].rate * t));
    }

    return jde0 + 0.00001L * correction / delta_lambda;
}

static long double
delta_t_seconds(gint year)
{
    if (year <= 2050)
    {
        const long double t = (long double)year - 2000.0L;
        return 62.92L + 0.32217L * t + 0.005589L * t * t;
    }
    if (year <= 2150)
    {
        const long double u = ((long double)year - 1820.0L) / 100.0L;
        return -20.0L + 32.0L * u * u -
               0.5628L * (2150.0L - (long double)year);
    }

    {
        const long double u = ((long double)year - 1820.0L) / 100.0L;
        return -20.0L + 32.0L * u * u;
    }
}

static gboolean
modern_naw_ruz_jdn(gint gregorian_year, gint64 *result)
{
    const long double jde_tt = march_equinox_jde_tt(gregorian_year);
    const long double jd_utc =
        jde_tt - delta_t_seconds(gregorian_year) / SECONDS_PER_DAY;
    const gint64 unix_microseconds = (gint64)llroundl(
        (jd_utc - JULIAN_DATE_UNIX_EPOCH) *
        SECONDS_PER_DAY * G_USEC_PER_SEC);
    const gint64 local_microseconds = calendar_i64_add_saturating(
        unix_microseconds,
        calendar_i64_multiply_saturating(
            TEHRAN_UTC_OFFSET_SECONDS, G_USEC_PER_SEC));
    const gint64 tehran_civil_jdn = calendar_i64_add_saturating(
        floor_divide(local_microseconds, MICROSECONDS_PER_DAY),
        CALENDAR_UNIX_EPOCH_JDN);
    /* Civil and apparent-solar midnight differ. Comparing wrapped solar
     * seconds to a civil date can move an equinox near midnight by a whole
     * day (2060), producing impossible 364/367-day years. Compare UTC instants
     * at the sunset belonging to that Tehran civil date instead. Noon in
     * Tehran also lies on the same UTC date; years 1000..3000 fit gint64. */
    const gint64 tehran_noon_microseconds =
        (tehran_civil_jdn - CALENDAR_UNIX_EPOCH_JDN) * MICROSECONDS_PER_DAY +
        MICROSECONDS_PER_DAY / 2 -
        (gint64)TEHRAN_UTC_OFFSET_SECONDS * G_USEC_PER_SEC;
    gint64 dawn = 0;
    gint64 dusk = 0;

    if (!calendar_solar_boundary_instants(
            tehran_noon_microseconds, TEHRAN_LATITUDE, TEHRAN_LONGITUDE,
            0.833, &dawn, &dusk))
    {
        return FALSE;
    }

    (void)dawn;
    *result = unix_microseconds < dusk
        ? tehran_civil_jdn : tehran_civil_jdn + 1;
    return TRUE;
}

/* Out-of-domain years are not the 1844 epoch. Explicit status is essential:
 * both field conversion and period navigation must fail rather than silently
 * reporting an unrelated, but seemingly valid, historical date. */
static gboolean
bahai_year_start_jdn(gint64 bahai_year, gint64 *result)
{
    if (result == NULL ||
        bahai_year < (gint64)G_MININT - BAHAI_TO_GREGORIAN_YEAR_OFFSET ||
        bahai_year > (gint64)G_MAXINT - BAHAI_TO_GREGORIAN_YEAR_OFFSET)
        return FALSE;

    const gint64 gregorian_year =
        bahai_year + BAHAI_TO_GREGORIAN_YEAR_OFFSET;
    if (bahai_year >= MODERN_BAHAI_FIRST_YEAR &&
        gregorian_year >= 1000 && gregorian_year <= 3000)
        return modern_naw_ruz_jdn((gint)gregorian_year, result);

    *result = gregorian_to_jdn(gregorian_year, 3, 21);
    return TRUE;
}

gint
calendar_bahai_intercalary_days(gint64 bahai_year)
{
    gint64 start, next;
    if (!bahai_year_start_jdn(bahai_year, &start) ||
        !bahai_year_start_jdn(bahai_year + 1, &next))
        return 0;
    const gint64 span = next - start;
    /* Badíʿ years have 365 or 366 days; an unexpected computed year
     * boundary is a calculation failure, not a calendar value. */
    return span == 365 || span == 366 ? (gint)(span - 361) : 0;
}

void
calendar_bahai_fields_from_jdn(gint64 jdn, CalendarCalendarFields *fields)
{
    gint gregorian_year, month, day;
    gint64 bahai_year, start, next;
    gint64 day_index;
    gint intercalary;
    CalendarCalendarFields converted = { 0 };
    g_return_if_fail(fields != NULL);
    *fields = converted;

    /* jdn_to_gregorian narrows the civil year to gint. */
    if (!calendar_gregorian_jdn_representable(jdn)) return;
    jdn_to_gregorian(jdn, &gregorian_year, &month, &day);
    (void)month; (void)day;
    bahai_year = (gint64)gregorian_year - BAHAI_TO_GREGORIAN_YEAR_OFFSET;
    if (!bahai_year_start_jdn(bahai_year, &start)) return;
    if (jdn < start)
    {
        --bahai_year;
        if (!bahai_year_start_jdn(bahai_year, &start)) return;
    }
    if (!bahai_year_start_jdn(bahai_year + 1, &next)) return;
    if (jdn >= next)
    {
        ++bahai_year;
        start = next;
        if (!bahai_year_start_jdn(bahai_year + 1, &next)) return;
    }
    if (jdn < start || jdn >= next) return;
    intercalary = calendar_bahai_intercalary_days(bahai_year);
    if (intercalary != 4 && intercalary != 5) return;

    day_index = jdn - start;
    converted.year = bahai_year;
    if (day_index < 18 * 19)
    {
        converted.month = (gint)(day_index / 19) + 1;
        converted.day = (gint)(day_index % 19) + 1;
    }
    else if (day_index < 18 * 19 + intercalary)
    {
        converted.month = 0;
        converted.day = (gint)(day_index - 18 * 19) + 1;
        converted.special = TRUE;
    }
    else
    {
        converted.month = 19;
        converted.day = (gint)(day_index - 18 * 19 - intercalary) + 1;
    }
    *fields = converted;
}

gint
calendar_bahai_period_index(gint month)
{
    if (month == 0)
        return 18;
    if (month == 19)
        return 19;
    return month - 1;
}

gint
calendar_bahai_month_from_period(gint period)
{
    if (period == 18)
        return 0;
    if (period == 19)
        return 19;
    return period + 1;
}

gint64
calendar_bahai_fields_to_jdn(const CalendarCalendarFields *fields)
{
    gint64 start, candidate;
    gint offset, max_day;
    if (fields == NULL || !bahai_year_start_jdn(fields->year, &start))
        return G_MAXINT64;
    if (fields->month >= 1 && fields->month <= 18)
    {
        offset = (fields->month - 1) * 19;
        max_day = 19;
    }
    else if (fields->month == 0 || fields->month == 19)
    {
        const gint intercalary = calendar_bahai_intercalary_days(fields->year);
        if (intercalary != 4 && intercalary != 5) return G_MAXINT64;
        offset = 18 * 19 + (fields->month == 19 ? intercalary : 0);
        max_day = fields->month == 0 ? intercalary : 19;
    }
    else return G_MAXINT64;

    if (fields->day < 1 || fields->day > max_day) return G_MAXINT64;
    candidate = start + offset + fields->day - 1;
    return calendar_gregorian_jdn_representable(candidate)
        ? candidate : G_MAXINT64;
}
