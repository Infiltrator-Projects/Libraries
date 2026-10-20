// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 1993-2026 Shannon Smith
/*
 * Standalone 24-equal-hour sunrise/sunset-origin clock resource.
 * Compile with CLOCK_SUNRISE for Renaissance Babylonian hours, otherwise
 * Italian hours. The two IDs remain independently installed capabilities.
 * Canonical instants are UTC microseconds; civil timezone does not alter
 * the solar-event calculations.
 */
#include <infiltratr/temporal_clock_resource.h>
#include "temporal_solar_compact.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define US_SECOND INT64_C(1000000)
#define US_DAY INT64_C(86400000000)
#define PI 3.141592653589793238462643383279502884L
#if defined(CLOCK_ANCIENT_BABYLONIAN)
#define CLOCK_ID "babylonian-ancient"
#define CLOCK_SUFFIX "BAB-A"
#elif defined(CLOCK_GHATI)
#define CLOCK_ID "indian-ghati"
#define CLOCK_SUFFIX "GH"
#elif defined(CLOCK_SUNRISE)
#define CLOCK_ID "babylonian-hours"
#define CLOCK_SUFFIX "BAB-R"
#else
#define CLOCK_ID "italian-hours"
#define CLOCK_SUFFIX "IT"
#endif

static int64_t floor_div(int64_t n, int64_t d)
{ int64_t q = n / d; return n % d < 0 ? q - 1 : q; }

/* Gregorian 400-year arithmetic avoids time_t, localtime and locale drift. */
/* Origin and calendar traditions differ; the solar model is shared. */
static bool solar_terms(int64_t instant, double *equation_minutes,
                        double *declination_radians)
{
    return temporal_solar_compact_terms(
        instant, equation_minutes, declination_radians);
}

static bool origin_on_day(int64_t day, double lat, double lon, int64_t *event)
{
    const int64_t safe_day = INT64_MAX/US_DAY-2;
    if (!event || day < -safe_day || day > safe_day ||
        !isfinite(lat) || lat < -90.0 || lat > 90.0 ||
        !isfinite(lon) || lon < -180.0 || lon > 180.0) return false;
    int64_t noon = day*US_DAY + US_DAY/2;
    double eq, decl;
    if (!solar_terms(noon, &eq, &decl)) return false;
    double latrad = lat * (double)PI / 180.0;
    double zenith = 90.833 * (double)PI / 180.0;
    double divisor = cos(latrad)*cos(decl);
    if (fabs(divisor) < 1.0e-12) return false;
    double cos_ha = cos(zenith)/divisor-tan(latrad)*tan(decl);
    if (cos_ha < -1.0 || cos_ha > 1.0) return false;
    long double ha_minutes = acosl((long double)cos_ha)*180.0L/PI*4.0L;
    long double noon_minutes = 720.0L-4.0L*lon-eq;
#if defined(CLOCK_SUNRISE) || defined(CLOCK_GHATI)
    long double event_minutes = noon_minutes-ha_minutes;
#else
    long double event_minutes = noon_minutes+ha_minutes;
#endif
    long double instant = (long double)day*US_DAY +
                          event_minutes*60.0L*US_SECOND;
    if (instant < INT64_MIN || instant > INT64_MAX) return false;
    *event = (int64_t)llroundl(instant);
    return true;
}

static bool previous_origin(int64_t instant, double lat, double lon,
                            int64_t *origin)
{
    if (!origin || instant < INT64_MIN+3*US_DAY ||
        instant > INT64_MAX-3*US_DAY) return false;
    int64_t day = floor_div(instant, US_DAY);
    int64_t best = INT64_MIN;
    for (int delta=-2; delta<=2; ++delta) {
        int64_t candidate;
        if (origin_on_day(day+delta,lat,lon,&candidate) &&
            candidate<=instant && candidate>best) best=candidate;
    }
    if (best==INT64_MIN) return false;
    *origin=best;
    return true;
}

static bool render(const InfiltratrTemporalClockProvider *self,
                   const char *locale, int64_t instant, int32_t offset,
                   bool seconds, bool vertical, double lat, double lon,
                   char *dst, size_t capacity, size_t *length)
{
    (void)self; (void)locale; (void)offset;
    if (!dst || capacity==0 || !isfinite(lat) || lat < -90.0 ||
        lat > 90.0 || !isfinite(lon) || lon < -180.0 ||
        lon > 180.0) return false;
    char text[96]; int n;
    int64_t origin;
    if (!previous_origin(instant,lat,lon,&origin)) {
        n=snprintf(text,sizeof(text),vertical?"N/A\n%s":"N/A %s",CLOCK_SUFFIX);
    } else {
        int64_t elapsed=floor_div(instant-origin,US_SECOND);
#if defined(CLOCK_ANCIENT_BABYLONIAN)
        const int beru=(int)(elapsed/7200)+1;
        const int ush=(int)((elapsed%7200)/240);
        (void)seconds;
        n=snprintf(text,sizeof(text),vertical?"bēru %02d\nUŠ %02d":
                   "bēru %02d · UŠ %02d",beru,ush);
#elif defined(CLOCK_GHATI)
        const int ghati=(int)(elapsed/1440);
        const int pala=(int)((elapsed/24)%60);
        (void)seconds;
        n=snprintf(text,sizeof(text),vertical?"%d ghaṭī\n%d pala":
                   "%d ghaṭī · %d pala",ghati,pala);
#else
        int hour=(int)(elapsed/3600);
        int minute=(int)(elapsed/60%60);
        int second=(int)(elapsed%60);
        const char *sep=vertical?"\n":":";
        n=seconds ? snprintf(text,sizeof(text),"%02d%s%02d%s%02d%s%s",
                          hour,sep,minute,sep,second,vertical?"\n":" ",CLOCK_SUFFIX)
                  : snprintf(text,sizeof(text),"%02d%s%02d%s%s",
                          hour,sep,minute,vertical?"\n":" ",CLOCK_SUFFIX);
#endif
    }
    if (n<=0 || (size_t)n>=sizeof(text) || (size_t)n>=capacity) return false;
    memcpy(dst,text,(size_t)n+1);
    if (length) *length=(size_t)n;
    return true;
}

static bool changed(int64_t instant,uint32_t delay,bool seconds,
                    double lat,double lon,const char *previous)
{
    int64_t delta=(int64_t)delay*1000;
    char output[96];
    return instant<=INT64_MAX-delta &&
        render(NULL,NULL,instant+delta,0,seconds,false,lat,lon,
               output,sizeof(output),NULL) && strcmp(previous,output)!=0;
}

static bool next_tick(const InfiltratrTemporalClockProvider *self,
                      int64_t instant,int32_t offset,bool seconds,
                      double lat,double lon,uint32_t *milliseconds)
{
    (void)self;(void)offset;
    if (!milliseconds) return false;
    char previous[96];
    if (!render(NULL,NULL,instant,0,seconds,false,lat,lon,
                previous,sizeof(previous),NULL)) return false;
    uint32_t lo=0;
#if defined(CLOCK_ANCIENT_BABYLONIAN)
    uint32_t hi=241000U;
#elif defined(CLOCK_GHATI)
    uint32_t hi=25000U;
#else
    uint32_t hi=seconds?2000U:61000U;
#endif
    if (!changed(instant,hi,seconds,lat,lon,previous)) {
        /* A polar interval without a crossing has stable N/A output. */
        if (strncmp(previous,"N/A ",4)==0) {
            *milliseconds=60000U;
            return true;
        }
        return false;
    }
    while (hi-lo>1) {
        uint32_t mid=lo+(hi-lo)/2;
        if (changed(instant,mid,seconds,lat,lon,previous)) hi=mid;
        else lo=mid;
    }
    *milliseconds=hi;
    return true;
}

static const InfiltratrTemporalResourceMetadata metadata={
    sizeof(InfiltratrTemporalResourceMetadata),
    INFILTRATR_TEMPORAL_RESOURCE_METADATA_ABI,
    INFILTRATR_TEMPORAL_RESOURCE_CLOCK,
    CLOCK_ID,INFILTRATR_TEMPORAL_DISPLAY_NAME,
#if defined(CLOCK_ANCIENT_BABYLONIAN) || defined(CLOCK_GHATI)
    false,
#else
    true,
#endif
    true,true
};
static const InfiltratrTemporalClockProvider provider={
    sizeof(InfiltratrTemporalClockProvider),
    INFILTRATR_TEMPORAL_CLOCK_PROVIDER_ABI,
    CLOCK_ID,render,next_tick
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
infiltratr_temporal_resource_metadata_v1(void){return &metadata;}
EXPORT const InfiltratrTemporalClockProvider *
infiltratr_temporal_clock_provider_v1(void){return &provider;}
