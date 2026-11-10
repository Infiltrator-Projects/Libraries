/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "infiltratr/filesystem_module_loader.h"
#include <stddef.h>

typedef const IfsFsLegacyPrefix *(*LegacyModuleEntry)(void);
static const char *const legacy_names[] = {
    "infiltrator_filesystem_module_v1",
    "infiltrator_filesystem_module_v2",
    "infiltrator_filesystem_module_v3",
    "infiltrator_filesystem_module_v4",
    "infiltrator_filesystem_module_v5",
    "infiltrator_filesystem_module_v6",
    "infiltrator_filesystem_module_v7",
    "infiltrator_filesystem_module_v8"
};
bool infiltratr_filesystem_module_inspect_loaded(
    const InfiltratrDynlib *library,
    const char *expected_format_id,
    InfiltratrFilesystemModuleDiscovery *out)
{
    IfsFsModuleApiEntryV1 common_entry = NULL;
    if (!out) return false;
    out->common_api = NULL;
    out->legacy_table = NULL;
    out->legacy_version = 0u;
    if (!infiltratr_dynlib_is_open(library) ||
        !ifs_fs_format_id_valid(expected_format_id))
        return false;

    /* Found-but-invalid Common ABI is a hard error, not an invitation
     * to downgrade to a potentially different legacy implementation. */
    if (infiltratr_dynlib_symbol(library, IFS_FS_COMMON_EXPORT_V1,
                                 &common_entry, sizeof(common_entry))) {
        const IfsFsModuleApiV1 *api;
        if (!common_entry) return false;
        api = common_entry();
        if (!ifs_fs_common_api_valid(api, expected_format_id)) return false;
        out->common_api = api;
        return true;
    }

    /* Existing format-specific tables only share their first three fields.
     * Do not reinterpret native capability bits or function signatures. */
    for (size_t n = sizeof(legacy_names) / sizeof(legacy_names[0]);
         n > 0u; --n) {
        LegacyModuleEntry entry = NULL;
        const IfsFsLegacyPrefix *legacy;
        if (!infiltratr_dynlib_symbol(library, legacy_names[n - 1u],
                                      &entry, sizeof(entry)))
            continue;
        if (!entry) return false;
        legacy = entry();
        if (!ifs_fs_legacy_prefix_valid(legacy, (uint32_t)n,
                                        expected_format_id))
            return false;
        out->legacy_version = (uint32_t)n;
        out->legacy_table = legacy;
        return true;
    }
    return false;
}

bool infiltratr_filesystem_module_inspect_loaded_v2(
    const InfiltratrDynlib *library,
    const char *expected_format_id,
    InfiltratrFilesystemModuleDiscoveryV2 *out)
{
    IfsFsModuleApiEntryV2 entry = NULL;
    InfiltratrFilesystemModuleDiscovery v1 = {0};
    if (!out) return false;
    out->v2 = NULL;
    out->v1 = NULL;
    out->legacy_table = NULL;
    out->legacy_version = 0u;
    if (!infiltratr_dynlib_is_open(library) ||
        !ifs_fs_format_id_valid(expected_format_id))
        return false;

    /* V2 exists: it is either valid or the module is rejected. A broken V2
     * declaration can never silently select the old entry point. */
    if (infiltratr_dynlib_symbol(library, IFS_FS_COMMON_EXPORT_V2,
                                &entry, sizeof(entry))) {
        const IfsFsModuleApiV2 *api;
        if (!entry) return false;
        api = entry();
        if (!ifs_fs_api_v2_valid(api, expected_format_id)) return false;
        out->v2 = api;
        out->v1 = api->base_v1;
        return true;
    }
    /* Preserve the previously qualified V1 and opaque-legacy fallback.
     * The original ABI and search order remain entirely unchanged. */
    if (!infiltratr_filesystem_module_inspect_loaded(library,
                                                     expected_format_id, &v1))
        return false;
    out->v1 = v1.common_api;
    out->legacy_table = v1.legacy_table;
    out->legacy_version = v1.legacy_version;
    return true;
}

/* There is deliberately no legacy search path for new V2 consumers. */
bool infiltratr_filesystem_module_inspect_loaded_v2_native(
    const InfiltratrDynlib *library,
    const char *expected_format_id,
    InfiltratrFilesystemModuleDiscoveryV2 *out)
{
    InfiltratrFilesystemModuleDiscoveryV2 found = {0};
    if (!out) return false;
    out->v2 = NULL;
    out->v1 = NULL;
    out->legacy_table = NULL;
    out->legacy_version = 0u;
    if (!infiltratr_filesystem_module_inspect_loaded_v2(
            library, expected_format_id, &found) ||
        !ifs_fs_api_v2_native_valid(found.v2, expected_format_id))
        return false;
    *out = found;
    return true;
}
