/* SPDX-License-Identifier: GPL-3.0-or-later
 * An authenticated-loader test fixture; not a filesystem implementation.
 */
#include <infiltratr/filesystem_module_v2.h>
static int probe(const IfsFsHostIoV1 *host, uint32_t *variant) {
    (void)host;if (variant) *variant=12u;return IFS_FS_OK;
}
static const IfsFsModuleApiV1 base={
    .abi_version=IFS_FS_COMMON_ABI_V1,
    .structure_bytes=sizeof(IfsFsModuleApiV1),
    .format_id="fat12", .capabilities=IFS_FS_CAP_PROBE, .probe=probe
};
const IfsFsModuleApiV1 *infiltrator_filesystem_api_v1(void){return &base;}
static const IfsFsModuleApiV2 api={
    .abi_version=IFS_FS_COMMON_ABI_V2,
    .structure_bytes=sizeof(IfsFsModuleApiV2),
    .format_id="fat12",
    .runtime_profiles=IFS_FS_PROFILE_HOSTED,
#if defined(IFS_FIXTURE_INVALID_V2)
    .group_mask=IFS_FS_GROUP_BIT(IFS_FS_GROUP_HANDLES),
#else
    .group_mask=0,
#endif
    .base_v1=&base, .query_group=NULL
};
const IfsFsModuleApiV2 *infiltrator_filesystem_api_v2(void){return &api;}
