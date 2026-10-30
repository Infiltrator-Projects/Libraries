/* SPDX-License-Identifier: GPL-3.0-or-later
 * Format-neutral discovery of an ALREADY OPENED, TRUST-VERIFIED module.
 */
#ifndef INFILTRATR_FILESYSTEM_MODULE_LOADER_H
#define INFILTRATR_FILESYSTEM_MODULE_LOADER_H
#include <stdbool.h>
#include <stdint.h>
#include <infiltratr/dynlib.h>
#include <infiltratr/filesystem_module.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    const IfsFsModuleApiV1 *common_api; /* NULL for legacy-only modules */
    const IfsFsLegacyPrefix *legacy_table; /* NULL for Common API modules */
    uint32_t legacy_version; /* zero when common_api is present */
} InfiltratrFilesystemModuleDiscovery;

/* Host performs independent metadata/signature/architecture verification
 * BEFORE opening the .so. This function never opens a file, performs disk
 * writes, runs a filesystem operation or constructs format-specific shims.
 *
 * Discovers the uniform Common API first. For older modules, selects the
 * highest exported infiltrator_filesystem_module_vN (currently up to V8)
 * and validates its shared {version,size,format_id} prefix. Crucially,
 * its native operation table and capability bits remain OPAQUE.
 *
 * The caller MUST keep 'library' open until all pointers are discarded.
 * Failure clears 'out'. Unsupported or mismatched versions fail closed.
 */
bool infiltratr_filesystem_module_inspect_loaded(
    const InfiltratrDynlib *library,
    const char *expected_format_id,
    InfiltratrFilesystemModuleDiscovery *out);
#ifdef __cplusplus
}
#endif
#endif
