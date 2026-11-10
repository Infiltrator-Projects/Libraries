/* SPDX-License-Identifier: GPL-3.0-or-later
 * Format-neutral discovery of an ALREADY OPENED, TRUST-VERIFIED module.
 */
#ifndef INFILTRATR_FILESYSTEM_MODULE_LOADER_H
#define INFILTRATR_FILESYSTEM_MODULE_LOADER_H
#include <stdbool.h>
#include <stdint.h>
#include <infiltratr/dynlib.h>
#include <infiltratr/filesystem_module.h>
#include <infiltratr/filesystem_module_v2.h>
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
/* V2 discovery is additive: the old discovery structure and exported
 * function remain ABI-immutable for existing consumers. */
typedef struct {
    const IfsFsModuleApiV2 *v2; /* native V2 descriptor, if present */
    const IfsFsModuleApiV1 *v1; /* verified base, possibly supplied by V2 */
    const IfsFsLegacyPrefix *legacy_table;
    uint32_t legacy_version;
} InfiltratrFilesystemModuleDiscoveryV2;

/* Inspect an already authenticated/loaded binary. V2 is authoritative if
 * its export exists: invalid V2 NEVER falls back to V1 or legacy tables.
 * This does no disk I/O, module opening, or mutation. */
bool infiltratr_filesystem_module_inspect_loaded_v2(
    const InfiltratrDynlib *library,
    const char *expected_format_id,
    InfiltratrFilesystemModuleDiscoveryV2 *out);
/* Final migration path: accept genuine V2-only providers, never V1/legacy
 * fallbacks and never a V2 descriptor whose base_v1 remains populated.
 * This only inspects an already trusted, opened binary. */
bool infiltratr_filesystem_module_inspect_loaded_v2_native(
    const InfiltratrDynlib *library,
    const char *expected_format_id,
    InfiltratrFilesystemModuleDiscoveryV2 *out);
#ifdef __cplusplus
}
#endif
#endif
