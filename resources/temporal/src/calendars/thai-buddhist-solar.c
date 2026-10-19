// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Thai Buddhist solar civil calendar, 1 April 1912 onwards.
 *
 * The existing "buddhist" resource represents the modern +543 Gregorian-era
 * projection. It must not be silently repurposed to represent a different
 * historical chronology. This independent resource implements the documented
 * Thai civil-year transitions:
 *
 *   1912-04-01..1940-12-31: New Year falls on 1 April.
 *   1941-01-01 onwards:     New Year falls on 1 January.
 *
 * Consequently 2483 B.E. contains April-December 1940 only. January-March
 * 1940 belong to 2482 B.E.; January 1941 begins 2484 B.E. A request for a
 * nonexistent month in 2483 B.E. fails rather than being remapped to a date
 * in a different year. Earlier Thai eras require their own sources and
 * resource identities, so this module deliberately rejects pre-April 1912.
 *
 * Authority: Thailand Parliamentary Museum's history of the 2483 Calendar
 * Act (https://parliamentmuseum.go.th/ar63-Calendar.html). Numeric solar
 * dates use the Gregorian day/month introduced for Thai civil administration;
 * era-year policy is owned here.
 */
#include <infiltratr/temporal_calendar.h>

#include <glib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define THAI_START_JDN INT64_C(2419494)
#define GREGORIAN_JDN_ORDINAL_OFFSET INT64_C(1721425)
#define FIRST_THAI_YEAR INT64_C(2455)
#define LAST_APRIL_YEAR INT64_C(2483)
#define FIRST_JANUARY_YEAR INT64_C(2484)

static bool date_at(int64_t jdn, GDate *date)
{
    /* Validate before subtraction: arbitrary JDNs must not overflow. */
    if (date == NULL || jdn < THAI_START_JDN ||
        jdn > GREGORIAN_JDN_ORDINAL_OFFSET + G_MAXUINT32)
        return false;
    const int64_t ordinal = jdn - GREGORIAN_JDN_ORDINAL_OFFSET;
    g_date_clear(date, 1U);
    g_date_set_julian(date, (guint32)ordinal);
    return g_date_valid(date);
}

static int64_t thai_year(const GDate *date)
{
    const int year = g_date_get_year(date);
    const int month = g_date_get_month(date);
    return (int64_t)year +
        ((year <= 1940 && month < 4) ? 542 : 543);
}

static bool fields_at(int64_t jdn, InfiltratrTemporalCalendarFields *fields)
{
    GDate date;

    if (fields == NULL || !date_at(jdn, &date))
        return false;
    *fields = (InfiltratrTemporalCalendarFields){
        .year = thai_year(&date),
        .month = (int32_t)g_date_get_month(&date),
        .day = (int32_t)g_date_get_day(&date),
        .auxiliary = 0,
        .special = false
    };
    return true;
}

static bool thai_to_jdn(int64_t buddhist_year, int month, int day,
                        int64_t *result)
{
    GDate date;
    int64_t gregorian_year;
    int64_t jdn;

    if (result == NULL || buddhist_year < FIRST_THAI_YEAR ||
        month < 1 || month > 12 ||
        (buddhist_year == LAST_APRIL_YEAR && month < 4))
        return false;

    /*
     * Before 2484 B.E., January-March belong to the civil year which
     * started in the preceding Gregorian April. The 2483 transition omits
     * those months entirely; applying +542 to them would invent dates.
     */
    gregorian_year = buddhist_year -
        ((buddhist_year < FIRST_JANUARY_YEAR && month < 4) ? 542 : 543);
    if (gregorian_year < 1 || gregorian_year > G_MAXUINT16 ||
        day < 1 || day > 31 ||
        !g_date_valid_dmy((GDateDay)day, (GDateMonth)month,
                          (GDateYear)gregorian_year))
        return false;

    g_date_clear(&date, 1U);
    g_date_set_dmy(&date, (GDateDay)day, (GDateMonth)month,
                   (GDateYear)gregorian_year);
    jdn = (int64_t)g_date_get_julian(&date) +
          GREGORIAN_JDN_ORDINAL_OFFSET;
    if (jdn < THAI_START_JDN || thai_year(&date) != buddhist_year)
        return false;
    *result = jdn;
    return true;
}

static bool provider_fields(const InfiltratrTemporalCalendarProvider *self,
                            int64_t jdn,
                            InfiltratrTemporalCalendarFields *fields)
{
    (void)self;
    return fields_at(jdn, fields);
}

static bool provider_format(const InfiltratrTemporalCalendarProvider *self,
                            int64_t jdn,
                            InfiltratrTemporalCalendarDatePart part,
                            char *buffer, size_t capacity, size_t *length)
{
    GDate date;
    char month[128];
    char weekday[128];
    int count;
    int64_t year;

    (void)self;
    if (buffer == NULL || capacity == 0U || length == NULL ||
        !date_at(jdn, &date))
        return false;
    year = thai_year(&date);

    if (g_date_strftime(month, sizeof(month), "%B", &date) == 0U ||
        g_date_strftime(weekday, sizeof(weekday), "%A", &date) == 0U)
        return false;

    switch (part)
    {
        case INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_DAY:
            count = snprintf(buffer, capacity, "%u",
                             (unsigned)g_date_get_day(&date));
            break;
        case INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_MONTH:
            count = snprintf(buffer, capacity, "%s", month);
            break;
        case INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_YEAR:
            count = snprintf(buffer, capacity, "%" G_GINT64_FORMAT, year);
            break;
        case INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_SHORT:
            count = snprintf(buffer, capacity, "%u %s %" G_GINT64_FORMAT " B.E.",
                             (unsigned)g_date_get_day(&date), month, year);
            break;
        case INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_FULL:
            count = snprintf(buffer, capacity,
                             "%s, %u %s %" G_GINT64_FORMAT " B.E.",
                             weekday, (unsigned)g_date_get_day(&date),
                             month, year);
            break;
        default:
            return false;
    }
    if (count <= 0 || (size_t)count >= capacity)
        return false;
    *length = (size_t)count;
    return true;
}

static bool provider_period_start(const InfiltratrTemporalCalendarProvider *self,
                                  int64_t jdn, int64_t *result)
{
    InfiltratrTemporalCalendarFields fields;

    (void)self;
    return fields_at(jdn, &fields) &&
        thai_to_jdn(fields.year, fields.month, 1, result);
}

static bool provider_add_periods(const InfiltratrTemporalCalendarProvider *self,
                                 int64_t jdn, int32_t amount, int64_t *result)
{
    GDate date;
    int64_t serial;
    int64_t year;
    int month;
    int day;
    GDate destination;

    (void)self;
    if (result == NULL || !date_at(jdn, &date))
        return false;
    serial = (int64_t)g_date_get_year(&date) * 12 +
             (int64_t)g_date_get_month(&date) - 1 + amount;
    year = serial / 12;
    month = (int)(serial % 12);
    if (month < 0)
    {
        month += 12;
        --year;
    }
    ++month;
    if (year < 1 || year > G_MAXUINT16)
        return false;
    day = MIN((int)g_date_get_day(&date),
              (int)g_date_get_days_in_month((GDateMonth)month,
                                             (GDateYear)year));
    g_date_clear(&destination, 1U);
    g_date_set_dmy(&destination, (GDateDay)day,
                   (GDateMonth)month, (GDateYear)year);
    return thai_to_jdn(thai_year(&destination), month, day, result);
}

static bool provider_add_years(const InfiltratrTemporalCalendarProvider *self,
                              int64_t jdn, int32_t amount, int64_t *result)
{
    InfiltratrTemporalCalendarFields fields;
    GDate date;
    int64_t year;
    int64_t gregorian_year;
    int day;

    (void)self;
    if (!fields_at(jdn, &fields) || result == NULL)
        return false;
    year = fields.year + amount;
    if (year < FIRST_THAI_YEAR ||
        (year == LAST_APRIL_YEAR && fields.month < 4))
        return false;
    gregorian_year = year -
        ((year < FIRST_JANUARY_YEAR && fields.month < 4) ? 542 : 543);
    if (gregorian_year < 1 || gregorian_year > G_MAXUINT16)
        return false;
    day = MIN(fields.day, (int)g_date_get_days_in_month(
        (GDateMonth)fields.month, (GDateYear)gregorian_year));
    g_date_clear(&date, 1U);
    g_date_set_dmy(&date, (GDateDay)day, (GDateMonth)fields.month,
                   (GDateYear)gregorian_year);
    return thai_to_jdn(year, fields.month, day, result);
}

static const InfiltratrTemporalResourceMetadata metadata = {
    sizeof(InfiltratrTemporalResourceMetadata),
    INFILTRATR_TEMPORAL_RESOURCE_METADATA_ABI,
    INFILTRATR_TEMPORAL_RESOURCE_CALENDAR,
    "thai-buddhist-solar", INFILTRATR_TEMPORAL_DISPLAY_NAME,
    false, false, false
};
static const InfiltratrTemporalCalendarProvider provider = {
    sizeof(InfiltratrTemporalCalendarProvider),
    INFILTRATR_TEMPORAL_CALENDAR_PROVIDER_ABI,
    "thai-buddhist-solar",
    provider_fields, provider_format, provider_period_start,
    provider_add_periods, provider_add_years
};

/* The public ABI entry points are externally visible and must have
 * declarations before their definitions under strict -Wmissing-prototypes. */
const InfiltratrTemporalResourceMetadata *
infiltratr_temporal_resource_metadata_v1(void);
const InfiltratrTemporalCalendarProvider *
infiltratr_temporal_calendar_provider_v1(void);

#if defined(_WIN32)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif
EXPORT const InfiltratrTemporalResourceMetadata *
infiltratr_temporal_resource_metadata_v1(void) { return &metadata; }
EXPORT const InfiltratrTemporalCalendarProvider *
infiltratr_temporal_calendar_provider_v1(void) { return &provider; }
