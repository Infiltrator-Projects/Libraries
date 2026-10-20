// SPDX-License-Identifier: GPL-3.0-or-later
/* Preserved implementation tests moved with their engines into Common. */
#include "calendar-arithmetic.h"
#include "calendar-helpers.h"
#include "calendar-swedish.h"
#include "calendar-ancient.h"
#include "calendar-reform.h"
#include "calendar-historical.h"
#include "calendar-bahai.h"
#include "icu-calendar.h"
#include "julian-day.h"
#include "time-astronomy.h"
#include <glib.h>
#include <math.h>
#include "temporal_solar_compact.h"
static void
test_shared_calendar_helpers(void)
{
    g_assert_cmpint(calendar_gregorian_month_length(2000, 2), ==, 29);
    g_assert_cmpint(calendar_gregorian_month_length(1900, 2), ==, 28);
    g_assert_cmpint(calendar_julian_month_length(1900, 2), ==, 29);
    g_assert_cmpint(calendar_julian_month_length(1901, 2), ==, 28);

    g_assert_cmpint(calendar_count_multiples_inclusive(1, 12, 4), ==, 3);
    g_assert_cmpint(calendar_count_multiples_inclusive(-12, -1, 4), ==, 3);
    g_assert_cmpint(calendar_count_multiples_inclusive(-4, 4, 4), ==, 3);
    g_assert_cmpint(calendar_count_multiples_inclusive(5, 4, 4), ==, 0);

    /* Extreme public inputs must remain defined under ASan/UBSan. */
    (void)calendar_gregorian_to_jdn(G_MININT64, 1, 1);
    (void)calendar_gregorian_to_jdn(G_MAXINT64, 12, 31);
    (void)calendar_julian_to_jdn(G_MININT64, 1, 1);
    (void)calendar_julian_to_jdn(G_MAXINT64, 12, 31);
}

static void
test_arithmetic_calendar_engines(void)
{
    typedef struct
    {
        CalendarCalendarMode mode;
        gint64 jdn;
        gint64 year;
        gint month;
        gint day;
        gint auxiliary;
    } ArithmeticVector;

    static const ArithmeticVector vectors[] = {
        {
            CALENDAR_CALENDAR_MODE_GREGORIAN,
            1721060, 1, 1, 1, 0
        },
        {
            CALENDAR_CALENDAR_MODE_ISLAMIC_CIVIL,
            1948440, 1, 1, 1, 0
        },
        {
            CALENDAR_CALENDAR_MODE_ISLAMIC_UMM_AL_QURA,
            2408762, 1300, 1, 1, 0
        },
        {
            CALENDAR_CALENDAR_MODE_ISLAMIC_TBLA,
            1948439, 1, 1, 1, 0
        },
        {
            CALENDAR_CALENDAR_MODE_COPTIC,
            1825030, 1, 1, 1, 1
        },
        {
            CALENDAR_CALENDAR_MODE_COPTIC,
            1825029, 1, 13, 5, 0
        },
        {
            CALENDAR_CALENDAR_MODE_ETHIOPIAN,
            1724221, 1, 1, 1, 1
        },
        {
            CALENDAR_CALENDAR_MODE_ETHIOPIAN,
            1724220, 5500, 13, 5, 0
        },
        {
            CALENDAR_CALENDAR_MODE_ETHIOPIC_AMETE_ALEM,
            1724221, 5501, 1, 1, 0
        },
        {
            CALENDAR_CALENDAR_MODE_INDIAN,
            2461122, 1948, 1, 1, 0
        },
        {
            CALENDAR_CALENDAR_MODE_BUDDHIST,
            2461042, 2569, 1, 1, 0
        },
        {
            CALENDAR_CALENDAR_MODE_MINGUO,
            2419403, 1, 1, 1, 1
        },
        {
            CALENDAR_CALENDAR_MODE_JAPANESE,
            2458605, 1, 5, 1, 236
        }
    };
    gsize index;

    for (index = 0; index < G_N_ELEMENTS(vectors); index++)
    {
        CalendarCalendarFields fields = { 0 };

        g_assert_true(calendar_arithmetic_fields_from_jdn(
            vectors[index].mode, vectors[index].jdn, &fields));
        g_assert_cmpint(fields.year, ==, vectors[index].year);
        g_assert_cmpint(fields.month, ==, vectors[index].month);
        g_assert_cmpint(fields.day, ==, vectors[index].day);
        g_assert_cmpint(fields.auxiliary, ==, vectors[index].auxiliary);
        g_assert_false(fields.special);
    }

    {
        const gint64 leap_day =
            calendar_gregorian_to_jdn(2024, 2, 29);
        const gint64 next_year = calendar_arithmetic_add_years(
            CALENDAR_CALENDAR_MODE_GREGORIAN, leap_day, 1);

        g_assert_cmpint(
            next_year,
            ==,
            calendar_gregorian_to_jdn(2025, 2, 28));
    }

    {
        CalendarCalendarFields fields = { 0 };
        const gint64 next_month = calendar_arithmetic_add_months(
            CALENDAR_CALENDAR_MODE_ISLAMIC_CIVIL, 1948440, 1);

        g_assert_true(calendar_arithmetic_fields_from_jdn(
            CALENDAR_CALENDAR_MODE_ISLAMIC_CIVIL,
            next_month,
            &fields));
        g_assert_cmpint(fields.year, ==, 1);
        g_assert_cmpint(fields.month, ==, 2);
        g_assert_cmpint(fields.day, ==, 1);
    }

    {
        CalendarCalendarFields fields = { 0 };
        const gint64 date = calendar_gregorian_to_jdn(2026, 9, 20);
        const gint64 month_start = calendar_arithmetic_month_start(
            CALENDAR_CALENDAR_MODE_ETHIOPIAN, date);

        g_assert_true(calendar_arithmetic_fields_from_jdn(
            CALENDAR_CALENDAR_MODE_ETHIOPIAN,
            month_start,
            &fields));
        g_assert_cmpint(fields.year, ==, 2019);
        g_assert_cmpint(fields.month, ==, 1);
        g_assert_cmpint(fields.day, ==, 1);
    }

    {
        CalendarCalendarFields fields = { 0 };
        const gint64 first = G_GINT64_CONSTANT(2408762);
        const gint64 last_boundary =
            calendar_i64_add_saturating(first, 106665);
        const gint64 last_year = calendar_i64_add_saturating(
            first, 106311);

        g_assert_false(calendar_arithmetic_fields_from_jdn(
            CALENDAR_CALENDAR_MODE_ISLAMIC_UMM_AL_QURA,
            first - 1,
            &fields));
        g_assert_false(calendar_arithmetic_fields_from_jdn(
            CALENDAR_CALENDAR_MODE_ISLAMIC_UMM_AL_QURA,
            last_boundary,
            &fields));

        g_assert_true(calendar_arithmetic_fields_from_jdn(
            CALENDAR_CALENDAR_MODE_ISLAMIC_UMM_AL_QURA,
            last_year,
            &fields));
        g_assert_cmpint(fields.year, ==, 1600);
        g_assert_cmpint(fields.month, ==, 1);
        g_assert_cmpint(fields.day, ==, 1);

        g_assert_cmpint(
            calendar_arithmetic_add_years(
                CALENDAR_CALENDAR_MODE_ISLAMIC_UMM_AL_QURA,
                last_year,
                1),
            ==,
            G_MAXINT64);
    }
}

static void
test_swedish_historical_boundaries(void)
{
    CalendarCalendarFields fields = { 0 };
    CalendarCalendarFields date;
    gint64 jdn;
    g_autofree gchar *formatted = NULL;

    /* Julian before the failed gradual reform. */
    jdn = calendar_julian_to_jdn(1699, 12, 31);
    calendar_swedish_fields_from_jdn(jdn, &fields);
    g_assert_cmpint(fields.year, ==, 1699);
    g_assert_cmpint(fields.month, ==, 12);
    g_assert_cmpint(fields.day, ==, 31);
    g_assert_false(fields.special);

    /* Swedish style: one displayed civil day ahead of Julian. */
    jdn = calendar_gregorian_to_jdn(1700, 3, 11);
    calendar_swedish_fields_from_jdn(jdn, &fields);
    g_assert_cmpint(fields.year, ==, 1700);
    g_assert_cmpint(fields.month, ==, 3);
    g_assert_cmpint(fields.day, ==, 1);

    /* The unique restoration day must round-trip exactly. */
    jdn = calendar_julian_to_jdn(1712, 2, 29);
    calendar_swedish_fields_from_jdn(jdn, &fields);
    g_assert_cmpint(fields.year, ==, 1712);
    g_assert_cmpint(fields.month, ==, 2);
    g_assert_cmpint(fields.day, ==, 30);
    g_assert_true(fields.special);
    g_assert_cmpint(calendar_swedish_fields_to_jdn(&fields), ==, jdn);

    /* After 30 February Sweden returned to ordinary Julian dating. */
    jdn = calendar_julian_to_jdn(1712, 3, 1);
    calendar_swedish_fields_from_jdn(jdn, &fields);
    g_assert_cmpint(fields.year, ==, 1712);
    g_assert_cmpint(fields.month, ==, 3);
    g_assert_cmpint(fields.day, ==, 1);

    /* Gregorian adoption skipped directly from 17 February to 1 March. */
    jdn = calendar_gregorian_to_jdn(1753, 3, 1);
    calendar_swedish_fields_from_jdn(jdn, &fields);
    g_assert_cmpint(fields.year, ==, 1753);
    g_assert_cmpint(fields.month, ==, 3);
    g_assert_cmpint(fields.day, ==, 1);

    date = (CalendarCalendarFields){ .year = 1700, .month = 2, .day = 1 };
    g_assert_cmpint(calendar_swedish_month_length(&date), ==, 28);
    date = (CalendarCalendarFields){ .year = 1712, .month = 2, .day = 1 };
    g_assert_cmpint(calendar_swedish_month_length(&date), ==, 30);
    date = (CalendarCalendarFields){ .year = 1753, .month = 2, .day = 1 };
    g_assert_cmpint(calendar_swedish_month_length(&date), ==, 17);
    date = (CalendarCalendarFields){ .year = 1704, .month = 2, .day = 1 };
    g_assert_cmpint(calendar_swedish_month_length(&date), ==, 29);
    date = (CalendarCalendarFields){ .year = 1800, .month = 2, .day = 1 };
    g_assert_cmpint(calendar_swedish_month_length(&date), ==, 28);
    date = (CalendarCalendarFields){ .year = 1804, .month = 2, .day = 1 };
    g_assert_cmpint(calendar_swedish_month_length(&date), ==, 29);
    date = (CalendarCalendarFields){ .year = 1804, .month = 4, .day = 1 };
    g_assert_cmpint(calendar_swedish_month_length(&date), ==, 30);
    date = (CalendarCalendarFields){ .year = 1804, .month = 1, .day = 1 };
    g_assert_cmpint(calendar_swedish_month_length(&date), ==, 31);

    date = (CalendarCalendarFields){ .year = 1712, .month = 2, .day = 30,
                                         .special = TRUE };
    formatted = calendar_swedish_format(&date, CALENDAR_DATE_PART_SHORT);
    g_assert_cmpstr(formatted, ==, "30 February 1712");
    g_clear_pointer(&formatted, g_free);
    formatted = calendar_swedish_format(&date, CALENDAR_DATE_PART_DAY);
    g_assert_cmpstr(formatted, ==, "30");
    g_clear_pointer(&formatted, g_free);
    formatted = calendar_swedish_format(&date, CALENDAR_DATE_PART_MONTH);
    g_assert_cmpstr(formatted, ==, "February");
    g_clear_pointer(&formatted, g_free);
    formatted = calendar_swedish_format(&date, CALENDAR_DATE_PART_YEAR);
    g_assert_cmpstr(formatted, ==, "1712");

    date = (CalendarCalendarFields){ .year = 1712, .month = 1, .day = 30 };
    jdn = calendar_swedish_fields_to_jdn(&date);
    calendar_swedish_fields_from_jdn(
        calendar_swedish_add_months(jdn, 1), &fields);
    g_assert_cmpint(fields.month, ==, 2);
    g_assert_cmpint(fields.day, ==, 30);
    g_assert_true(fields.special);

    date = (CalendarCalendarFields){ .year = 1753, .month = 1, .day = 31 };
    jdn = calendar_swedish_fields_to_jdn(&date);
    calendar_swedish_fields_from_jdn(
        calendar_swedish_add_months(jdn, 1), &fields);
    g_assert_cmpint(fields.month, ==, 2);
    g_assert_cmpint(fields.day, ==, 17);

    date = (CalendarCalendarFields){ .year = 1712, .month = 2, .day = 30,
                                         .special = TRUE };
    jdn = calendar_swedish_fields_to_jdn(&date);
    calendar_swedish_fields_from_jdn(
        calendar_swedish_add_years(jdn, 1), &fields);
    g_assert_cmpint(fields.year, ==, 1713);
    g_assert_cmpint(fields.month, ==, 2);
    g_assert_cmpint(fields.day, ==, 28);

    date = (CalendarCalendarFields){ .year = 1753, .month = 2, .day = 17 };
    jdn = calendar_swedish_fields_to_jdn(&date);
    g_assert_cmpint(calendar_swedish_month_start(jdn), ==,
                    calendar_swedish_fields_to_jdn(
                        &(CalendarCalendarFields){
                            .year = 1753, .month = 2, .day = 1 }));
}

static void
assert_gregorian_round_trip(gint year,
                            gint month,
                            gint day)
{
    const gint64 jdn = calendar_gregorian_to_jdn(year, month, day);
    gint actual_year = 0;
    gint actual_month = 0;
    gint actual_day = 0;

    calendar_jdn_to_gregorian(jdn,
                                   &actual_year,
                                   &actual_month,
                                   &actual_day);
    g_assert_cmpint(actual_year, ==, year);
    g_assert_cmpint(actual_month, ==, month);
    g_assert_cmpint(actual_day, ==, day);
}

static void
assert_julian_round_trip(gint year,
                        gint month,
                        gint day)
{
    const gint64 jdn = calendar_julian_to_jdn(year, month, day);
    gint actual_year = 0;
    gint actual_month = 0;
    gint actual_day = 0;

    calendar_jdn_to_julian(jdn,
                                &actual_year,
                                &actual_month,
                                &actual_day);
    g_assert_cmpint(actual_year, ==, year);
    g_assert_cmpint(actual_month, ==, month);
    g_assert_cmpint(actual_day, ==, day);
}

static void
test_solar_instant_overflow_rejection(void)
{
    gint64 dawn = 0;
    gint64 dusk = 0;

    /*
     * The instant API must reject values whose containing solar day cannot be
     * represented without overflow; it must never silently clamp them.
     */
    g_assert_false(calendar_solar_boundary_instants(
        G_MININT64, -36.3833, 145.4000, 0.833, &dawn, &dusk));
    g_assert_false(calendar_solar_boundary_instants(
        G_MAXINT64, -36.3833, 145.4000, 0.833, &dawn, &dusk));

    /* A normal mid-latitude date remains representable and ordered. */
    g_assert_true(calendar_solar_boundary_instants(
        1788177600LL * G_USEC_PER_SEC,
        -36.3833, 145.4000, 0.833, &dawn, &dusk));
    g_assert_cmpint(dawn, <, dusk);
}

static void
test_complete_civil_year_domain(void)
{
    assert_gregorian_round_trip(G_MININT, 1, 1);
    assert_gregorian_round_trip(G_MININT, 12, 31);
    assert_gregorian_round_trip(G_MAXINT, 1, 1);
    assert_gregorian_round_trip(G_MAXINT, 12, 31);

    assert_julian_round_trip(G_MININT, 1, 1);
    assert_julian_round_trip(G_MININT, 12, 31);
    assert_julian_round_trip(G_MAXINT, 1, 1);
    assert_julian_round_trip(G_MAXINT, 12, 31);
}

static void
test_roman_extreme_format_is_bounded(void)
{
    g_autofree gchar *large = calendar_roman_number(G_MAXINT);

    g_assert_cmpstr(large, ==, "2147483647");
}
/* The private solar projection handles pre-epoch days without a negative
 * UTC fraction and reproduces exact Gregorian leap-year ordinals. */
static void test_compact_solar_contract(void)
{
    int ordinal = 0;
    double equation = 0.0, declination = 0.0;
    g_assert_true(temporal_solar_day_of_year(
        INT64_C(1709164800000000), &ordinal)); /* 2024-02-29 UTC */
    g_assert_cmpint(ordinal, ==, 60);
    g_assert_true(temporal_solar_day_of_year(-INT64_C(1000000), &ordinal));
    g_assert_cmpint(ordinal, ==, 365); /* 1969-12-31 UTC */
    g_assert_true(temporal_solar_compact_terms(
        INT64_C(1709164800000000), &equation, &declination));
    g_assert_true(isfinite(equation));
    g_assert_true(isfinite(declination));
    g_assert_cmpfloat(fabs(equation), <, 30.0);
    g_assert_cmpfloat(fabs(declination), <, 0.5);
    g_assert_true(temporal_solar_compact_terms(
        G_MININT64, &equation, &declination));
    g_assert_true(isfinite(equation));
    g_assert_true(isfinite(declination));
}

/* Direct engine callers must also terminate on impossible numerical input;
 * the resource ABI's refusal alone is not enough to prevent an infinite loop. */
/* The private Maya converter is also directly callable by engine tests.
 * An unrepresentable epoch subtraction must not execute signed overflow. */
static void test_mayan_extreme_arithmetic(void)
{
    CalendarCalendarFields fields = { .year = 42 };
    calendar_mayan_fields_from_jdn(G_MININT64, &fields);
    g_assert_cmpint(fields.year, ==, 0);
    g_assert_cmpint(fields.month, ==, 0);
    g_assert_cmpint(fields.day, ==, 0);
    calendar_mayan_fields_from_jdn(INT64_C(584283), &fields);
    g_assert_cmpint(fields.year, ==, 0);
    g_assert_cmpint(fields.month, ==, 0);
    g_assert_cmpint(fields.day, ==, 0);
}

static void test_correction_loops_are_bounded(void)
{
    CalendarCalendarFields fields = {0};
    calendar_french_fields_from_jdn(G_MININT64, &fields);
    g_assert_cmpint(fields.month, ==, 0);
    calendar_french_fields_from_jdn(G_MAXINT64, &fields);
    g_assert_cmpint(fields.month, ==, 0);
    calendar_revised_julian_fields_from_jdn(G_MININT64, &fields);
    g_assert_cmpint(fields.month, ==, 0);
    calendar_revised_julian_fields_from_jdn(G_MAXINT64, &fields);
    g_assert_cmpint(fields.month, ==, 0);
}

/* ICU has a finite UDate horizon. Every operation must reject impossible
 * JDNs before subtraction, conversion, navigation, or text generation. */
static void test_icu_finite_dates_and_failure_outputs(void)
{
    const gint64 invalid[] = {
        G_MININT64, G_MAXINT64,
        CALENDAR_UNIX_EPOCH_JDN - G_GINT64_CONSTANT(100000001),
        CALENDAR_UNIX_EPOCH_JDN + G_GINT64_CONSTANT(100000000)
    };
    for (gsize i = 0; i < G_N_ELEMENTS(invalid); ++i)
    {
        CalendarCalendarFields fields = { .year = 17, .month = 23 };
        g_autofree gchar *label = NULL;
        g_assert_false(calendar_icu_fields_from_jdn(
            "gregorian", invalid[i], &fields));
        g_assert_cmpint(fields.year, ==, 17);
        g_assert_cmpint(fields.month, ==, 23);
        label = calendar_icu_format(CALENDAR_CALENDAR_MODE_GREGORIAN,
                                    "gregorian", invalid[i],
                                    CALENDAR_DATE_PART_FULL);
        g_assert_cmpstr(label, ==, "");
        g_assert_cmpint(calendar_icu_month_start(
            "gregorian", invalid[i]), ==, G_MAXINT64);
        g_assert_cmpint(calendar_icu_add_months(
            "gregorian", invalid[i], 1), ==, G_MAXINT64);
        g_assert_cmpint(calendar_icu_add_years(
            "gregorian", invalid[i], -1), ==, G_MAXINT64);
    }

    CalendarCalendarFields epoch = { 0 };
    g_assert_true(calendar_icu_fields_from_jdn(
        "gregorian", CALENDAR_UNIX_EPOCH_JDN, &epoch));
    g_assert_cmpint(epoch.year, ==, 1970);
    g_assert_cmpint(epoch.month, ==, 1);
    g_assert_cmpint(epoch.day, ==, 1);
}

static void test_non_icu_correction_domain(void)
{
    const gint64 samples[] = {
        -G_GINT64_CONSTANT(1000000001),
        G_GINT64_CONSTANT(1000000001),
        -G_GINT64_CONSTANT(10000000000),
        G_GINT64_CONSTANT(10000000000)
    };
    for (gsize i = 0; i < G_N_ELEMENTS(samples); ++i)
    {
        CalendarCalendarFields fields = {0};
        calendar_french_fields_from_jdn(samples[i], &fields);
        g_assert_cmpint(fields.month, >=, 1);
        g_assert_cmpint(fields.day, >=, 1);
        g_assert_cmpint(calendar_french_fields_to_jdn(&fields),
                        ==, samples[i]);
        calendar_revised_julian_fields_from_jdn(samples[i], &fields);
        g_assert_cmpint(fields.month, >=, 1);
        g_assert_cmpint(fields.day, >=, 1);
        g_assert_cmpint(calendar_revised_julian_fields_to_jdn(&fields),
                        ==, samples[i]);
    }
    CalendarCalendarFields fields = {0};
    const gint64 beyond = (G_GINT64_CONSTANT(1) << 52) + 1;
    calendar_french_fields_from_jdn(beyond, &fields);
    g_assert_cmpint(fields.month, ==, 0);
    calendar_revised_julian_fields_from_jdn(-beyond, &fields);
    g_assert_cmpint(fields.month, ==, 0);
}

/*
 * Civil conversion returns gint years. The resource must validate input JDN
 * before narrowing; exact endpoint checks protect the limit computation.
 */
static void test_checked_civil_jdn_bounds(void)
{
    const gint64 gmin = calendar_gregorian_to_jdn(G_MININT, 1, 1);
    const gint64 gmax = calendar_gregorian_to_jdn(G_MAXINT, 12, 31);
    const gint64 jmin = calendar_julian_to_jdn(G_MININT, 1, 1);
    const gint64 jmax = calendar_julian_to_jdn(G_MAXINT, 12, 31);
    g_assert_true(calendar_gregorian_jdn_representable(gmin));
    g_assert_true(calendar_gregorian_jdn_representable(gmax));
    g_assert_false(calendar_gregorian_jdn_representable(gmin - 1));
    g_assert_false(calendar_gregorian_jdn_representable(gmax + 1));
    g_assert_true(calendar_julian_jdn_representable(jmin));
    g_assert_true(calendar_julian_jdn_representable(jmax));
    g_assert_false(calendar_julian_jdn_representable(jmin - 1));
    g_assert_false(calendar_julian_jdn_representable(jmax + 1));
    g_assert_false(calendar_iso_week_jdn_representable(G_MININT64));
    g_assert_false(calendar_iso_week_jdn_representable(G_MAXINT64));
}

/* An impossible Badíʿ year must not resolve to Naw-Rúz 1844. The private
 * engine uses the same explicit invalid-date result as its resource adapter. */
static void test_bahai_rejects_invalid_year(void)
{
    CalendarCalendarFields fields = { .year = G_MAXINT64, .month = 1, .day = 1 };
    const gint64 epoch = calendar_gregorian_to_jdn(1844, 3, 21);
    g_assert_cmpint(calendar_bahai_intercalary_days(G_MAXINT64), ==, 0);
    g_assert_cmpint(calendar_bahai_intercalary_days(G_MININT64), ==, 0);
    g_assert_cmpint(calendar_bahai_fields_to_jdn(&fields), ==, G_MAXINT64);
    g_assert_cmpint(calendar_bahai_fields_to_jdn(&fields), !=, epoch);
    calendar_bahai_fields_from_jdn(G_MAXINT64, &fields);
    g_assert_cmpint(fields.month, ==, 0);
    g_assert_cmpint(fields.day, ==, 0);
    calendar_bahai_fields_from_jdn(G_MININT64, &fields);
    g_assert_cmpint(fields.day, ==, 0);
    /* Legitimate intercalary days must not be mistaken for the invalid
     * sentinel simply because the Badíʿ month number is zero. */
    calendar_bahai_fields_from_jdn(calendar_gregorian_to_jdn(2024, 2, 28), &fields);
    g_assert_cmpint(fields.day, >=, 1);
    g_assert_cmpint(calendar_bahai_fields_to_jdn(&fields), ==,
                    calendar_gregorian_to_jdn(2024, 2, 28));
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_setenv("LANGUAGE", "en_US", TRUE);
    g_test_add_func("/resource-engine/shared-calendar-helpers", test_shared_calendar_helpers);
    g_test_add_func("/resource-engine/arithmetic-calendar-engines", test_arithmetic_calendar_engines);
    g_test_add_func("/resource-engine/swedish-historical-boundaries", test_swedish_historical_boundaries);
    g_test_add_func("/resource-engine/solar-instant-overflow-rejection", test_solar_instant_overflow_rejection);
    g_test_add_func("/resource-engine/compact-solar-contract", test_compact_solar_contract);
    g_test_add_func("/resource-engine/bounded-extreme-corrections", test_correction_loops_are_bounded);
    g_test_add_func("/resource-engine/mayan-extreme-arithmetic", test_mayan_extreme_arithmetic);
    g_test_add_func("/resource-engine/icu-date-domain", test_icu_finite_dates_and_failure_outputs);
    g_test_add_func("/resource-engine/independent-correction-domain", test_non_icu_correction_domain);
    g_test_add_func("/resource-engine/complete-civil-year-domain", test_complete_civil_year_domain);
    g_test_add_func("/resource-engine/checked-civil-jdn-bounds", test_checked_civil_jdn_bounds);
    g_test_add_func("/resource-engine/bahai-invalid-year-refusal", test_bahai_rejects_invalid_year);
    g_test_add_func("/resource-engine/roman-extreme-format-is-bounded", test_roman_extreme_format_is_bounded);
    return g_test_run();
}
