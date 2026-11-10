/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <infiltratr/filesystem_module_loader.h>
#include <string.h>

/* Fail closed. The caller has already validated the DSO's authenticity and
 * architecture before opening it. Only one Common V2 export is accepted. */
bool infiltratr_filesystem_module_inspect_loaded_v2(
    const InfiltratrDynlib *library,
    const char *expected_format_id,
    const IfsFsModuleApiV2 **out)
{
    IfsFsModuleApiEntryV2 entry = NULL;
    const IfsFsModuleApiV2 *module;
    if (!out) return false;
    *out = NULL;
    if (!infiltratr_dynlib_is_open(library) ||
        !ifs_fs_format_id_valid(expected_format_id) ||
        !infiltratr_dynlib_symbol(library, IFS_FS_COMMON_EXPORT_V2,
                                 &entry, sizeof(entry)) ||
        !entry)
        return false;
    module = entry();
    if (!ifs_fs_api_v2_valid(module, expected_format_id)) return false;
    *out = module;
    return true;
}
