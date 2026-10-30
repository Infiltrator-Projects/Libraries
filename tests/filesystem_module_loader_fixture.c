/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <infiltratr/filesystem_module.h>
#ifdef IFS_FIXTURE_COMMON
static int probe(const IfsFsHostIoV1 *io, uint32_t *variant)
{
    (void)io; if (variant) *variant = 0; return IFS_FS_OK;
}
static const IfsFsModuleApiV1 module = {
    .abi_version = 1u, .structure_bytes = sizeof(IfsFsModuleApiV1),
    .format_id = "fat12", .capabilities = IFS_FS_CAP_PROBE,
    .probe = probe
};
const IfsFsModuleApiV1 *infiltrator_filesystem_api_v1(void)
{ return &module; }
#else
/* V1 and V3 imitate the first three fields in existing module families. */
static const IfsFsLegacyPrefix older = {1u, sizeof(IfsFsLegacyPrefix), "ofs"};
static const IfsFsLegacyPrefix newest = {3u, sizeof(IfsFsLegacyPrefix), "ofs"};
const IfsFsLegacyPrefix *infiltrator_filesystem_module_v1(void)
{ return &older; }
const IfsFsLegacyPrefix *infiltrator_filesystem_module_v3(void)
{ return &newest; }
#endif
