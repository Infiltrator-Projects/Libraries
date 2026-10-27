// SPDX-License-Identifier: GPL-3.0-or-later
/* Private ABI-1 validation shared by runtime discovery and typed loaders.
 * Keep these checks identical: catalogue enumeration must never advertise a
 * resource whose executable provider would be rejected by direct loading. */
#ifndef INFILTRATR_TEMPORAL_MODULE_INTERNAL_H
#define INFILTRATR_TEMPORAL_MODULE_INTERNAL_H

#include "infiltratr/temporal_calendar.h"
#include "infiltratr/temporal_clock_resource.h"
#include "infiltratr/temporal_id.h"

#include <string.h>

static inline bool temporal_module_metadata_valid(
    const InfiltratrTemporalResourceMetadata *metadata,
    InfiltratrTemporalResourceKind kind)
{
    return metadata != NULL &&
        metadata->struct_size >= sizeof(*metadata) &&
        metadata->abi_version == INFILTRATR_TEMPORAL_RESOURCE_METADATA_ABI &&
        metadata->kind == kind &&
        infiltratr_temporal_id_valid(metadata->id) &&
        metadata->name != NULL && metadata->name[0] != '\0';
}

static inline bool temporal_calendar_provider_valid(
    const InfiltratrTemporalResourceMetadata *metadata,
    const InfiltratrTemporalCalendarProvider *provider)
{
    return temporal_module_metadata_valid(
            metadata, INFILTRATR_TEMPORAL_RESOURCE_CALENDAR) &&
        provider != NULL &&
        provider->struct_size >= sizeof(*provider) &&
        provider->abi_version == INFILTRATR_TEMPORAL_CALENDAR_PROVIDER_ABI &&
        provider->calendar_id != NULL &&
        strcmp(provider->calendar_id, metadata->id) == 0 &&
        provider->fields_from_jdn != NULL &&
        provider->format != NULL &&
        provider->period_start != NULL &&
        provider->add_periods != NULL &&
        provider->add_years != NULL;
}

static inline bool temporal_clock_provider_valid(
    const InfiltratrTemporalResourceMetadata *metadata,
    const InfiltratrTemporalClockProvider *provider)
{
    return temporal_module_metadata_valid(
            metadata, INFILTRATR_TEMPORAL_RESOURCE_CLOCK) &&
        provider != NULL &&
        provider->struct_size >= sizeof(*provider) &&
        provider->abi_version == INFILTRATR_TEMPORAL_CLOCK_PROVIDER_ABI &&
        provider->clock_id != NULL &&
        strcmp(provider->clock_id, metadata->id) == 0 &&
        provider->format != NULL && provider->next_tick != NULL;
}

#endif
