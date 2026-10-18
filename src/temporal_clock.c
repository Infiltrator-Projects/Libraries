// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 1993-2026 Shannon Smith
 *
 * Independent elapsed-duration projections and legacy ABI stubs.
 *
 * All twenty-three clocks are rendered solely by installed native .so
 * modules through Common's validated resource ABI. Their old clock-face
 * implementations and language tables do not belong here. Duration
 * representation is distinct from formatting a canonical instant.
 */
#include "infiltratr/temporal.h"

#include "infiltratr/arithmetic.h"
#include "infiltratr/compiler.h"
#include "infiltratr/core.h"
#include "infiltratr/i18n.h"
#include "infiltratr/timing.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define MICROSECONDS_PER_SECOND INT64_C(1000000)
#define SECONDS_PER_MINUTE INT64_C(60)
#define MINUTES_PER_HOUR INT64_C(60)
#define SECONDS_PER_HOUR INT64_C(3600)
#define SECONDS_PER_DAY INT64_C(86400)
#define MICROSECONDS_PER_DAY (SECONDS_PER_DAY * MICROSECONDS_PER_SECOND)
#define INTERNET_TICKS_PER_DAY UINT64_C(1000)
#define HEX_TICKS_PER_DAY UINT64_C(65536)
#define DECIMAL_SECONDS_PER_DAY UINT64_C(100000)
#define CHINESE_DOUBLE_HOURS_PER_DAY UINT64_C(12)
#define CHINESE_KE_PER_DAY UINT64_C(100)
#define VIGHATI_PER_DAY UINT64_C(3600)
#define SIDEREAL_RATE_PER_SI_SECOND (24.06570982441908L / 24.0L)
#define INFILTRATR_PI 3.141592653589793238462643383279502884L

static bool write_text(char *buffer, size_t capacity, size_t *length,
                       const char *text)
{
    size_t needed;

    if (buffer == NULL || capacity == 0U || text == NULL) {
        return false;
    }
    needed = strlen(text);
    if (needed >= capacity) {
        return false;
    }
    memcpy(buffer, text, needed + 1U);
    if (length != NULL) {
        *length = needed;
    }
    return true;
}

static bool write_printf(char *buffer, size_t capacity, size_t *length,
                         const char *format, ...)
    INFILTRATR_PRINTF_FORMAT(4, 5);
static bool write_printf(char *buffer, size_t capacity, size_t *length,
                         const char *format, ...)
{
    va_list arguments;
    int written;

    if (buffer == NULL || capacity == 0U || format == NULL) {
        return false;
    }
    va_start(arguments, format);
    written = vsnprintf(buffer, capacity, format, arguments);
    va_end(arguments);
    if (written < 0 || (size_t)written >= capacity) {
        buffer[0] = '\0';
        return false;
    }
    if (length != NULL) {
        *length = (size_t)written;
    }
    return true;
}

static int64_t floor_divide(int64_t value, int64_t divisor)
{
    int64_t quotient = 0;

    if (!infiltratr_i64_floor_divmod(value, divisor, &quotient, NULL)) {
        return 0;
    }
    return quotient;
}

static int64_t positive_modulo(int64_t value, int64_t modulus)
{
    int64_t remainder = 0;

    if (!infiltratr_i64_floor_divmod(value, modulus, NULL, &remainder)) {
        return 0;
    }
    return remainder;
}

/*
 * Convert a Unix day number to Gregorian day-of-year with exact 400-year
 * cycle arithmetic. Keeping this independent of time_t makes astronomical
 * formatting deterministic on 32-bit and 64-bit hosts alike.
 */
static bool gregorian_day_of_year(int64_t unix_microseconds,
                                  int *day_of_year,
                                  bool *is_leap_year)
{
    static const int cumulative_days[12] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };
    int64_t unix_seconds;
    int64_t days;
    int64_t z;
    int64_t era;
    int64_t doe;
    int64_t yoe;
    int64_t doy_march;
    int64_t mp;
    int64_t year;
    int month;
    int day;
    bool leap;

    if (day_of_year == NULL) {
        return false;
    }
    unix_seconds = floor_divide(unix_microseconds, MICROSECONDS_PER_SECOND);
    days = floor_divide(unix_seconds, SECONDS_PER_DAY);
    if (!infiltratr_i64_add_checked(days, INT64_C(719468), &z)) {
        return false;
    }

    era = floor_divide(z, INT64_C(146097));
    doe = z - era * INT64_C(146097);
    yoe = (doe - doe / INT64_C(1460) + doe / INT64_C(36524) -
           doe / INT64_C(146096)) / INT64_C(365);
    year = yoe + era * INT64_C(400);
    doy_march = doe - (INT64_C(365) * yoe + yoe / INT64_C(4) -
                       yoe / INT64_C(100));
    mp = (INT64_C(5) * doy_march + INT64_C(2)) / INT64_C(153);
    day = (int)(doy_march - (INT64_C(153) * mp + INT64_C(2)) /
                INT64_C(5) + INT64_C(1));
    month = (int)(mp + (mp < 10 ? 3 : -9));
    if (month <= 2) {
        year++;
    }

    leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    if (month < 1 || month > 12 || day < 1 || day > 31) {
        return false;
    }
    *day_of_year = cumulative_days[month - 1] + day +
                   (leap && month > 2 ? 1 : 0);
    if (is_leap_year != NULL) {
        *is_leap_year = leap;
    }
    return true;
}

/*
 * Compact equation-of-time and solar-declination approximation shared by the
 * apparent-solar and seasonal clock families. Both values come from the same
 * fractional-year phase so all location-aware modes remain internally
 * consistent.
 */
static bool solar_terms(int64_t unix_microseconds,
                        double *equation_minutes,
                        double *declination_radians)
{
    const int64_t utc_phase =
        positive_modulo(unix_microseconds, MICROSECONDS_PER_DAY);
    const long double fractional_hour =
        (long double)utc_phase /
        ((long double)SECONDS_PER_HOUR * MICROSECONDS_PER_SECOND);
    int day_of_year;
    long double gamma;

    if (!gregorian_day_of_year(unix_microseconds, &day_of_year, NULL)) {
        return false;
    }
    gamma = 2.0L * INFILTRATR_PI / 365.0L *
            ((long double)day_of_year - 1.0L +
             (fractional_hour - 12.0L) / 24.0L);

    if (equation_minutes != NULL) {
        *equation_minutes = 229.18 *
            (0.000075 + 0.001868 * cos((double)gamma) -
             0.032077 * sin((double)gamma) -
             0.014615 * cos((double)(2.0L * gamma)) -
             0.040849 * sin((double)(2.0L * gamma)));
    }
    if (declination_radians != NULL) {
        *declination_radians =
            0.006918 - 0.399912 * cos((double)gamma) +
            0.070257 * sin((double)gamma) -
            0.006758 * cos((double)(2.0L * gamma)) +
            0.000907 * sin((double)(2.0L * gamma)) -
            0.002697 * cos((double)(3.0L * gamma)) +
            0.001480 * sin((double)(3.0L * gamma));
    }
    return true;
}

static double clamp_longitude(double longitude)
{
    return isfinite(longitude)
        ? infiltratr_clamp_double(longitude, -180.0, 180.0) : 0.0;
}

static double clamp_latitude(double latitude)
{
    return isfinite(latitude)
        ? infiltratr_clamp_double(latitude, -90.0, 90.0) : 0.0;
}

static bool solar_boundary_instants(int64_t unix_microseconds,
                                    double latitude,
                                    double longitude,
                                    double solar_depression_degrees,
                                    int64_t *dawn_microseconds,
                                    int64_t *dusk_microseconds)
{
    const int64_t safety_margin = INT64_C(2) * MICROSECONDS_PER_DAY;
    const double safe_latitude = clamp_latitude(latitude);
    const double safe_longitude = clamp_longitude(longitude);
    int64_t day_index;
    int64_t day_start;
    int64_t noon;
    double equation_minutes = 0.0;
    double declination = 0.0;
    double latitude_radians;
    double zenith_radians;
    double cosine_hour_angle;
    long double hour_angle;
    long double solar_noon_minutes;
    long double hour_angle_minutes;
    long double dawn_value;
    long double dusk_value;

    if (dawn_microseconds == NULL || dusk_microseconds == NULL ||
        unix_microseconds < INT64_MIN + safety_margin ||
        unix_microseconds > INT64_MAX - safety_margin ||
        !isfinite(solar_depression_degrees) ||
        solar_depression_degrees < 0.0 ||
        solar_depression_degrees >= 90.0) {
        return false;
    }

    day_index = floor_divide(unix_microseconds, MICROSECONDS_PER_DAY);
    if (!infiltratr_i64_multiply_checked(
            day_index, MICROSECONDS_PER_DAY, &day_start) ||
        !infiltratr_i64_add_checked(
            day_start, MICROSECONDS_PER_DAY / 2, &noon) ||
        !solar_terms(noon, &equation_minutes, &declination)) {
        return false;
    }

    latitude_radians = safe_latitude * (double)INFILTRATR_PI / 180.0;
    zenith_radians =
        (90.0 + solar_depression_degrees) * (double)INFILTRATR_PI / 180.0;
    if (fabs(cos(latitude_radians) * cos(declination)) < 1.0e-12) {
        return false;
    }

    cosine_hour_angle =
        cos(zenith_radians) /
            (cos(latitude_radians) * cos(declination)) -
        tan(latitude_radians) * tan(declination);
    if (cosine_hour_angle < -1.0 || cosine_hour_angle > 1.0) {
        return false;
    }

    hour_angle = acosl((long double)infiltratr_clamp_double(
        cosine_hour_angle, -1.0, 1.0));
    solar_noon_minutes =
        720.0L - 4.0L * safe_longitude - equation_minutes;
    hour_angle_minutes =
        hour_angle * 180.0L / INFILTRATR_PI * 4.0L;
    dawn_value =
        (long double)day_start +
        (solar_noon_minutes - hour_angle_minutes) *
            60.0L * MICROSECONDS_PER_SECOND;
    dusk_value =
        (long double)day_start +
        (solar_noon_minutes + hour_angle_minutes) *
            60.0L * MICROSECONDS_PER_SECOND;

    if (dawn_value < (long double)INT64_MIN ||
        dawn_value > (long double)INT64_MAX ||
        dusk_value < (long double)INT64_MIN ||
        dusk_value > (long double)INT64_MAX) {
        return false;
    }
    *dawn_microseconds = (int64_t)llroundl(dawn_value);
    *dusk_microseconds = (int64_t)llroundl(dusk_value);
    return true;
}

typedef struct SeasonalProgress {
    long double daylight_units;
    long double night_units;
} SeasonalProgress;

static bool solar_boundaries_for_day_index(
    int64_t day_index,
    double latitude,
    double longitude,
    double solar_depression_degrees,
    int64_t *dawn_microseconds,
    int64_t *dusk_microseconds)
{
    int64_t day_start;
    int64_t noon;

    if (!infiltratr_i64_multiply_checked(
            day_index, MICROSECONDS_PER_DAY, &day_start) ||
        !infiltratr_i64_add_checked(
            day_start, MICROSECONDS_PER_DAY / 2, &noon)) {
        return false;
    }
    return solar_boundary_instants(
        noon, latitude, longitude, solar_depression_degrees,
        dawn_microseconds, dusk_microseconds);
}

/*
 * Map a civil instant onto two monotonic seasonal-unit coordinates. Every
 * complete dawn-to-dawn cycle contributes exactly daylight_parts daylight
 * units and night_parts night units, while the current partial unit is scaled
 * by that day's real astronomical dawn/dusk span. Taking the difference of two
 * coordinates therefore integrates an elapsed interval without pretending an
 * unequal Roman hora, Roman vigilia or Edo toki has a fixed SI duration.
 */
static bool seasonal_progress_at(int64_t unix_microseconds,
                                 double latitude,
                                 double longitude,
                                 double solar_depression_degrees,
                                 unsigned daylight_parts,
                                 unsigned night_parts,
                                 SeasonalProgress *progress)
{
    const int64_t approximate_day =
        floor_divide(unix_microseconds, MICROSECONDS_PER_DAY);
    int64_t best_dawn = INT64_MIN;
    int64_t best_dusk = INT64_MIN;
    int64_t best_day = 0;
    int offset;

    if (progress == NULL || daylight_parts == 0U || night_parts == 0U) {
        return false;
    }

    for (offset = -2; offset <= 2; ++offset) {
        int64_t candidate_day;
        int64_t dawn;
        int64_t dusk;

        if (!infiltratr_i64_add_checked(
                approximate_day, (int64_t)offset, &candidate_day) ||
            !solar_boundaries_for_day_index(
                candidate_day, latitude, longitude,
                solar_depression_degrees, &dawn, &dusk)) {
            continue;
        }
        if (dawn <= unix_microseconds && dawn > best_dawn) {
            best_dawn = dawn;
            best_dusk = dusk;
            best_day = candidate_day;
        }
    }

    if (best_dawn == INT64_MIN || best_dusk <= best_dawn) {
        return false;
    }

    if (unix_microseconds < best_dusk) {
        const long double fraction =
            (long double)(unix_microseconds - best_dawn) /
            (long double)(best_dusk - best_dawn);
        progress->daylight_units =
            (long double)best_day * daylight_parts +
            fraction * daylight_parts;
        progress->night_units =
            (long double)best_day * night_parts;
        return true;
    }

    {
        int64_t next_day;
        int64_t next_dawn;
        int64_t next_dusk;
        long double fraction;

        if (!infiltratr_i64_add_checked(best_day, INT64_C(1), &next_day) ||
            !solar_boundaries_for_day_index(
                next_day, latitude, longitude,
                solar_depression_degrees, &next_dawn, &next_dusk) ||
            next_dawn <= best_dusk) {
            return false;
        }
        (void)next_dusk;
        fraction =
            (long double)(unix_microseconds - best_dusk) /
            (long double)(next_dawn - best_dusk);
        progress->daylight_units =
            (long double)best_day * daylight_parts + daylight_parts;
        progress->night_units =
            (long double)best_day * night_parts +
            fraction * night_parts;
        return true;
    }
}

static bool seasonal_interval_progress(
    uint64_t elapsed_microseconds,
    int64_t end_unix_microseconds,
    double latitude,
    double longitude,
    double solar_depression_degrees,
    unsigned daylight_parts,
    unsigned night_parts,
    long double *daylight_units,
    long double *night_units)
{
    SeasonalProgress start;
    SeasonalProgress end;
    int64_t start_unix_microseconds;

    if (daylight_units == NULL || night_units == NULL ||
        end_unix_microseconds == INT64_MIN ||
        elapsed_microseconds > (uint64_t)INT64_MAX ||
        !infiltratr_i64_add_checked(
            end_unix_microseconds, -(int64_t)elapsed_microseconds,
            &start_unix_microseconds) ||
        !seasonal_progress_at(
            start_unix_microseconds, latitude, longitude,
            solar_depression_degrees, daylight_parts, night_parts, &start) ||
        !seasonal_progress_at(
            end_unix_microseconds, latitude, longitude,
            solar_depression_degrees, daylight_parts, night_parts, &end)) {
        return false;
    }

    *daylight_units = end.daylight_units - start.daylight_units;
    *night_units = end.night_units - start.night_units;
    if (*daylight_units < -1.0e-9L || *night_units < -1.0e-9L) {
        return false;
    }
    if (*daylight_units < 0.0L) *daylight_units = 0.0L;
    if (*night_units < 0.0L) *night_units = 0.0L;
    return true;
}

static bool seasonal_quantise(long double units,
                              uint64_t subdivisions,
                              bool include_fraction,
                              uint64_t *ticks)
{
    long double scaled;

    if (ticks == NULL || subdivisions == 0U ||
        !isfinite((double)units) || units < 0.0L) {
        return false;
    }
    scaled = include_fraction
        ? units * (long double)subdivisions
        : floorl(units) * (long double)subdivisions;
    if (scaled < 0.0L || scaled > (long double)UINT64_MAX) {
        return false;
    }
    *ticks = include_fraction
        ? (uint64_t)llroundl(scaled)
        : (uint64_t)scaled;
    return true;
}

static bool duration_partition(uint64_t phase_microseconds,
                               uint64_t units_per_day,
                               uint64_t *tick);

static bool format_duration_chinese_partition(
    uint64_t elapsed_microseconds,
    uint64_t units_per_day,
    const char *unit,
    bool vertical,
    char *buffer,
    size_t capacity,
    size_t *length)
{
    const uint64_t day_us = (uint64_t)MICROSECONDS_PER_DAY;
    const uint64_t days = elapsed_microseconds / day_us;
    const uint64_t phase = elapsed_microseconds % day_us;
    uint64_t units = 0U;
    const char *separator = vertical ? "\n" : " · ";

    if (unit == NULL ||
        !duration_partition(phase, units_per_day, &units)) {
        return false;
    }
    if (days != 0U && units != 0U) {
        return write_printf(buffer, capacity, length,
                            "%llu日%s%llu%s",
                            (unsigned long long)days, separator,
                            (unsigned long long)units, unit);
    }
    if (days != 0U) {
        return write_printf(buffer, capacity, length,
                            "%llu日", (unsigned long long)days);
    }
    return write_printf(buffer, capacity, length,
                        "%llu%s", (unsigned long long)units, unit);
}

static bool format_duration_roman_temporal(
    uint64_t elapsed_microseconds,
    int64_t end_unix_microseconds,
    bool show_seconds,
    bool vertical,
    double latitude,
    double longitude,
    char *buffer,
    size_t capacity,
    size_t *length)
{
    long double horae;
    long double vigiliae;
    uint64_t hora_twelfths;
    uint64_t vigilia_twelfths;
    uint64_t whole_days;
    uint64_t whole_horae;
    uint64_t whole_vigiliae;
    unsigned hora_unciae;
    unsigned vigilia_unciae;
    char day_part[48] = "";
    char hora_part[64] = "";
    char vigilia_part[64] = "";
    const char *separator = vertical ? "\n" : " · ";
    int written;

    if (!seasonal_interval_progress(
            elapsed_microseconds, end_unix_microseconds,
            latitude, longitude, 0.833, 12U, 4U,
            &horae, &vigiliae) ||
        !seasonal_quantise(
            horae, UINT64_C(12), show_seconds, &hora_twelfths) ||
        !seasonal_quantise(
            vigiliae, UINT64_C(12), show_seconds, &vigilia_twelfths)) {
        return false;
    }

    /*
     * Twelve daylight horae plus four night vigiliae make one complete Roman
     * civil cycle. Collapse complete paired cycles into dies before showing
     * the residual seasonal units, so long intervals remain immediately
     * understandable instead of becoming unbounded hora/vigilia totals.
     */
    whole_days = hora_twelfths / UINT64_C(144);
    if (vigilia_twelfths / UINT64_C(48) < whole_days) {
        whole_days = vigilia_twelfths / UINT64_C(48);
    }
    hora_twelfths -= whole_days * UINT64_C(144);
    vigilia_twelfths -= whole_days * UINT64_C(48);

    whole_horae = hora_twelfths / UINT64_C(12);
    whole_vigiliae = vigilia_twelfths / UINT64_C(12);
    hora_unciae = (unsigned)(hora_twelfths % UINT64_C(12));
    vigilia_unciae = (unsigned)(vigilia_twelfths % UINT64_C(12));

    if (whole_days != 0U) {
        written = snprintf(day_part, sizeof(day_part), "%llu dies",
                           (unsigned long long)whole_days);
        if (written < 0 || (size_t)written >= sizeof(day_part)) return false;
    }
    if (hora_twelfths != 0U) {
        written = show_seconds && hora_unciae != 0U
            ? snprintf(hora_part, sizeof(hora_part), "%llu %s %u unciae",
                       (unsigned long long)whole_horae,
                       whole_horae == 1U ? "hora" : "horae", hora_unciae)
            : snprintf(hora_part, sizeof(hora_part), "%llu %s",
                       (unsigned long long)whole_horae,
                       whole_horae == 1U ? "hora" : "horae");
        if (written < 0 || (size_t)written >= sizeof(hora_part)) return false;
    }
    if (vigilia_twelfths != 0U) {
        written = show_seconds && vigilia_unciae != 0U
            ? snprintf(vigilia_part, sizeof(vigilia_part),
                       "%llu %s %u unciae",
                       (unsigned long long)whole_vigiliae,
                       whole_vigiliae == 1U ? "vigilia" : "vigiliae",
                       vigilia_unciae)
            : snprintf(vigilia_part, sizeof(vigilia_part), "%llu %s",
                       (unsigned long long)whole_vigiliae,
                       whole_vigiliae == 1U ? "vigilia" : "vigiliae");
        if (written < 0 || (size_t)written >= sizeof(vigilia_part)) return false;
    }

    if (day_part[0] == '\0' && hora_part[0] == '\0' &&
        vigilia_part[0] == '\0') {
        return write_text(buffer, capacity, length, "0 horae");
    }
    if (day_part[0] != '\0' && hora_part[0] != '\0' &&
        vigilia_part[0] != '\0') {
        return write_printf(buffer, capacity, length, "%s%s%s%s%s",
                            day_part, separator, hora_part,
                            separator, vigilia_part);
    }
    if (day_part[0] != '\0' && hora_part[0] != '\0') {
        return write_printf(buffer, capacity, length, "%s%s%s",
                            day_part, separator, hora_part);
    }
    if (day_part[0] != '\0' && vigilia_part[0] != '\0') {
        return write_printf(buffer, capacity, length, "%s%s%s",
                            day_part, separator, vigilia_part);
    }
    if (hora_part[0] != '\0' && vigilia_part[0] != '\0') {
        return write_printf(buffer, capacity, length, "%s%s%s",
                            hora_part, separator, vigilia_part);
    }
    if (day_part[0] != '\0') return write_text(buffer, capacity, length, day_part);
    if (hora_part[0] != '\0') return write_text(buffer, capacity, length, hora_part);
    return write_text(buffer, capacity, length, vigilia_part);
}

static bool format_duration_japanese_temporal(
    uint64_t elapsed_microseconds,
    int64_t end_unix_microseconds,
    bool show_seconds,
    bool vertical,
    double solar_depression_degrees,
    double latitude,
    double longitude,
    char *buffer,
    size_t capacity,
    size_t *length)
{
    long double daylight_toki;
    long double night_toki;
    uint64_t half_toki;
    uint64_t days;
    uint64_t remainder;
    uint64_t whole_toki;
    const char *separator = " · ";

    if (!seasonal_interval_progress(
            elapsed_microseconds, end_unix_microseconds,
            latitude, longitude, solar_depression_degrees, 6U, 6U,
            &daylight_toki, &night_toki) ||
        !seasonal_quantise(
            daylight_toki + night_toki, UINT64_C(2),
            show_seconds, &half_toki)) {
        return false;
    }

    /* Twelve unequal toki form one complete day/night cycle. */
    days = half_toki / UINT64_C(24);
    remainder = half_toki % UINT64_C(24);
    whole_toki = remainder / UINT64_C(2);
    if (vertical) {
        separator = "\n";
    }

    if (remainder == 0U) {
        return days == 0U
            ? write_text(buffer, capacity, length, "0刻")
            : write_printf(buffer, capacity, length,
                           "%llu日", (unsigned long long)days);
    }
    if (show_seconds && (remainder % UINT64_C(2)) != 0U) {
        if (days != 0U) {
            return whole_toki == 0U
                ? write_printf(buffer, capacity, length,
                               "%llu日%s半刻",
                               (unsigned long long)days, separator)
                : write_printf(buffer, capacity, length,
                               "%llu日%s%llu刻半",
                               (unsigned long long)days, separator,
                               (unsigned long long)whole_toki);
        }
        return whole_toki == 0U
            ? write_text(buffer, capacity, length, "半刻")
            : write_printf(buffer, capacity, length,
                           "%llu刻半", (unsigned long long)whole_toki);
    }
    return days != 0U
        ? write_printf(buffer, capacity, length,
                       "%llu日%s%llu刻",
                       (unsigned long long)days, separator,
                       (unsigned long long)whole_toki)
        : write_printf(buffer, capacity, length,
                       "%llu刻", (unsigned long long)whole_toki);
}

static bool format_duration_babylonian_ancient(
    uint64_t elapsed_microseconds,
    bool vertical,
    char *buffer,
    size_t capacity,
    size_t *length)
{
    const uint64_t ush_us = UINT64_C(240) * UINT64_C(1000000);
    const uint64_t beru_us = UINT64_C(30) * ush_us;
    const uint64_t umu_us = UINT64_C(12) * beru_us;
    const uint64_t umu = elapsed_microseconds / umu_us;
    const uint64_t phase = elapsed_microseconds % umu_us;
    const uint64_t beru = phase / beru_us;
    const uint64_t ush = (phase % beru_us) / ush_us;
    const char *separator = vertical ? "\n" : " · ";

    if (umu != 0U) {
        return write_printf(buffer, capacity, length,
                            "%llu ūmu%s%llu bēru%s%02llu UŠ",
                            (unsigned long long)umu, separator,
                            (unsigned long long)beru, separator,
                            (unsigned long long)ush);
    }
    return write_printf(buffer, capacity, length,
                        "%llu bēru%s%02llu UŠ",
                        (unsigned long long)beru, separator,
                        (unsigned long long)ush);
}

/*
 * Reconstruct the Nürnberger Uhr's Wendetag rule for the configured place.
 * Equal 60-minute hours are assigned as a whole-number block to daylight and
 * the remainder to night. The allocation changes only when astronomical day
 * length crosses the next half-hour threshold. This reproduces the discrete
 * Wendetag behaviour while allowing for the historical fact that nearby towns
 * used slightly different changeover dates.
 */
static bool mode_location_valid(const InfiltratrTemporalClockModeInfo *info,
                                bool location_configured,
                                double latitude,
                                double longitude)
{
    if (info == NULL) {
        return false;
    }
    if ((info->requires_latitude || info->requires_longitude) &&
        !location_configured) {
        return false;
    }
    return (!info->requires_latitude ||
            (isfinite(latitude) && latitude >= -90.0 && latitude <= 90.0)) &&
           (!info->requires_longitude ||
            (isfinite(longitude) && longitude >= -180.0 &&
             longitude <= 180.0));
}


static bool format_duration_hms(uint64_t whole_seconds,
                                bool show_seconds,
                                bool vertical,
                                const char *suffix,
                                char *buffer,
                                size_t capacity,
                                size_t *length)
{
    const uint64_t days = whole_seconds / UINT64_C(86400);
    const uint64_t phase = whole_seconds % UINT64_C(86400);
    const unsigned hour = (unsigned)(phase / UINT64_C(3600));
    const unsigned minute =
        (unsigned)((phase / UINT64_C(60)) % UINT64_C(60));
    const unsigned second = (unsigned)(phase % UINT64_C(60));
    const char *separator = vertical ? "\n" : ":";
    const char *field_separator = vertical ? "\n" : " ";
    const bool has_suffix = suffix != NULL && suffix[0] != '\0';

    if (days > 0U) {
        if (show_seconds) {
            return write_printf(
                buffer, capacity, length,
                has_suffix
                    ? (vertical ? "%llud\n%02u%s%02u%s%02u\n%s"
                                : "%llud %02u%s%02u%s%02u %s")
                    : (vertical ? "%llud\n%02u%s%02u%s%02u"
                                : "%llud %02u%s%02u%s%02u"),
                (unsigned long long)days,
                hour, separator, minute, separator, second,
                has_suffix ? suffix : "");
        }
        return write_printf(
            buffer, capacity, length,
            has_suffix
                ? (vertical ? "%llud\n%02u%s%02u\n%s"
                            : "%llud %02u%s%02u %s")
                : (vertical ? "%llud\n%02u%s%02u"
                            : "%llud %02u%s%02u"),
            (unsigned long long)days,
            hour, separator, minute,
            has_suffix ? suffix : "");
    }

    if (show_seconds) {
        return write_printf(
            buffer, capacity, length,
            has_suffix
                ? (vertical ? "%02u%s%02u%s%02u\n%s"
                            : "%02u%s%02u%s%02u %s")
                : "%02u%s%02u%s%02u",
            hour, separator, minute, separator, second,
            has_suffix ? suffix : "");
    }

    if (has_suffix) {
        return write_printf(
            buffer, capacity, length,
            vertical ? "%02u%s%02u\n%s" : "%02u%s%02u %s",
            hour, separator, minute, suffix);
    }
    (void)field_separator;
    return write_printf(
        buffer, capacity, length, "%02u%s%02u",
        hour, separator, minute);
}

static bool duration_partition(uint64_t phase_microseconds,
                               uint64_t ticks_per_day,
                               uint64_t *tick)
{
    return infiltratr_cycle_partition_u64(
        phase_microseconds,
        (uint64_t)MICROSECONDS_PER_DAY,
        ticks_per_day,
        tick,
        NULL);
}

static bool format_duration_decimal(uint64_t elapsed_microseconds,
                                    bool show_seconds,
                                    bool vertical,
                                    char *buffer,
                                    size_t capacity,
                                    size_t *length)
{
    const uint64_t day_us = (uint64_t)MICROSECONDS_PER_DAY;
    const uint64_t days = elapsed_microseconds / day_us;
    const uint64_t phase = elapsed_microseconds % day_us;
    const uint64_t ticks_per_day =
        show_seconds ? DECIMAL_SECONDS_PER_DAY : UINT64_C(1000);
    uint64_t tick = 0U;
    uint64_t hour;
    uint64_t minute;
    uint64_t second = 0U;
    const char *separator = vertical ? "\n" : ":";

    if (!duration_partition(phase, ticks_per_day, &tick)) {
        return false;
    }
    if (show_seconds) {
        hour = tick / UINT64_C(10000);
        minute = (tick / UINT64_C(100)) % UINT64_C(100);
        second = tick % UINT64_C(100);
    } else {
        hour = tick / UINT64_C(100);
        minute = tick % UINT64_C(100);
    }

    if (days > 0U) {
        return show_seconds
            ? write_printf(buffer, capacity, length,
                           vertical ? "%llud\n%llu%s%02llu%s%02llu"
                                    : "%llud %llu%s%02llu%s%02llu",
                           (unsigned long long)days,
                           (unsigned long long)hour, separator,
                           (unsigned long long)minute, separator,
                           (unsigned long long)second)
            : write_printf(buffer, capacity, length,
                           vertical ? "%llud\n%llu%s%02llu"
                                    : "%llud %llu%s%02llu",
                           (unsigned long long)days,
                           (unsigned long long)hour, separator,
                           (unsigned long long)minute);
    }
    return show_seconds
        ? write_printf(buffer, capacity, length, "%llu%s%02llu%s%02llu",
                       (unsigned long long)hour, separator,
                       (unsigned long long)minute, separator,
                       (unsigned long long)second)
        : write_printf(buffer, capacity, length, "%llu%s%02llu",
                       (unsigned long long)hour, separator,
                       (unsigned long long)minute);
}

static bool format_duration_binary(uint64_t elapsed_microseconds,
                                   bool show_seconds,
                                   bool vertical,
                                   char *buffer,
                                   size_t capacity,
                                   size_t *length)
{
    const uint64_t whole_seconds =
        elapsed_microseconds / (uint64_t)MICROSECONDS_PER_SECOND;
    const uint64_t days = whole_seconds / UINT64_C(86400);
    const uint64_t phase = whole_seconds % UINT64_C(86400);
    const unsigned hour = (unsigned)(phase / UINT64_C(3600));
    const unsigned minute =
        (unsigned)((phase / UINT64_C(60)) % UINT64_C(60));
    const unsigned second = (unsigned)(phase % UINT64_C(60));
    char hour_bits[6];
    char minute_bits[7];
    char second_bits[7];
    const char *separator = vertical ? "\n" : ":";
    unsigned bit;

    for (bit = 0U; bit < 5U; ++bit) {
        hour_bits[bit] =
            (hour & (1U << (4U - bit))) != 0U ? '1' : '0';
    }
    hour_bits[5] = '\0';
    for (bit = 0U; bit < 6U; ++bit) {
        minute_bits[bit] =
            (minute & (1U << (5U - bit))) != 0U ? '1' : '0';
        second_bits[bit] =
            (second & (1U << (5U - bit))) != 0U ? '1' : '0';
    }
    minute_bits[6] = '\0';
    second_bits[6] = '\0';

    if (days > 0U) {
        return show_seconds
            ? write_printf(
                buffer, capacity, length,
                vertical ? "%llud\n%s%s%s%s%s"
                         : "%llud %s%s%s%s%s",
                (unsigned long long)days,
                hour_bits, separator, minute_bits, separator, second_bits)
            : write_printf(
                buffer, capacity, length,
                vertical ? "%llud\n%s%s%s" : "%llud %s%s%s",
                (unsigned long long)days,
                hour_bits, separator, minute_bits);
    }
    return show_seconds
        ? write_printf(buffer, capacity, length, "%s%s%s%s%s",
                       hour_bits, separator, minute_bits, separator, second_bits)
        : write_printf(buffer, capacity, length, "%s%s%s",
                       hour_bits, separator, minute_bits);
}

static bool format_duration_day_ticks(uint64_t elapsed_microseconds,
                                      uint64_t ticks_per_day,
                                      const char *prefix,
                                      const char *suffix,
                                      unsigned width,
                                      bool hexadecimal,
                                      bool vertical,
                                      char *buffer,
                                      size_t capacity,
                                      size_t *length)
{
    const uint64_t day_us = (uint64_t)MICROSECONDS_PER_DAY;
    const uint64_t days = elapsed_microseconds / day_us;
    const uint64_t phase = elapsed_microseconds % day_us;
    uint64_t tick = 0U;

    if (!duration_partition(phase, ticks_per_day, &tick)) {
        return false;
    }

    if (hexadecimal) {
        if (days > 0U) {
            return write_printf(
                buffer, capacity, length,
                vertical ? "%llud\n%0*llX" : "%llud %0*llX",
                (unsigned long long)days, (int)width,
                (unsigned long long)tick);
        }
        return write_printf(
            buffer, capacity, length, "%0*llX",
            (int)width, (unsigned long long)tick);
    }

    if (days > 0U) {
        return write_printf(
            buffer, capacity, length,
            vertical ? "%llud\n%s%0*llu%s" : "%llud %s%0*llu%s",
            (unsigned long long)days,
            prefix != NULL ? prefix : "",
            (int)width, (unsigned long long)tick,
            suffix != NULL ? suffix : "");
    }
    return write_printf(
        buffer, capacity, length, "%s%0*llu%s",
        prefix != NULL ? prefix : "",
        (int)width, (unsigned long long)tick,
        suffix != NULL ? suffix : "");
}

static bool format_duration_julian(const char *mode,
                                   uint64_t elapsed_microseconds,
                                   bool show_seconds,
                                   bool vertical,
                                   char *buffer,
                                   size_t capacity,
                                   size_t *length)
{
    const uint64_t day_us = (uint64_t)MICROSECONDS_PER_DAY;
    const uint64_t days = elapsed_microseconds / day_us;
    const uint64_t phase = elapsed_microseconds % day_us;
    const uint64_t scale =
        show_seconds ? UINT64_C(100000) : UINT64_C(1000);
    uint64_t fraction = 0U;
    const int digits = show_seconds ? 5 : 3;
    const char *label =
        strcmp(mode, "modified-julian") == 0 ? "MJD" : "JD";

    if (!duration_partition(phase, scale, &fraction)) {
        return false;
    }
    return write_printf(
        buffer, capacity, length,
        vertical ? "%s\n+%llu.%0*llu d" : "%s +%llu.%0*llu d",
        label, (unsigned long long)days, digits,
        (unsigned long long)fraction);
}

static bool format_duration_internet(uint64_t elapsed_microseconds,
                                     bool show_seconds,
                                     bool vertical,
                                     char *buffer,
                                     size_t capacity,
                                     size_t *length)
{
    const uint64_t day_us = (uint64_t)MICROSECONDS_PER_DAY;
    const uint64_t days = elapsed_microseconds / day_us;
    const uint64_t phase = elapsed_microseconds % day_us;
    const uint64_t ticks_per_day =
        show_seconds
            ? INTERNET_TICKS_PER_DAY * UINT64_C(100)
            : INTERNET_TICKS_PER_DAY;
    uint64_t tick = 0U;

    if (!duration_partition(phase, ticks_per_day, &tick)) {
        return false;
    }
    if (days > 0U) {
        return show_seconds
            ? write_printf(
                buffer, capacity, length,
                vertical ? "%llud\n@%03llu.%02llu"
                         : "%llud @%03llu.%02llu",
                (unsigned long long)days,
                (unsigned long long)(tick / UINT64_C(100)),
                (unsigned long long)(tick % UINT64_C(100)))
            : write_printf(
                buffer, capacity, length,
                vertical ? "%llud\n@%03llu" : "%llud @%03llu",
                (unsigned long long)days,
                (unsigned long long)tick);
    }
    return show_seconds
        ? write_printf(buffer, capacity, length, "@%03llu.%02llu",
                       (unsigned long long)(tick / UINT64_C(100)),
                       (unsigned long long)(tick % UINT64_C(100)))
        : write_printf(buffer, capacity, length, "@%03llu",
                       (unsigned long long)tick);
}

static bool format_duration_sidereal(uint64_t elapsed_microseconds,
                                     bool show_seconds,
                                     bool vertical,
                                     char *buffer,
                                     size_t capacity,
                                     size_t *length)
{
    const long double sidereal_seconds =
        (long double)elapsed_microseconds /
        (long double)MICROSECONDS_PER_SECOND *
        SIDEREAL_RATE_PER_SI_SECOND;

    if (!isfinite((double)sidereal_seconds) ||
        sidereal_seconds < 0.0L ||
        sidereal_seconds > (long double)UINT64_MAX) {
        return false;
    }
    return format_duration_hms(
        (uint64_t)floorl(sidereal_seconds),
        show_seconds, vertical, "LST",
        buffer, capacity, length);
}

static bool format_duration_apparent_solar(uint64_t elapsed_microseconds,
                                           int64_t end_unix_microseconds,
                                           bool show_seconds,
                                           bool vertical,
                                           char *buffer,
                                           size_t capacity,
                                           size_t *length)
{
    double start_equation_minutes = 0.0;
    double end_equation_minutes = 0.0;
    int64_t start_unix_microseconds;
    long double apparent_microseconds;
    long double apparent_seconds;

    if (elapsed_microseconds > (uint64_t)INT64_MAX ||
        end_unix_microseconds <
            INT64_MIN + (int64_t)elapsed_microseconds) {
        return format_duration_hms(
            elapsed_microseconds / (uint64_t)MICROSECONDS_PER_SECOND,
            show_seconds, vertical, "SOL",
            buffer, capacity, length);
    }

    start_unix_microseconds =
        end_unix_microseconds - (int64_t)elapsed_microseconds;
    if (!solar_terms(start_unix_microseconds,
                     &start_equation_minutes, NULL) ||
        !solar_terms(end_unix_microseconds,
                     &end_equation_minutes, NULL)) {
        return false;
    }

    apparent_microseconds =
        (long double)elapsed_microseconds +
        ((long double)end_equation_minutes -
         (long double)start_equation_minutes) *
            60.0L * (long double)MICROSECONDS_PER_SECOND;
    if (apparent_microseconds < 0.0L) {
        apparent_microseconds = 0.0L;
    }
    apparent_seconds =
        floorl(apparent_microseconds /
               (long double)MICROSECONDS_PER_SECOND);
    if (apparent_seconds > (long double)UINT64_MAX) {
        return false;
    }
    return format_duration_hms(
        (uint64_t)apparent_seconds,
        show_seconds, vertical, "SOL",
        buffer, capacity, length);
}

static bool format_duration_ghati(uint64_t elapsed_microseconds,
                                  bool vertical,
                                  char *buffer,
                                  size_t capacity,
                                  size_t *length)
{
    const uint64_t day_us = (uint64_t)MICROSECONDS_PER_DAY;
    const uint64_t days = elapsed_microseconds / day_us;
    const uint64_t phase = elapsed_microseconds % day_us;
    uint64_t pala = 0U;
    uint64_t ghati;
    const char *separator = vertical ? "\n" : " · ";

    if (!duration_partition(phase, VIGHATI_PER_DAY, &pala)) {
        return false;
    }
    ghati = pala / UINT64_C(60);
    pala %= UINT64_C(60);

    if (days > 0U) {
        return write_printf(
            buffer, capacity, length,
            "%llu dina%s%llu ghaṭī%s%llu pala",
            (unsigned long long)days, separator,
            (unsigned long long)ghati, separator,
            (unsigned long long)pala);
    }
    return write_printf(
        buffer, capacity, length,
        "%llu ghaṭī%s%llu pala",
        (unsigned long long)ghati, separator,
        (unsigned long long)pala);
}

bool infiltratr_temporal_format_duration_mode(
    const char *mode,
    uint64_t elapsed_microseconds,
    int64_t end_unix_microseconds,
    bool show_seconds,
    bool vertical,
    bool location_configured,
    double latitude,
    double longitude,
    char *buffer,
    size_t capacity,
    size_t *length)
{
    const InfiltratrTemporalClockModeInfo *info;
    const uint64_t whole_seconds =
        elapsed_microseconds / (uint64_t)MICROSECONDS_PER_SECOND;

    if (mode == NULL || buffer == NULL || capacity == 0U) {
        return false;
    }
    buffer[0] = '\0';
    info = infiltratr_temporal_clock_mode_find(mode);
    if (info == NULL ||
        !mode_location_valid(
            info, location_configured, latitude, longitude)) {
        return false;
    }

    if (strcmp(mode, "standard") == 0 ||
        strcmp(mode, "standard-24") == 0 ||
        strcmp(mode, "standard-12") == 0) {
        return format_duration_hms(
            whole_seconds, show_seconds, vertical, NULL,
            buffer, capacity, length);
    }

    if (strcmp(mode, "roman-temporal") == 0 ||
        strcmp(mode, "japanese-temporal") == 0 ||
        strcmp(mode, "japanese-temporal-early") == 0) {
        if (!location_configured ||
            !isfinite(latitude) || !isfinite(longitude)) {
            return false;
        }
        /*
         * Seasonal civil durations are meaningful only when anchored to a real
         * interval. For abstract accumulated quantities there is no historical
         * seasonal unit to apply honestly, so retain SI duration and label it.
         */
        if (end_unix_microseconds == INT64_MIN) {
            return format_duration_hms(
                whole_seconds, show_seconds, vertical, "SI",
                buffer, capacity, length);
        }
        if (strcmp(mode, "roman-temporal") == 0) {
            return format_duration_roman_temporal(
                elapsed_microseconds, end_unix_microseconds,
                show_seconds, vertical, latitude, longitude,
                buffer, capacity, length);
        }
        return format_duration_japanese_temporal(
            elapsed_microseconds, end_unix_microseconds,
            show_seconds, vertical,
            strcmp(mode, "japanese-temporal") == 0
                ? 7.0 + 21.0 / 60.0 + 40.0 / 3600.0
                : 0.833,
            latitude, longitude,
            buffer, capacity, length);
    }

    if (strcmp(mode, "babylonian-ancient") == 0) {
        return format_duration_babylonian_ancient(
            elapsed_microseconds, vertical,
            buffer, capacity, length);
    }

    if (strcmp(mode, "decimal") == 0) {
        return format_duration_decimal(
            elapsed_microseconds, show_seconds, vertical,
            buffer, capacity, length);
    }

    if (strcmp(mode, "internet") == 0) {
        return format_duration_internet(
            elapsed_microseconds, show_seconds, vertical,
            buffer, capacity, length);
    }

    if (strcmp(mode, "unix") == 0) {
        return write_printf(
            buffer, capacity, length, "%llu s",
            (unsigned long long)whole_seconds);
    }

    if (strcmp(mode, "binary") == 0) {
        return format_duration_binary(
            elapsed_microseconds, show_seconds, vertical,
            buffer, capacity, length);
    }

    if (strcmp(mode, "hexadecimal") == 0) {
        return format_duration_day_ticks(
            elapsed_microseconds, HEX_TICKS_PER_DAY,
            NULL, NULL, 4U, true, vertical,
            buffer, capacity, length);
    }

    if (strcmp(mode, "julian") == 0 ||
        strcmp(mode, "modified-julian") == 0) {
        return format_duration_julian(
            mode, elapsed_microseconds, show_seconds, vertical,
            buffer, capacity, length);
    }

    if (strcmp(mode, "sidereal") == 0) {
        return format_duration_sidereal(
            elapsed_microseconds, show_seconds, vertical,
            buffer, capacity, length);
    }

    if (strcmp(mode, "solar") == 0) {
        if (!location_configured || !isfinite(longitude)) {
            return false;
        }
        return format_duration_apparent_solar(
            elapsed_microseconds, end_unix_microseconds,
            show_seconds, vertical, buffer, capacity, length);
    }

    if (strcmp(mode, "mean-solar") == 0) {
        if (!location_configured || !isfinite(longitude)) {
            return false;
        }
        return format_duration_hms(
            whole_seconds, show_seconds, vertical, "LMT",
            buffer, capacity, length);
    }

    if (strcmp(mode, "chinese-time") == 0) {
        return format_duration_chinese_partition(
            elapsed_microseconds, CHINESE_DOUBLE_HOURS_PER_DAY,
            "時辰", vertical, buffer, capacity, length);
    }

    if (strcmp(mode, "chinese-ke") == 0) {
        return format_duration_chinese_partition(
            elapsed_microseconds, CHINESE_KE_PER_DAY,
            "刻", vertical, buffer, capacity, length);
    }

    if (strcmp(mode, "italian-hours") == 0) {
        if (!location_configured ||
            !isfinite(latitude) || !isfinite(longitude)) {
            return false;
        }
        return format_duration_hms(
            whole_seconds, show_seconds, vertical, "IT",
            buffer, capacity, length);
    }

    if (strcmp(mode, "babylonian-hours") == 0) {
        if (!location_configured ||
            !isfinite(latitude) || !isfinite(longitude)) {
            return false;
        }
        return format_duration_hms(
            whole_seconds, show_seconds, vertical, "BAB",
            buffer, capacity, length);
    }

    if (strcmp(mode, "indian-ghati") == 0) {
        if (!location_configured ||
            !isfinite(latitude) || !isfinite(longitude)) {
            return false;
        }
        return format_duration_ghati(
            elapsed_microseconds, vertical,
            buffer, capacity, length);
    }

    if (strcmp(mode, "nuremberg-hours") == 0) {
        return format_duration_hms(
            whole_seconds, show_seconds, vertical, "NUR",
            buffer, capacity, length);
    }

    if (strcmp(mode, "nuremberg-solar") == 0) {
        if (!location_configured ||
            !isfinite(latitude) || !isfinite(longitude)) {
            return false;
        }
        return format_duration_hms(
            whole_seconds, show_seconds, vertical, "NUR-L",
            buffer, capacity, length);
    }

    return false;
}

/* Legacy linkage only. Absolute clock faces require an installed resource. */
bool infiltratr_temporal_format_clock_mode(const char *mode,
    int64_t instant, int32_t offset, bool seconds, bool vertical,
    bool configured, double latitude, double longitude,
    char *buffer, size_t capacity, size_t *length)
{
    (void)mode; (void)instant; (void)offset; (void)seconds; (void)vertical;
    (void)configured; (void)latitude; (void)longitude;
    (void)buffer; (void)capacity; (void)length;
    return false;
}
bool infiltratr_temporal_format_clock_mode_localized(
    const char *locale, const char *mode, int64_t instant, int32_t offset,
    bool seconds, bool vertical, bool configured, double latitude,
    double longitude, char *buffer, size_t capacity, size_t *length)
{
    (void)locale;
    return infiltratr_temporal_format_clock_mode(mode, instant, offset,
        seconds, vertical, configured, latitude, longitude,
        buffer, capacity, length);
}
