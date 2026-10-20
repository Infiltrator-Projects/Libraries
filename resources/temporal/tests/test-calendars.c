// SPDX-License-Identifier: GPL-3.0-or-later
/* Exercise every implementation through Common's public resource ABI. */
#include <infiltratr/temporal_calendar.h>
#include <infiltratr/temporal_runtime.h>
#include <glib.h>
#include <string.h>

#include "resource-test-path.h"
static gchar *test_calendar_filename(const gchar *id)
{ return resource_test_filename(TEST_CALENDAR_RESOURCE_DIRECTORY, INFILTRATR_TEMPORAL_RESOURCE_CALENDAR, id); }

#ifndef TEST_CALENDAR_RESOURCE_DIRECTORY
#error TEST_CALENDAR_RESOURCE_DIRECTORY is required
#endif

static void test_catalogue(void)
{
    gchar *catalogue = NULL;
    gchar **ids;
    guint count = 0U;
    g_assert_true(g_file_get_contents("src/calendars/catalogue.txt", &catalogue, NULL, NULL));
    ids = g_strsplit(catalogue, "\n", -1);
    for (guint i = 0U; ids[i] != NULL; ++i)
    {
        InfiltratrTemporalCalendarModule *module;
        const InfiltratrTemporalResourceMetadata *metadata;
        InfiltratrTemporalCalendarFields fields;
        gint64 result = 0;
        gchar *filename;
        gchar *path;
        if (ids[i][0] == '\0') continue;
        filename = test_calendar_filename(ids[i]);
        path = g_build_filename(TEST_CALENDAR_RESOURCE_DIRECTORY, filename, NULL);
        g_test_message("calendar provider: %s", ids[i]);
        module = infiltratr_temporal_calendar_module_open(path);
        g_assert_nonnull(module);
        metadata = infiltratr_temporal_calendar_module_metadata(module);
        g_assert_nonnull(metadata);
        g_assert_cmpstr(metadata->id, ==, ids[i]);
        g_assert_true(infiltratr_temporal_calendar_fields_from_jdn(module, 2451545, &fields));
        if (strcmp(ids[i], "gregorian") == 0)
        {
            g_assert_cmpint(fields.year, ==, 2000);
            g_assert_cmpint(fields.month, ==, 1);
            g_assert_cmpint(fields.day, ==, 1);
        }
        for (int part = INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_DAY;
             part <= INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_FULL; ++part)
        {
            char text[1024] = {0};
            size_t length = 0U;
            g_assert_true(infiltratr_temporal_calendar_format(module, 2451545,
                (InfiltratrTemporalCalendarDatePart)part, text, sizeof(text), &length));
            g_assert_cmpuint(length, >, 0U);
            g_assert_cmpuint(length, ==, strlen(text));
            g_assert_true(g_utf8_validate(text, -1, NULL));
        }
        g_assert_true(infiltratr_temporal_calendar_period_start(module, 2451545, &result));
        g_assert_cmpint(result, <=, 2451545);
        g_assert_true(infiltratr_temporal_calendar_add_periods(module, 2451545, 0, &result));
        g_assert_cmpint(result, ==, 2451545);
        g_assert_true(infiltratr_temporal_calendar_add_years(module, 2451545, 0, &result));
        g_assert_cmpint(result, ==, 2451545);
        infiltratr_temporal_calendar_module_free(module);
        g_free(path);
        g_free(filename);
        ++count;
    }
    g_assert_cmpuint(count, ==, 31U);
    g_assert_null(infiltratr_temporal_calendar_module_open("/missing/calendar.so"));
    g_strfreev(ids);
    g_free(catalogue);
}

/*
 * Independently attested chronology anchors.
 *
 * These are not round-trip tests: comparing a provider against its own
 * inverse would preserve the same systematic error on both sides.  The
 * Gregorian dates below are converted to integer JDNs independently of the
 * provider under test.  Distinct regional/historical conventions must have
 * separate stable IDs rather than silently changing these contracts.
 *
 * Evidence: the French Republican year-start concordance (Archives de Lyon);
 * Hebcal's 1 Tishrei 5784; FAMSI's GMT Maya correlation; Swedish 1712 civil
 * reform; ISO 8601 week-year rules; and the Indian National Calendar epoch.
 */
static void test_historical_chronology_anchors(void)
{
    static const struct {
        const char *id;
        gint64 jdn;
        gint64 year;
        gint32 month;
        gint32 day;
        gboolean check_special;
        gboolean special;
        gboolean check_auxiliary;
        gint32 auxiliary;
    } vectors[] = {
        { "gregorian",          2451545, 2000,  1,  1, FALSE, FALSE, FALSE, 0 },
        { "julian",             2451545, 1999, 12, 19, FALSE, FALSE, FALSE, 0 },
        { "revised-julian",     2451545, 2000,  1,  1, FALSE, FALSE, FALSE, 0 },
        { "iso-week",           2451545, 1999, 52,  6, FALSE, FALSE, FALSE, 0 },
        { "islamic-civil",      1948440,    1,  1,  1, FALSE, FALSE, FALSE, 0 },
        { "islamic-tbla",       1948439,    1,  1,  1, FALSE, FALSE, FALSE, 0 },
        { "hebrew",             2460204, 5784,  1,  1, FALSE, FALSE, FALSE, 0 },
        { "persian",            2460390, 1403,  1,  1, FALSE, FALSE, FALSE, 0 },
        { "buddhist",           2451545, 2543,  1,  1, FALSE, FALSE, FALSE, 0 },
        { "thai-buddhist-solar", 2419494, 2455,  4,  1, FALSE, FALSE, FALSE, 0 },
        { "thai-buddhist-solar", 2429720, 2482,  3, 31, FALSE, FALSE, FALSE, 0 },
        { "thai-buddhist-solar", 2429721, 2483,  4,  1, FALSE, FALSE, FALSE, 0 },
        { "thai-buddhist-solar", 2429995, 2483, 12, 31, FALSE, FALSE, FALSE, 0 },
        { "thai-buddhist-solar", 2429996, 2484,  1,  1, FALSE, FALSE, FALSE, 0 },
        { "minguo",             2419403,    1,  1,  1, FALSE, FALSE, FALSE, 0 },
        { "japanese",           2458605,    1,  5,  1, FALSE, FALSE, TRUE, 236 },
        { "indian",             2460391, 1946,  1,  1, FALSE, FALSE, FALSE, 0 },
        { "french-republican",  2375840,    1,  1,  1, FALSE, FALSE, FALSE, 0 },
        { "french-republican",  2379858,   12,  1,  1, FALSE, FALSE, FALSE, 0 },
        { "swedish-historical", 2346425, 1712,  2, 30, FALSE, FALSE, FALSE, 0 },
        { "mayan",              2456283, 5200,  0,  0, FALSE, FALSE, FALSE, 0 },
        { "international-fixed",2460479, 2024,  6, 29, TRUE, TRUE, FALSE, 0 },
        { "world",              2460676, 2024, 12, 31, TRUE, TRUE, FALSE, 0 },
        { "positivist",         2374480,    1,  1,  1, FALSE, FALSE, FALSE, 0 }
    };

    for (gsize i = 0; i < G_N_ELEMENTS(vectors); ++i)
    {
        g_autofree gchar *path = test_calendar_filename(vectors[i].id);
        g_autofree gchar *full_path =
            g_build_filename(TEST_CALENDAR_RESOURCE_DIRECTORY, path, NULL);
        InfiltratrTemporalCalendarModule *module =
            infiltratr_temporal_calendar_module_open(full_path);
        InfiltratrTemporalCalendarFields fields = {0};

        g_test_message("chronology anchor: %s at JDN %" G_GINT64_FORMAT,
                       vectors[i].id, vectors[i].jdn);
        g_assert_nonnull(module);
        g_assert_true(infiltratr_temporal_calendar_fields_from_jdn(
            module, vectors[i].jdn, &fields));
        g_assert_cmpint(fields.year, ==, vectors[i].year);
        g_assert_cmpint(fields.month, ==, vectors[i].month);
        g_assert_cmpint(fields.day, ==, vectors[i].day);
        if (vectors[i].check_special)
            g_assert_cmpint(fields.special, ==, vectors[i].special);
        if (vectors[i].check_auxiliary)
            g_assert_cmpint(fields.auxiliary, ==, vectors[i].auxiliary);
        infiltratr_temporal_calendar_module_free(module);
    }
}

/* 2483 B.E. was a nine-month year: January-March never existed. */
static void test_thai_historical_missing_months(void)
{
    g_autofree gchar *path = test_calendar_filename("thai-buddhist-solar");
    g_autofree gchar *full_path =
        g_build_filename(TEST_CALENDAR_RESOURCE_DIRECTORY, path, NULL);
    InfiltratrTemporalCalendarModule *module =
        infiltratr_temporal_calendar_module_open(full_path);
    InfiltratrTemporalCalendarFields fields = {0};
    gint64 result = 0;

    g_assert_nonnull(module);
    g_assert_false(infiltratr_temporal_calendar_fields_from_jdn(
        module, INT64_C(2419493), &fields));
    g_assert_false(infiltratr_temporal_calendar_add_years(
        module, INT64_C(2429720), 1, &result));
    g_assert_true(infiltratr_temporal_calendar_add_periods(
        module, INT64_C(2429995), 1, &result));
    g_assert_true(infiltratr_temporal_calendar_fields_from_jdn(
        module, result, &fields));
    g_assert_cmpint(fields.year, ==, 2484);
    g_assert_cmpint(fields.month, ==, 1);
    g_assert_cmpint(fields.day, ==, 31);
    infiltratr_temporal_calendar_module_free(module);
}

/* Several readers race first-touch initialization. The catalogue must be
 * completely published before any thread sees optional entries. */
static gpointer catalogue_reader(gpointer user_data)
{
    gsize *observed_count = user_data;
    *observed_count = infiltratr_temporal_installed_calendar_count();
    for (gsize n = 0; n < *observed_count; ++n)
        g_assert_nonnull(infiltratr_temporal_installed_calendar_at(n));
    g_assert_cmpuint(infiltratr_temporal_installed_clock_mode_count(), >=, 1U);
    return NULL;
}

static void test_concurrent_catalogue_readers(void)
{
    GThread *threads[8];
    gsize counts[8] = { 0 };
    for (gsize i = 0; i < G_N_ELEMENTS(threads); ++i)
        threads[i] = g_thread_new("calendar-catalogue", catalogue_reader,
                                   &counts[i]);
    for (gsize i = 0; i < G_N_ELEMENTS(threads); ++i)
        g_thread_join(threads[i]);
    for (gsize i = 1; i < G_N_ELEMENTS(counts); ++i)
        g_assert_cmpuint(counts[i], ==, counts[0]);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/resources/all-calendar-providers", test_catalogue);
    g_test_add_func("/resources/00-concurrent-catalogue-initialization", test_concurrent_catalogue_readers);
    g_test_add_func("/resources/historical-chronology-anchors", test_historical_chronology_anchors);
    g_test_add_func("/resources/thai-historical-transition", test_thai_historical_missing_months);
    return g_test_run();
}
