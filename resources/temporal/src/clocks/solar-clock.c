// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Independent local apparent solar clock (SOL).
 * NOAA/GML compact fractional-year equation-of-time approximation, matching
 * the Infiltrator documented presentation model (not an ephemeris).
 * Time is a canonical Unix-microsecond instant; longitude is east positive.
 * Locale, conventional time zone and daylight-saving rules do not change
 * solar geometry. Exact visible boundaries are resolved from the resource's
 * own output rather than a duplicate application polling policy.
 */
#include <infiltratr/temporal_clock_resource.h>
#include "temporal_solar_compact.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SECOND_US INT64_C(1000000)
#define DAY_US INT64_C(86400000000)

static bool apparent_seconds(int64_t instant, double longitude,
                             long double *result)
{
    const long double utc_phase =
        (long double)temporal_solar_mod(instant, DAY_US);
    double equation_minutes = 0.0;
    long double seconds;
    if (result == NULL || !temporal_solar_valid_longitude(longitude) ||
        !temporal_solar_compact_terms(instant, &equation_minutes, NULL))
        return false;

    seconds = utc_phase / (long double)SECOND_US +
              (long double)longitude * 240.0L +
              (long double)equation_minutes * 60.0L;
    seconds = fmodl(seconds, 86400.0L);
    if (seconds < 0.0L) seconds += 86400.0L;
    *result = seconds;
    return true;
}

static bool render(const InfiltratrTemporalClockProvider *provider,
                   const char *locale, int64_t instant, int32_t offset,
                   bool show_seconds, bool vertical, double latitude,
                   double longitude, char *buffer, size_t capacity,
                   size_t *length)
{
    const char *sep = vertical ? "\n" : ":";
    const char *suffix_sep = vertical ? "\n" : " ";
    long double solar;
    int64_t whole;
    unsigned int hour, minute, second;
    char text[96];
    int written;
    (void)provider; (void)locale; (void)offset; (void)latitude;
    if (!apparent_seconds(instant, longitude, &solar))
        return false;
    whole = (int64_t)floorl(solar);
    hour = (unsigned int)(whole / INT64_C(3600));
    minute = (unsigned int)((whole / INT64_C(60)) % INT64_C(60));
    second = (unsigned int)(whole % INT64_C(60));
    if (show_seconds)
        written = snprintf(text, sizeof(text), "%02u%s%02u%s%02u%sSOL",
                           hour, sep, minute, sep, second, suffix_sep);
    else
        written = snprintf(text, sizeof(text), "%02u%s%02u%sSOL",
                           hour, sep, minute, suffix_sep);
    if (buffer == NULL || written <= 0 ||
        (size_t)written >= sizeof(text) || (size_t)written >= capacity)
        return false;
    memcpy(buffer, text, (size_t)written + 1U);
    if (length != NULL) *length = (size_t)written;
    return true;
}

static bool text_changed(int64_t instant, uint32_t delta_ms,
                         bool show_seconds, double longitude,
                         const char *original)
{
    char next[96] = {0};
    const int64_t delta = (int64_t)delta_ms * INT64_C(1000);
    if (instant > INT64_MAX - delta)
        return false;
    if (!render(NULL, NULL, instant + delta, 0, show_seconds,
                false, 0.0, longitude, next, sizeof(next), NULL))
        return false;
    return strcmp(original, next) != 0;
}

/* Binary-search the first displayed change at millisecond resolution. */
static bool next_tick(const InfiltratrTemporalClockProvider *provider,
                      int64_t instant, int32_t offset, bool show_seconds,
                      double latitude, double longitude,
                      uint32_t *milliseconds)
{
    char original[96] = {0};
    uint32_t low = 0U;
    uint32_t high = show_seconds ? 2000U : 61000U;
    (void)provider; (void)offset; (void)latitude;
    if (milliseconds == NULL ||
        !render(NULL, NULL, instant, 0, show_seconds, false,
                0.0, longitude, original, sizeof(original), NULL) ||
        !text_changed(instant, high, show_seconds, longitude, original))
        return false;
    while (high - low > 1U) {
        const uint32_t middle = low + (high - low) / 2U;
        if (text_changed(instant, middle, show_seconds,
                         longitude, original))
            high = middle;
        else
            low = middle;
    }
    *milliseconds = high;
    return true;
}

static const InfiltratrTemporalResourceMetadata metadata = {
    sizeof(InfiltratrTemporalResourceMetadata),
    INFILTRATR_TEMPORAL_RESOURCE_METADATA_ABI,
    INFILTRATR_TEMPORAL_RESOURCE_CLOCK,
    "solar", INFILTRATR_TEMPORAL_DISPLAY_NAME,
    true, false, true
};
static const InfiltratrTemporalClockProvider provider = {
    sizeof(InfiltratrTemporalClockProvider),
    INFILTRATR_TEMPORAL_CLOCK_PROVIDER_ABI,
    "solar", render, next_tick
};

#if defined(_WIN32)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif
EXPORT const InfiltratrTemporalResourceMetadata *
infiltratr_temporal_resource_metadata_v1(void);
EXPORT const InfiltratrTemporalClockProvider *
infiltratr_temporal_clock_provider_v1(void);
EXPORT const InfiltratrTemporalResourceMetadata *
infiltratr_temporal_resource_metadata_v1(void) { return &metadata; }
EXPORT const InfiltratrTemporalClockProvider *
infiltratr_temporal_clock_provider_v1(void) { return &provider; }
