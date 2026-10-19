// SPDX-License-Identifier: GPL-3.0-or-later
/* Resolve module paths through their stable metadata IDs, not file stems. */
#ifndef INFILTRATOR_RESOURCE_TEST_PATH_H
#define INFILTRATOR_RESOURCE_TEST_PATH_H
#include <infiltratr/temporal_runtime.h>
#include <glib.h>
#include <string.h>
typedef struct ResourceTestLookup {
    const gchar *id;
    gchar *path;
    gboolean incorrect_filename;
} ResourceTestLookup;
static inline bool resource_test_capture(const InfiltratrTemporalResourceMetadata *meta,
                                  const char *path, void *user_data)
{
    ResourceTestLookup *lookup = user_data;
    if (g_strcmp0(meta->id, lookup->id) != 0) return true;
    gchar *base = g_path_get_basename(path);
    gchar *expected = g_strconcat(meta->name, ".so", NULL);
    if (g_strcmp0(base, expected) != 0) lookup->incorrect_filename = TRUE;
    g_free(base);
    g_free(expected);
    if (lookup->path == NULL) lookup->path = g_strdup(path);
    return true;
}
static inline gchar *resource_test_path(const gchar *directory,
                                 InfiltratrTemporalResourceKind kind,
                                 const gchar *id)
{
    ResourceTestLookup lookup = {id, NULL, FALSE};
    size_t visited = 0U;
    g_assert_true(infiltratr_temporal_runtime_scan_directory(
        directory, kind, resource_test_capture, &lookup, &visited));
    g_assert_false(lookup.incorrect_filename);
    g_assert_nonnull(lookup.path);
    return lookup.path;
}
static inline gchar *resource_test_filename(const gchar *directory,
                                     InfiltratrTemporalResourceKind kind,
                                     const gchar *id)
{
    gchar *path = resource_test_path(directory, kind, id);
    gchar *basename = g_path_get_basename(path);
    g_free(path);
    return basename;
}
#endif
