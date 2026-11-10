/* SPDX-License-Identifier: GPL-3.0-or-later
 * Discover an independently authenticated, already-open Common V2 module.
 * There is one ABI and no alternative export, downgrade or fallback.
 */
#ifndef INFILTRATR_FILESYSTEM_MODULE_LOADER_H
#define INFILTRATR_FILESYSTEM_MODULE_LOADER_H
#include <stdbool.h>
#include <infiltratr/dynlib.h>
#include <infiltratr/filesystem_module_v2.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Verify the module architecture, signature, ownership and execution
 * profile BEFORE opening it. Keep the DSO loaded while using the table.
 * This function never opens a module or reads/writes a filesystem. */
bool infiltratr_filesystem_module_inspect_loaded_v2(
    const InfiltratrDynlib *library,
    const char *expected_format_id,
    const IfsFsModuleApiV2 **out);

#ifdef __cplusplus
}
#endif
#endif
