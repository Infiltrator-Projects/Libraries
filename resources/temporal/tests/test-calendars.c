// SPDX-License-Identifier: GPL-3.0-or-later
/* Exercise every implementation through Common's public resource ABI. */
#include <infiltratr/temporal_calendar.h>
#include <glib.h>
#include <string.h>

#include "resource-test-path.h"
static gchar *test_calendar_path(const gchar *id)
{ return resource_test_path(TEST_CALENDAR_RESOURCE_DIRECTORY, INFILTRATR_TEMPORAL_RESOURCE_CALENDAR, id); }
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
    g_assert_cmpuint(count, ==, 30U);
    g_assert_null(infiltratr_temporal_calendar_module_open("/missing/calendar.so"));
    g_strfreev(ids);
    g_free(catalogue);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/resources/all-calendar-providers", test_catalogue);
    return g_test_run();
}
