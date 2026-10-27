// SPDX-License-Identifier: GPL-3.0-or-later
#include "infiltratr/temporal_runtime.h"
#include "infiltratr/temporal_calendar.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifndef TEMPORAL_RUNTIME_FIXTURE_PATH
#error "TEMPORAL_RUNTIME_FIXTURE_PATH must name the runtime metadata fixture"
#endif

static bool capture_fixture(
    const InfiltratrTemporalResourceMetadata *metadata,
    const char *module_path,
    void *user_data)
{
    size_t *seen = user_data;

    assert(metadata != NULL);
    assert(module_path != NULL && module_path[0] != '\0');
    assert(metadata->struct_size >= sizeof(*metadata));
    assert(metadata->abi_version == INFILTRATR_TEMPORAL_RESOURCE_METADATA_ABI);
    assert(metadata->kind == INFILTRATR_TEMPORAL_RESOURCE_CALENDAR);
    assert(strcmp(metadata->id, "fixture-calendar") == 0);
    assert(strcmp(metadata->name, "Fixture Calendar") == 0);
    (*seen)++;
    return true;
}

static void fixture_directory(char *directory, size_t capacity)
{
    const char *path = TEMPORAL_RUNTIME_FIXTURE_PATH;
    const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');
    const char *separator = slash;
    size_t length;

    if (backslash != NULL && (separator == NULL || backslash > separator)) {
        separator = backslash;
    }
    assert(separator != NULL);
    length = (size_t)(separator - path);
    assert(length != 0U && length < capacity);
    memcpy(directory, path, length);
    directory[length] = '\0';
}

int main(void)
{
    char directory[1024];
    size_t seen = 0U;
    size_t visited = 99U;

    fixture_directory(directory, sizeof(directory));
    assert(infiltratr_temporal_runtime_scan_directory(
        directory,
        INFILTRATR_TEMPORAL_RESOURCE_CALENDAR,
        capture_fixture,
        &seen,
        &visited));
    assert(seen == 1U);
    assert(visited == 1U);
    /* Metadata-only fixtures remain visible to the public scan, but are not
     * executable calendar modules without the separately validated ABI. */
    assert(infiltratr_temporal_calendar_module_open(
        TEMPORAL_RUNTIME_FIXTURE_PATH) == NULL);

    visited = 99U;
    assert(infiltratr_temporal_runtime_scan_directory(
        directory,
        INFILTRATR_TEMPORAL_RESOURCE_CLOCK,
        NULL,
        NULL,
        &visited));
    assert(visited == 0U);

    assert(infiltratr_temporal_installed_calendar_count() >=
           infiltratr_temporal_calendar_count());
    assert(infiltratr_temporal_installed_clock_mode_count() >= 1U);

    /*
     * Resource-only calendars are deliberately absent from Common's compiled
     * base catalogue. The installed-runtime API may still find any of these
     * IDs when the host genuinely has the corresponding module installed.
     */
    assert(infiltratr_temporal_calendar_find("hebrew") == NULL);
    assert(infiltratr_temporal_calendar_find("gregorian") != NULL);
    assert(infiltratr_temporal_calendar_find("persian") == NULL);
    assert(infiltratr_temporal_calendar_find("coptic") == NULL);
    assert(infiltratr_temporal_installed_clock_mode_find("standard") != NULL);

    puts("Libraries temporal runtime smoke test passed.");
    return 0;
}
