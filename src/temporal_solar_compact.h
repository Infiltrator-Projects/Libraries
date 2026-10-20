// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef INFILTRATR_TEMPORAL_SOLAR_COMPACT_H
#define INFILTRATR_TEMPORAL_SOLAR_COMPACT_H
/*
 * Private NOAA/GML compact solar model. Independently linked clock resources
 * compile these same equations without linking to another provider.
 *
 * Input: signed Unix microseconds. Outputs: equation of time in minutes,
 * solar declination in radians. This model is not an observational ephemeris.
 * Callers own historical interpretation and sunrise/sunset conventions.
 */
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

static inline int64_t temporal_solar_floor_div(int64_t n, int64_t d)
{
    const int64_t quotient = n / d;
    return n % d < 0 ? quotient - 1 : quotient;
}

static inline int64_t temporal_solar_mod(int64_t n, int64_t d)
{
    const int64_t remainder = n % d;
    return remainder < 0 ? remainder + d : remainder;
}

/* Gregorian 400-year arithmetic without time_t, valid for all int64_t
 * microsecond instants. A negative date retains a positive UTC-day phase. */
static inline bool temporal_solar_day_of_year(int64_t instant, int *ordinal)
{
    static const int cumulative_days[12] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };
    const int64_t days =
        temporal_solar_floor_div(instant, INT64_C(86400000000));
    const int64_t z = days + INT64_C(719468);
    const int64_t era = temporal_solar_floor_div(z, INT64_C(146097));
    const int64_t doe = z - era * INT64_C(146097);
    const int64_t yoe = (doe - doe / 1460 + doe / 36524 -
                         doe / 146096) / 365;
    const int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const int64_t mp = (5 * doy + 2) / 153;
    const int day = (int)(doy - (153 * mp + 2) / 5 + 1);
    const int month = (int)(mp + (mp < 10 ? 3 : -9));
    const int64_t year = yoe + era * 400 + (month <= 2);
    const bool leap =
        (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    if (ordinal == NULL || month < 1 || month > 12 ||
        day < 1 || day > 31) return false;
    *ordinal = cumulative_days[month - 1] + day +
               (leap && month > 2 ? 1 : 0);
    return true;
}

static inline bool temporal_solar_compact_terms(int64_t instant,
                                                double *equation_minutes,
                                                double *declination_radians)
{
    const long double pi = 3.141592653589793238462643383279502884L;
    const long double utc_hour =
        (long double)temporal_solar_mod(instant, INT64_C(86400000000)) /
        3600000000.0L;
    int ordinal;
    long double gamma;

    if (!temporal_solar_day_of_year(instant, &ordinal)) return false;
    gamma = 2.0L * pi / 365.0L *
            ((long double)ordinal - 1.0L +
             (utc_hour - 12.0L) / 24.0L);
    if (equation_minutes != NULL)
        *equation_minutes = 229.18 *
            (0.000075 + 0.001868 * cos((double)gamma) -
             0.032077 * sin((double)gamma) -
             0.014615 * cos((double)(2.0L * gamma)) -
             0.040849 * sin((double)(2.0L * gamma)));
    if (declination_radians != NULL)
        *declination_radians =
            0.006918 - 0.399912 * cos((double)gamma) +
            0.070257 * sin((double)gamma) -
            0.006758 * cos((double)(2.0L * gamma)) +
            0.000907 * sin((double)(2.0L * gamma)) -
            0.002697 * cos((double)(3.0L * gamma)) +
            0.001480 * sin((double)(3.0L * gamma));
    return true;
}
#endif
