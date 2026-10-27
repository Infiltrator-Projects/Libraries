// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file temporal_runtime.c
 * @brief Validated executable temporal-resource discovery and merged metadata.
 */
#include "infiltratr/temporal_runtime.h"

#include "infiltratr/dynlib.h"
#include "infiltratr/temporal_calendar.h"
#include "infiltratr/temporal_clock_resource.h"
#include "string_internal.h"
#include "temporal_module_internal.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

typedef struct RuntimeScanContext {
    InfiltratrTemporalResourceKind kind;
    InfiltratrTemporalRuntimeVisitor visitor;
    void *user_data;
    size_t emitted;
    bool require_executable_provider;
} RuntimeScanContext;

typedef struct RuntimeCalendarEntry {
    char *id;
    char *name;
    InfiltratrTemporalCalendarInfo info;
} RuntimeCalendarEntry;

typedef struct RuntimeClockEntry {
    char *id;
    char *name;
    InfiltratrTemporalClockModeInfo info;
} RuntimeClockEntry;

static RuntimeCalendarEntry *runtime_calendars;
static size_t runtime_calendar_count;
static size_t runtime_calendar_capacity;
static RuntimeClockEntry *runtime_clocks;
static size_t runtime_clock_count;
static size_t runtime_clock_capacity;

#ifdef _WIN32
static volatile LONG catalogue_state;
#else
static int catalogue_state;
static pthread_mutex_t catalogue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t catalogue_cond = PTHREAD_COND_INITIALIZER;
#endif
/* Module constructors may enter the catalogue recursively. Expose no
 * partially-built optional entries to that same thread. */
#if defined(_MSC_VER)
static __declspec(thread) bool catalogue_initialising_here;
#else
static _Thread_local bool catalogue_initialising_here;
#endif

enum {
    CATALOGUE_UNINITIALISED = 0,
    CATALOGUE_INITIALISING = 1,
    CATALOGUE_READY = 2
};

static bool catalogue_is_ready(void)
{
#ifdef _WIN32
    return InterlockedCompareExchange(
        &catalogue_state, CATALOGUE_READY, CATALOGUE_READY) ==
        CATALOGUE_READY;
#else
    return __atomic_load_n(&catalogue_state, __ATOMIC_ACQUIRE) ==
        CATALOGUE_READY;
#endif
}

static bool catalogue_try_begin_initialisation(void)
{
#ifdef _WIN32
    return InterlockedCompareExchange(
        &catalogue_state,
        CATALOGUE_INITIALISING,
        CATALOGUE_UNINITIALISED) == CATALOGUE_UNINITIALISED;
#else
    int expected = CATALOGUE_UNINITIALISED;
    return __atomic_compare_exchange_n(
        &catalogue_state,
        &expected,
        CATALOGUE_INITIALISING,
        false,
        __ATOMIC_ACQ_REL,
        __ATOMIC_ACQUIRE);
#endif
}

static void catalogue_mark_ready(void)
{
#ifdef _WIN32
    (void)InterlockedExchange(&catalogue_state, CATALOGUE_READY);
#else
    /* The predicate and broadcast use the same mutex: no lost wakeups. */
    (void)pthread_mutex_lock(&catalogue_mutex);
    __atomic_store_n(&catalogue_state, CATALOGUE_READY, __ATOMIC_RELEASE);
    (void)pthread_cond_broadcast(&catalogue_cond);
    (void)pthread_mutex_unlock(&catalogue_mutex);
#endif
}

static bool catalogue_wait_until_ready(void)
{
    if (catalogue_initialising_here) return false;
#ifdef _WIN32
    while (!catalogue_is_ready()) Sleep(1);
#else
    (void)pthread_mutex_lock(&catalogue_mutex);
    while (!catalogue_is_ready())
        (void)pthread_cond_wait(&catalogue_cond, &catalogue_mutex);
    (void)pthread_mutex_unlock(&catalogue_mutex);
#endif
    return true;
}

static bool metadata_valid(
    const InfiltratrTemporalResourceMetadata *metadata,
    InfiltratrTemporalResourceKind expected_kind)
{
    return temporal_module_metadata_valid(metadata, expected_kind) &&
        strlen(metadata->name) <= 255U;
}

/* Catalogue construction validates the typed provider while the metadata
 * library is already open, avoiding a second dlopen per accepted resource.
 * Public runtime scans remain metadata-only by their existing contract. */
static bool executable_provider_valid(
    InfiltratrDynlib *library,
    const InfiltratrTemporalResourceMetadata *metadata)
{
    if (metadata->kind == INFILTRATR_TEMPORAL_RESOURCE_CALENDAR) {
        InfiltratrTemporalCalendarProviderEntry entry = NULL;
        return infiltratr_dynlib_symbol(
                   library, INFILTRATR_TEMPORAL_CALENDAR_PROVIDER_SYMBOL,
                   &entry, sizeof(entry)) &&
            entry != NULL && temporal_calendar_provider_valid(metadata, entry());
    }
    if (metadata->kind == INFILTRATR_TEMPORAL_RESOURCE_CLOCK) {
        InfiltratrTemporalClockProviderEntry entry = NULL;
        return infiltratr_dynlib_symbol(
                   library, INFILTRATR_TEMPORAL_CLOCK_PROVIDER_SYMBOL,
                   &entry, sizeof(entry)) &&
            entry != NULL && temporal_clock_provider_valid(metadata, entry());
    }
    return false;
}

static bool inspect_candidate(
    const InfiltratrTemporalResourceInfo *resource,
    void *user_data)
{
    RuntimeScanContext *context = user_data;
    InfiltratrDynlib library = INFILTRATR_DYNLIB_INIT;
    InfiltratrTemporalResourceMetadataEntry entry = NULL;
    const InfiltratrTemporalResourceMetadata *metadata;
    bool keep_scanning = true;

    if (resource == NULL || context == NULL ||
        resource->form != INFILTRATR_TEMPORAL_RESOURCE_FILE) {
        return true;
    }
    if (!infiltratr_dynlib_open(&library, resource->path)) {
        return true;
    }
    if (!infiltratr_dynlib_symbol(
            &library,
            INFILTRATR_TEMPORAL_RESOURCE_METADATA_SYMBOL,
            &entry,
            sizeof(entry)) ||
        entry == NULL) {
        infiltratr_dynlib_close(&library);
        return true;
    }

    metadata = entry();
    if (metadata_valid(metadata, context->kind) &&
        (!context->require_executable_provider ||
         executable_provider_valid(&library, metadata))) {
        if (context->visitor != NULL) {
            keep_scanning = context->visitor(
                metadata, resource->path, context->user_data);
        }
        if (keep_scanning) context->emitted++;
    }

    infiltratr_dynlib_close(&library);
    return keep_scanning;
}

static bool scan_directory_impl(
    const char *directory,
    InfiltratrTemporalResourceKind kind,
    InfiltratrTemporalRuntimeVisitor visitor,
    void *user_data,
    size_t *visited,
    bool require_executable_provider)
{
    RuntimeScanContext context;
    size_t candidates = 0U;

    if (kind != INFILTRATR_TEMPORAL_RESOURCE_CALENDAR &&
        kind != INFILTRATR_TEMPORAL_RESOURCE_CLOCK) {
        return false;
    }

    context.kind = kind;
    context.visitor = visitor;
    context.user_data = user_data;
    context.emitted = 0U;
    context.require_executable_provider = require_executable_provider;
    if (!infiltratr_temporal_resource_scan_directory(
            directory, kind, inspect_candidate, &context, &candidates)) {
        return false;
    }
    (void)candidates;
    if (visited != NULL) *visited = context.emitted;
    return true;
}

bool infiltratr_temporal_runtime_scan_directory(
    const char *directory,
    InfiltratrTemporalResourceKind kind,
    InfiltratrTemporalRuntimeVisitor visitor,
    void *user_data,
    size_t *visited)
{
    return scan_directory_impl(directory, kind, visitor, user_data, visited,
                               false);
}

static bool scan_native_impl(
    InfiltratrTemporalResourceKind kind,
    InfiltratrTemporalRuntimeVisitor visitor,
    void *user_data,
    size_t *visited,
    bool require_executable_provider)
{
    const char *directory;

    if (kind == INFILTRATR_TEMPORAL_RESOURCE_CALENDAR) {
        directory = INFILTRATR_TEMPORAL_NATIVE_CALENDAR_DIRECTORY;
    } else if (kind == INFILTRATR_TEMPORAL_RESOURCE_CLOCK) {
        directory = INFILTRATR_TEMPORAL_NATIVE_CLOCK_DIRECTORY;
    } else {
        return false;
    }
    return scan_directory_impl(directory, kind, visitor, user_data, visited,
                               require_executable_provider);
}

bool infiltratr_temporal_runtime_scan_native(
    InfiltratrTemporalResourceKind kind,
    InfiltratrTemporalRuntimeVisitor visitor,
    void *user_data,
    size_t *visited)
{
    return scan_native_impl(kind, visitor, user_data, visited, false);
}

static bool calendar_duplicate(const char *id)
{
    size_t index;

    if (infiltratr_temporal_calendar_find(id) != NULL) return true;
    for (index = 0U; index < runtime_calendar_count; ++index) {
        if (strcmp(runtime_calendars[index].info.id, id) == 0) return true;
    }
    return false;
}

static bool clock_duplicate(const char *id)
{
    size_t index;

    for (index = 0U; index < runtime_clock_count; ++index) {
        if (strcmp(runtime_clocks[index].info.id, id) == 0) return true;
    }
    return false;
}

static bool append_calendar(
    const InfiltratrTemporalResourceMetadata *metadata,
    const char *module_path,
    void *user_data)
{
    RuntimeCalendarEntry *grown;
    RuntimeCalendarEntry *entry;
    size_t capacity;
    char *id;
    char *name;

    (void)module_path;
    (void)user_data;
    if (calendar_duplicate(metadata->id)) return true;

    if (runtime_calendar_count == runtime_calendar_capacity) {
        capacity = runtime_calendar_capacity == 0U
            ? 8U : runtime_calendar_capacity * 2U;
        if (capacity < runtime_calendar_capacity ||
            capacity > SIZE_MAX / sizeof(*grown)) {
            return false;
        }
        grown = realloc(runtime_calendars, capacity * sizeof(*grown));
        if (grown == NULL) return false;
        runtime_calendars = grown;
        runtime_calendar_capacity = capacity;
    }

    id = infiltratr_private_copy_text(metadata->id);
    name = infiltratr_private_copy_text(metadata->name);
    if (id == NULL || name == NULL) {
        free(id);
        free(name);
        return false;
    }
    entry = &runtime_calendars[runtime_calendar_count++];
    entry->id = id;
    entry->name = name;
    entry->info.id = entry->id;
    entry->info.name = entry->name;
    return true;
}

static bool append_clock(
    const InfiltratrTemporalResourceMetadata *metadata,
    const char *module_path,
    void *user_data)
{
    RuntimeClockEntry *grown;
    RuntimeClockEntry *entry;
    size_t capacity;
    char *id;
    char *name;

    (void)module_path;
    (void)user_data;
    if (clock_duplicate(metadata->id)) return true;

    if (runtime_clock_count == runtime_clock_capacity) {
        capacity = runtime_clock_capacity == 0U
            ? 8U : runtime_clock_capacity * 2U;
        if (capacity < runtime_clock_capacity ||
            capacity > SIZE_MAX / sizeof(*grown)) {
            return false;
        }
        grown = realloc(runtime_clocks, capacity * sizeof(*grown));
        if (grown == NULL) return false;
        runtime_clocks = grown;
        runtime_clock_capacity = capacity;
    }

    id = infiltratr_private_copy_text(metadata->id);
    name = infiltratr_private_copy_text(metadata->name);
    if (id == NULL || name == NULL) {
        free(id);
        free(name);
        return false;
    }
    entry = &runtime_clocks[runtime_clock_count++];
    entry->id = id;
    entry->name = name;
    entry->info.id = entry->id;
    entry->info.name = entry->name;
    entry->info.supports_seconds = metadata->supports_seconds;
    entry->info.requires_latitude = metadata->requires_latitude;
    entry->info.requires_longitude = metadata->requires_longitude;
    return true;
}

static void clear_runtime_calendars(void)
{
    size_t index;
    for (index = 0U; index < runtime_calendar_count; ++index) {
        free(runtime_calendars[index].id);
        free(runtime_calendars[index].name);
    }
    free(runtime_calendars);
    runtime_calendars = NULL;
    runtime_calendar_count = 0U;
    runtime_calendar_capacity = 0U;
}

static void clear_runtime_clocks(void)
{
    size_t index;
    for (index = 0U; index < runtime_clock_count; ++index) {
        free(runtime_clocks[index].id);
        free(runtime_clocks[index].name);
    }
    free(runtime_clocks);
    runtime_clocks = NULL;
    runtime_clock_count = 0U;
    runtime_clock_capacity = 0U;
}

static bool catalogue_initialise(void)
{
    size_t ignored = 0U;

    if (catalogue_is_ready()) return true;
    if (!catalogue_try_begin_initialisation())
        return catalogue_wait_until_ready();
    catalogue_initialising_here = true;

    if (!scan_native_impl(
            INFILTRATR_TEMPORAL_RESOURCE_CALENDAR,
            append_calendar, NULL, &ignored, true)) {
        clear_runtime_calendars();
    }
    if (!scan_native_impl(
            INFILTRATR_TEMPORAL_RESOURCE_CLOCK,
            append_clock, NULL, &ignored, true)) {
        clear_runtime_clocks();
    }
    catalogue_mark_ready();
    catalogue_initialising_here = false;
    return true;
}

size_t infiltratr_temporal_installed_calendar_count(void)
{
    if (!catalogue_initialise()) return infiltratr_temporal_calendar_count();
    return infiltratr_temporal_calendar_count() + runtime_calendar_count;
}

const InfiltratrTemporalCalendarInfo *
infiltratr_temporal_installed_calendar_at(size_t index)
{
    const size_t builtin_count = infiltratr_temporal_calendar_count();

    if (!catalogue_initialise())
        return index < builtin_count ? infiltratr_temporal_calendar_at(index) : NULL;
    if (index < builtin_count) return infiltratr_temporal_calendar_at(index);
    index -= builtin_count;
    if (index >= runtime_calendar_count) return NULL;
    return &runtime_calendars[index].info;
}

const InfiltratrTemporalCalendarInfo *
infiltratr_temporal_installed_calendar_find(const char *id)
{
    const InfiltratrTemporalCalendarInfo *builtin;
    size_t index;

    if (id == NULL) return NULL;
    builtin = infiltratr_temporal_calendar_find(id);
    if (builtin != NULL) return builtin;
    if (!catalogue_initialise()) return NULL;
    for (index = 0U; index < runtime_calendar_count; ++index) {
        if (strcmp(runtime_calendars[index].info.id, id) == 0) {
            return &runtime_calendars[index].info;
        }
    }
    return NULL;
}

size_t infiltratr_temporal_installed_clock_mode_count(void)
{
    if (!catalogue_initialise()) return 1U;
    return 1U + runtime_clock_count;
}

const InfiltratrTemporalClockModeInfo *
infiltratr_temporal_installed_clock_mode_at(size_t index)
{
    if (!catalogue_initialise())
        return index == 0U ? infiltratr_temporal_clock_mode_find("standard") : NULL;
    /* OS-following standard is a policy choice, not a native implementation. */
    if (index == 0U) return infiltratr_temporal_clock_mode_find("standard");
    index--;
    if (index >= runtime_clock_count) return NULL;
    return &runtime_clocks[index].info;
}

const InfiltratrTemporalClockModeInfo *
infiltratr_temporal_installed_clock_mode_find(const char *id)
{
    size_t index;

    if (id == NULL) return NULL;
    if (strcmp(id, "standard") == 0)
        return infiltratr_temporal_clock_mode_find("standard");
    if (!catalogue_initialise()) return NULL;
    for (index = 0U; index < runtime_clock_count; ++index) {
        if (strcmp(runtime_clocks[index].info.id, id) == 0) {
            return &runtime_clocks[index].info;
        }
    }
    return NULL;
}
