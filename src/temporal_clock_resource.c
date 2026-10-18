// SPDX-License-Identifier: GPL-3.0-or-later
/* Validated native clock resources; Common owns loader and executable ABI. */
#include "infiltratr/temporal_clock_resource.h"

#include "infiltratr/dynlib.h"
#include "infiltratr/temporal_id.h"
#include "infiltratr/utf8.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

struct InfiltratrTemporalClockModule {
    InfiltratrDynlib library;
    const InfiltratrTemporalResourceMetadata *metadata;
    const InfiltratrTemporalClockProvider *provider;
};

static bool provider_valid(const InfiltratrTemporalResourceMetadata *metadata,
                           const InfiltratrTemporalClockProvider *provider)
{
    return metadata != NULL && provider != NULL &&
        metadata->struct_size >= sizeof(*metadata) &&
        metadata->abi_version == INFILTRATR_TEMPORAL_RESOURCE_METADATA_ABI &&
        metadata->kind == INFILTRATR_TEMPORAL_RESOURCE_CLOCK &&
        infiltratr_temporal_id_valid(metadata->id) &&
        metadata->name != NULL && metadata->name[0] != '\0' &&
        provider->struct_size >= sizeof(*provider) &&
        provider->abi_version == INFILTRATR_TEMPORAL_CLOCK_PROVIDER_ABI &&
        provider->clock_id != NULL &&
        strcmp(provider->clock_id, metadata->id) == 0 &&
        provider->format != NULL && provider->next_tick != NULL;
}

void infiltratr_temporal_clock_module_free(InfiltratrTemporalClockModule *module)
{
    if (module != NULL) {
        infiltratr_dynlib_close(&module->library);
        free(module);
    }
}

InfiltratrTemporalClockModule *
infiltratr_temporal_clock_module_open(const char *module_path)
{
    InfiltratrTemporalClockModule *module;
    InfiltratrTemporalResourceMetadataEntry metadata_entry = NULL;
    InfiltratrTemporalClockProviderEntry provider_entry = NULL;
    InfiltratrDynlibBinding bindings[2];

    if (module_path == NULL || module_path[0] == '\0') return NULL;
    module = calloc(1U, sizeof(*module));
    if (module == NULL) return NULL;
    module->library = (InfiltratrDynlib)INFILTRATR_DYNLIB_INIT;
    if (!infiltratr_dynlib_open(&module->library, module_path)) {
        free(module);
        return NULL;
    }
    bindings[0] = (InfiltratrDynlibBinding){
        INFILTRATR_TEMPORAL_RESOURCE_METADATA_SYMBOL,
        &metadata_entry, sizeof(metadata_entry), true
    };
    bindings[1] = (InfiltratrDynlibBinding){
        INFILTRATR_TEMPORAL_CLOCK_PROVIDER_SYMBOL,
        &provider_entry, sizeof(provider_entry), true
    };
    if (!infiltratr_dynlib_bind_symbols(
            &module->library, bindings, 2U) ||
        metadata_entry == NULL || provider_entry == NULL) {
        infiltratr_temporal_clock_module_free(module);
        return NULL;
    }
    module->metadata = metadata_entry();
    module->provider = provider_entry();
    if (!provider_valid(module->metadata, module->provider)) {
        infiltratr_temporal_clock_module_free(module);
        return NULL;
    }
    return module;
}

typedef struct ClockLookup {
    const char *id;
    char *path;
} ClockLookup;

static bool capture_clock(const InfiltratrTemporalResourceMetadata *metadata,
                          const char *path, void *context)
{
    ClockLookup *lookup = context;
    size_t length;
    if (lookup == NULL || lookup->path != NULL || metadata == NULL ||
        metadata->id == NULL || path == NULL ||
        strcmp(metadata->id, lookup->id) != 0) return true;
    length = strlen(path);
    if (length == SIZE_MAX) return true;
    lookup->path = malloc(length + 1U);
    if (lookup->path != NULL) memcpy(lookup->path, path, length + 1U);
    return true;
}

InfiltratrTemporalClockModule *
infiltratr_temporal_clock_module_open_id(const char *clock_id)
{
    ClockLookup lookup;
    InfiltratrTemporalClockModule *module = NULL;
    size_t visited = 0U;
    if (!infiltratr_temporal_id_valid(clock_id)) return NULL;
    lookup.id = clock_id;
    lookup.path = NULL;
    if (!infiltratr_temporal_runtime_scan_native(
            INFILTRATR_TEMPORAL_RESOURCE_CLOCK,
            capture_clock, &lookup, &visited)) {
        free(lookup.path);
        return NULL;
    }
    (void)visited;
    if (lookup.path != NULL) {
        module = infiltratr_temporal_clock_module_open(lookup.path);
        if (module != NULL && strcmp(module->metadata->id, clock_id) != 0) {
            infiltratr_temporal_clock_module_free(module);
            module = NULL;
        }
    }
    free(lookup.path);
    return module;
}

const InfiltratrTemporalResourceMetadata *
infiltratr_temporal_clock_module_metadata(const InfiltratrTemporalClockModule *module)
{
    return module == NULL ? NULL : module->metadata;
}

bool infiltratr_temporal_clock_format(
    const InfiltratrTemporalClockModule *module,
    const char *locale, int64_t instant, int32_t offset,
    bool seconds, bool vertical, double latitude, double longitude,
    char *buffer, size_t capacity, size_t *length)
{
    char value[INFILTRATR_TEMPORAL_CLOCK_TEXT_CAPACITY] = {0};
    size_t size = 0U;

    if (module == NULL || module->provider == NULL || buffer == NULL ||
        capacity == 0U || !isfinite(latitude) || !isfinite(longitude) ||
        !module->provider->format(
            module->provider, locale, instant, offset, seconds,
            vertical, latitude, longitude, value, sizeof(value), &size) ||
        size == 0U || size >= sizeof(value) || size >= capacity ||
        value[size] != '\0' || strlen(value) != size ||
        !infiltratr_utf8_validate(value, size)) return false;

    memcpy(buffer, value, size + 1U);
    if (length != NULL) *length = size;
    return true;
}

bool infiltratr_temporal_clock_next_tick(
    const InfiltratrTemporalClockModule *module,
    int64_t instant, int32_t offset, bool seconds,
    double latitude, double longitude, uint32_t *milliseconds)
{
    uint32_t value = 0U;
    if (module == NULL || module->provider == NULL ||
        milliseconds == NULL || !isfinite(latitude) ||
        !isfinite(longitude) ||
        !module->provider->next_tick(
            module->provider, instant, offset, seconds,
            latitude, longitude, &value) || value == 0U) return false;
    *milliseconds = value;
    return true;
}

/*
 * Stable presentation object shared by Files, System Settings and other
 * consumers. Module discovery is performed once per selection, never once per
 * timestamp or UI repaint. Clock arithmetic stays in the loaded resource.
 */
struct InfiltratrTemporalClockPresentation {
    InfiltratrTemporalClockModule *module;
    char *mode;
};

static bool clock_requires_native_resource(const char *mode)
{
    static const char *const migrated[] = {
        "decimal", "internet", "unix", "hexadecimal", "binary",
        "julian", "modified-julian", "chinese-ke", "mean-solar",
        "sidereal", "chinese-time", "solar", "standard-12", "standard-24"
    };
    size_t i;
    if (mode == NULL) return false;
    for (i = 0U; i < sizeof(migrated) / sizeof(migrated[0]); ++i) {
        if (strcmp(mode, migrated[i]) == 0) return true;
    }
    return false;
}

InfiltratrTemporalClockPresentation *
infiltratr_temporal_clock_presentation_new(const char *clock_id)
{
    InfiltratrTemporalClockPresentation *presentation;
    const InfiltratrTemporalClockModeInfo *builtin;
    const bool resource_required = clock_requires_native_resource(clock_id);
    const size_t length = clock_id != NULL ? strlen(clock_id) : 0U;

    if (!infiltratr_temporal_id_valid(clock_id) ||
        strcmp(clock_id, "standard") == 0) return NULL;

    builtin = infiltratr_temporal_clock_mode_find(clock_id);
    presentation = calloc(1U, sizeof(*presentation));
    if (presentation == NULL) return NULL;
    if (length == SIZE_MAX) {
        free(presentation);
        return NULL;
    }
    presentation->mode = malloc(length + 1U);
    if (presentation->mode == NULL) {
        free(presentation);
        return NULL;
    }
    memcpy(presentation->mode, clock_id, length + 1U);

    /*
     * Executable system resources take precedence over legacy portable
     * formatting for every stable clock ID, including an ID already present
     * in the compiled capability catalogue. This avoids a new Common-side
     * routing exception whenever another clock is extracted.
     *
     * All fourteen extracted clocks are resource-only: an absent or
     * invalid module must fail closed rather than silently invoke a
     * duplicate compiled formatter. Unmigrated modes retain the
     * compatibility renderer until their resources have been extracted.
     */
    presentation->module = infiltratr_temporal_clock_module_open_id(clock_id);
    if (presentation->module == NULL &&
        (resource_required || builtin == NULL)) {
        infiltratr_temporal_clock_presentation_free(presentation);
        return NULL;
    }
    return presentation;
}

void infiltratr_temporal_clock_presentation_free(
    InfiltratrTemporalClockPresentation *presentation)
{
    if (presentation == NULL) return;
    infiltratr_temporal_clock_module_free(presentation->module);
    free(presentation->mode);
    free(presentation);
}

bool infiltratr_temporal_clock_presentation_format(
    const InfiltratrTemporalClockPresentation *presentation,
    const char *locale, int64_t instant, int32_t offset,
    bool seconds, bool vertical, bool location_configured,
    double latitude, double longitude,
    char *buffer, size_t capacity, size_t *length)
{
    if (presentation == NULL || buffer == NULL || capacity == 0U) {
        return false;
    }
    if (presentation->module != NULL) {
        const InfiltratrTemporalResourceMetadata *metadata =
            infiltratr_temporal_clock_module_metadata(presentation->module);
        if (metadata == NULL ||
            ((metadata->requires_latitude || metadata->requires_longitude) &&
             !location_configured)) return false;
        return infiltratr_temporal_clock_format(
            presentation->module, locale, instant, offset, seconds,
            vertical, latitude, longitude, buffer, capacity, length);
    }
    return infiltratr_temporal_format_clock_mode_localized(
        locale, presentation->mode, instant, offset, seconds, vertical,
        location_configured, latitude, longitude, buffer, capacity, length);
}

bool infiltratr_temporal_clock_presentation_next_tick(
    const InfiltratrTemporalClockPresentation *presentation,
    int64_t instant, int32_t offset, bool seconds,
    double latitude, double longitude, uint32_t *milliseconds)
{
    return presentation != NULL && presentation->module != NULL &&
        infiltratr_temporal_clock_next_tick(
            presentation->module, instant, offset, seconds,
            latitude, longitude, milliseconds);
}
