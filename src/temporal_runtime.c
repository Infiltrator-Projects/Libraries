// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file temporal_runtime.c
 * @brief Validated executable temporal-resource discovery and merged metadata.
 */
#include "infiltratr/temporal_runtime.h"

#include "infiltratr/dynlib.h"
#include "infiltratr/temporal_calendar.h"
#include "infiltratr/temporal_clock_resource.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

typedef struct RuntimeScanContext {
    InfiltratrTemporalResourceKind kind;
    InfiltratrTemporalRuntimeVisitor visitor;
    void *user_data;
    size_t emitted;
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
    __atomic_store_n(&catalogue_state, CATALOGUE_READY, __ATOMIC_RELEASE);
#endif
}

static void catalogue_wait_until_ready(void)
{
    while (!catalogue_is_ready()) {
#ifdef _WIN32
        (void)SwitchToThread();
#else
        /*
         * Initialisation is a bounded filesystem scan and normally completes
         * before another caller observes this state. A compiler barrier keeps
         * this dependency-free fallback from becoming an optimised-away loop.
         */
        __asm__ __volatile__("" ::: "memory");
#endif
    }
}

static char *copy_text(const char *text)
{
    size_t length;
    char *copy;

    if (text == NULL) return NULL;
    length = strlen(text);
    if (length == SIZE_MAX) return NULL;
    copy = malloc(length + 1U);
    if (copy == NULL) return NULL;
    memcpy(copy, text, length + 1U);
    return copy;
}

static bool resource_id_valid(const char *id)
{
    size_t length = 0U;

    if (id == NULL || id[0] == '\0') return false;
    while (id[length] != '\0') {
        const unsigned char value = (unsigned char)id[length];
        const bool valid =
            (value >= (unsigned char)'a' && value <= (unsigned char)'z') ||
            (value >= (unsigned char)'0' && value <= (unsigned char)'9') ||
            (value == (unsigned char)'-' && length != 0U);
        if (!valid || length + 1U >= INFILTRATR_TEMPORAL_ID_CAPACITY) {
            return false;
        }
        length++;
    }
    return length != 0U && id[length - 1U] != '-';
}

static bool metadata_valid(
    const InfiltratrTemporalResourceMetadata *metadata,
    InfiltratrTemporalResourceKind expected_kind)
{
    if (metadata == NULL ||
        metadata->struct_size < sizeof(*metadata) ||
        metadata->abi_version != INFILTRATR_TEMPORAL_RESOURCE_METADATA_ABI ||
        metadata->kind != expected_kind ||
        !resource_id_valid(metadata->id) ||
        metadata->name == NULL || metadata->name[0] == '\0') {
        return false;
    }
    return strlen(metadata->name) <= 255U;
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
    if (metadata_valid(metadata, context->kind)) {
        if (context->visitor != NULL) {
            keep_scanning = context->visitor(
                metadata, resource->path, context->user_data);
        }
        if (keep_scanning) context->emitted++;
    }

    infiltratr_dynlib_close(&library);
    return keep_scanning;
}

bool infiltratr_temporal_runtime_scan_directory(
    const char *directory,
    InfiltratrTemporalResourceKind kind,
    InfiltratrTemporalRuntimeVisitor visitor,
    void *user_data,
    size_t *visited)
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
    if (!infiltratr_temporal_resource_scan_directory(
            directory, kind, inspect_candidate, &context, &candidates)) {
        return false;
    }
    (void)candidates;
    if (visited != NULL) *visited = context.emitted;
    return true;
}

bool infiltratr_temporal_runtime_scan_native(
    InfiltratrTemporalResourceKind kind,
    InfiltratrTemporalRuntimeVisitor visitor,
    void *user_data,
    size_t *visited)
{
    const char *directory;

    if (kind == INFILTRATR_TEMPORAL_RESOURCE_CALENDAR) {
        directory = INFILTRATR_TEMPORAL_NATIVE_CALENDAR_DIRECTORY;
    } else if (kind == INFILTRATR_TEMPORAL_RESOURCE_CLOCK) {
        directory = INFILTRATR_TEMPORAL_NATIVE_CLOCK_DIRECTORY;
    } else {
        return false;
    }
    return infiltratr_temporal_runtime_scan_directory(
        directory, kind, visitor, user_data, visited);
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
    InfiltratrTemporalCalendarModule *module =
        infiltratr_temporal_calendar_module_open(module_path);
    if (module == NULL) return true;
    const InfiltratrTemporalResourceMetadata *loaded =
        infiltratr_temporal_calendar_module_metadata(module);
    const bool valid = loaded != NULL && strcmp(loaded->id, metadata->id) == 0;
    infiltratr_temporal_calendar_module_free(module);
    if (!valid) return true;


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

    id = copy_text(metadata->id);
    name = copy_text(metadata->name);
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
    InfiltratrTemporalClockModule *module =
        infiltratr_temporal_clock_module_open(module_path);
    if (module == NULL) return true;
    const InfiltratrTemporalResourceMetadata *loaded =
        infiltratr_temporal_clock_module_metadata(module);
    const bool valid = loaded != NULL && strcmp(loaded->id, metadata->id) == 0;
    infiltratr_temporal_clock_module_free(module);
    if (!valid) return true;


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

    id = copy_text(metadata->id);
    name = copy_text(metadata->name);
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

static void catalogue_initialise(void)
{
    size_t ignored = 0U;

    if (catalogue_is_ready()) return;
    if (!catalogue_try_begin_initialisation()) {
        catalogue_wait_until_ready();
        return;
    }

    if (!infiltratr_temporal_runtime_scan_native(
            INFILTRATR_TEMPORAL_RESOURCE_CALENDAR,
            append_calendar, NULL, &ignored)) {
        clear_runtime_calendars();
    }
    if (!infiltratr_temporal_runtime_scan_native(
            INFILTRATR_TEMPORAL_RESOURCE_CLOCK,
            append_clock, NULL, &ignored)) {
        clear_runtime_clocks();
    }
    catalogue_mark_ready();
}

size_t infiltratr_temporal_installed_calendar_count(void)
{
    catalogue_initialise();
    return infiltratr_temporal_calendar_count() + runtime_calendar_count;
}

const InfiltratrTemporalCalendarInfo *
infiltratr_temporal_installed_calendar_at(size_t index)
{
    const size_t builtin_count = infiltratr_temporal_calendar_count();

    catalogue_initialise();
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
    catalogue_initialise();
    for (index = 0U; index < runtime_calendar_count; ++index) {
        if (strcmp(runtime_calendars[index].info.id, id) == 0) {
            return &runtime_calendars[index].info;
        }
    }
    return NULL;
}

size_t infiltratr_temporal_installed_clock_mode_count(void)
{
    catalogue_initialise();
    return 1U + runtime_clock_count;
}

const InfiltratrTemporalClockModeInfo *
infiltratr_temporal_installed_clock_mode_at(size_t index)
{
    catalogue_initialise();
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
    catalogue_initialise();
    for (index = 0U; index < runtime_clock_count; ++index) {
        if (strcmp(runtime_clocks[index].info.id, id) == 0) {
            return &runtime_clocks[index].info;
        }
    }
    return NULL;
}
