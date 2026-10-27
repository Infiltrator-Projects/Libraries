// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file temporal_calendar.c
 * @brief System-wide executable calendar-resource loader and wrappers.
 */
#include "infiltratr/temporal_calendar.h"

#include "infiltratr/dynlib.h"
#include "infiltratr/temporal_id.h"
#include "infiltratr/utf8.h"
#include "temporal_system_calendar.h"
#include "temporal_module_internal.h"

#include <stdlib.h>
#include <string.h>

struct InfiltratrTemporalCalendarModule {
    InfiltratrDynlib library;
    const InfiltratrTemporalResourceMetadata *metadata;
    const InfiltratrTemporalCalendarProvider *provider;
};

InfiltratrTemporalCalendarModule *
infiltratr_temporal_calendar_module_open_system(void)
{
    static const InfiltratrTemporalResourceMetadata metadata = {
        sizeof(metadata), INFILTRATR_TEMPORAL_RESOURCE_METADATA_ABI,
        INFILTRATR_TEMPORAL_RESOURCE_CALENDAR, "gregorian", "Gregorian",
        false, false, false
    };
    InfiltratrTemporalCalendarModule *module = calloc(1U, sizeof(*module));
    if (module != NULL) {
        module->library = (InfiltratrDynlib)INFILTRATR_DYNLIB_INIT;
        module->metadata = &metadata;
        module->provider = infiltratr_temporal_system_calendar_provider();
    }
    return module;
}

InfiltratrTemporalCalendarModule *
infiltratr_temporal_calendar_module_open(const char *module_path)
{
    InfiltratrTemporalCalendarModule *module;
    InfiltratrTemporalResourceMetadataEntry metadata_entry = NULL;
    InfiltratrTemporalCalendarProviderEntry provider_entry = NULL;
    const InfiltratrTemporalResourceMetadata *metadata;
    const InfiltratrTemporalCalendarProvider *provider;
    InfiltratrDynlibBinding bindings[2];

    if (module_path == NULL || module_path[0] == '\0') {
        return NULL;
    }

    module = calloc(1U, sizeof(*module));
    if (module == NULL) {
        return NULL;
    }
    module->library = (InfiltratrDynlib)INFILTRATR_DYNLIB_INIT;
    if (!infiltratr_dynlib_open(&module->library, module_path)) {
        free(module);
        return NULL;
    }

    bindings[0] = (InfiltratrDynlibBinding){
        INFILTRATR_TEMPORAL_RESOURCE_METADATA_SYMBOL,
        &metadata_entry,
        sizeof(metadata_entry),
        true
    };
    bindings[1] = (InfiltratrDynlibBinding){
        INFILTRATR_TEMPORAL_CALENDAR_PROVIDER_SYMBOL,
        &provider_entry,
        sizeof(provider_entry),
        true
    };
    if (!infiltratr_dynlib_bind_symbols(
            &module->library, bindings, sizeof(bindings) / sizeof(bindings[0])) ||
        metadata_entry == NULL || provider_entry == NULL) {
        infiltratr_temporal_calendar_module_free(module);
        return NULL;
    }

    metadata = metadata_entry();
    provider = provider_entry();
    if (!temporal_calendar_provider_valid(metadata, provider)) {
        infiltratr_temporal_calendar_module_free(module);
        return NULL;
    }

    module->metadata = metadata;
    module->provider = provider;
    return module;
}

typedef struct CalendarLookup {
    const char *id;
    InfiltratrTemporalCalendarModule *module;
} CalendarLookup;

/* The first executable, ABI-valid provider wins in scan order. A preceding
 * metadata-only module cannot reserve the ID and shadow a usable module. */
static bool capture_calendar_module(
    const InfiltratrTemporalResourceMetadata *metadata,
    const char *path, void *user_data)
{
    CalendarLookup *lookup = user_data;
    InfiltratrTemporalCalendarModule *candidate;
    if (lookup == NULL || lookup->module != NULL || metadata == NULL ||
        metadata->id == NULL || path == NULL ||
        strcmp(metadata->id, lookup->id) != 0)
        return true;

    candidate = infiltratr_temporal_calendar_module_open(path);
    if (candidate == NULL) return true;
    if (strcmp(candidate->metadata->id, lookup->id) != 0) {
        infiltratr_temporal_calendar_module_free(candidate);
        return true;
    }
    lookup->module = candidate;
    return true;
}

InfiltratrTemporalCalendarModule *
infiltratr_temporal_calendar_module_open_id(const char *calendar_id)
{
    CalendarLookup lookup;
    size_t visited = 0U;
    if (!infiltratr_temporal_id_valid(calendar_id)) return NULL;

    lookup.id = calendar_id;
    lookup.module = NULL;
    if (!infiltratr_temporal_runtime_scan_native(
            INFILTRATR_TEMPORAL_RESOURCE_CALENDAR,
            capture_calendar_module, &lookup, &visited)) {
        infiltratr_temporal_calendar_module_free(lookup.module);
        return strcmp(calendar_id, "gregorian") == 0
            ? infiltratr_temporal_calendar_module_open_system() : NULL;
    }
    if (lookup.module == NULL && strcmp(calendar_id, "gregorian") == 0)
        return infiltratr_temporal_calendar_module_open_system();
    return lookup.module;
}

void infiltratr_temporal_calendar_module_free(
    InfiltratrTemporalCalendarModule *module)
{
    if (module != NULL) {
        infiltratr_dynlib_close(&module->library);
        free(module);
    }
}

const InfiltratrTemporalResourceMetadata *
infiltratr_temporal_calendar_module_metadata(
    const InfiltratrTemporalCalendarModule *module)
{
    return module != NULL ? module->metadata : NULL;
}

const InfiltratrTemporalCalendarProvider *
infiltratr_temporal_calendar_module_provider(
    const InfiltratrTemporalCalendarModule *module)
{
    return module != NULL ? module->provider : NULL;
}

bool infiltratr_temporal_calendar_fields_from_jdn(
    const InfiltratrTemporalCalendarModule *module,
    int64_t jdn,
    InfiltratrTemporalCalendarFields *fields)
{
    InfiltratrTemporalCalendarFields value;

    if (module == NULL || module->provider == NULL || fields == NULL ||
        !module->provider->fields_from_jdn(module->provider, jdn, &value)) {
        return false;
    }
    *fields = value;
    return true;
}

bool infiltratr_temporal_calendar_format(
    const InfiltratrTemporalCalendarModule *module,
    int64_t jdn,
    InfiltratrTemporalCalendarDatePart part,
    char *buffer,
    size_t capacity,
    size_t *length)
{
    char text[INFILTRATR_TEMPORAL_CALENDAR_TEXT_CAPACITY] = { 0 };
    size_t size = 0U;

    if (module == NULL || module->provider == NULL ||
        buffer == NULL || capacity == 0U ||
        part <= INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_INVALID ||
        part > INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_FULL ||
        !module->provider->format(
            module->provider, jdn, part, text, sizeof(text), &size) ||
        size == 0U || size >= sizeof(text) || size >= capacity ||
        text[size] != '\0' || strlen(text) != size ||
        !infiltratr_utf8_validate(text, size)) {
        return false;
    }

    memcpy(buffer, text, size + 1U);
    if (length != NULL) {
        *length = size;
    }
    return true;
}

bool infiltratr_temporal_calendar_period_start(
    const InfiltratrTemporalCalendarModule *module,
    int64_t jdn,
    int64_t *result)
{
    int64_t value;

    if (module == NULL || module->provider == NULL || result == NULL ||
        !module->provider->period_start(module->provider, jdn, &value)) {
        return false;
    }
    *result = value;
    return true;
}

bool infiltratr_temporal_calendar_add_periods(
    const InfiltratrTemporalCalendarModule *module,
    int64_t jdn,
    int32_t amount,
    int64_t *result)
{
    int64_t value;

    if (module == NULL || module->provider == NULL || result == NULL ||
        !module->provider->add_periods(module->provider, jdn, amount, &value)) {
        return false;
    }
    *result = value;
    return true;
}

bool infiltratr_temporal_calendar_add_years(
    const InfiltratrTemporalCalendarModule *module,
    int64_t jdn,
    int32_t amount,
    int64_t *result)
{
    int64_t value;

    if (module == NULL || module->provider == NULL || result == NULL ||
        !module->provider->add_years(module->provider, jdn, amount, &value)) {
        return false;
    }
    *result = value;
    return true;
}
