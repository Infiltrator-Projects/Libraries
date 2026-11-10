/* SPDX-License-Identifier: GPL-3.0-or-later
 * Strict Common V2 loader fixture.
 */
#include <infiltratr/filesystem_module_v2.h>

static int probe(const IfsFsHostRuntimeV2 *host, uint32_t *variant)
{
    if (!ifs_fs_host_v2_valid(host) || !variant) return IFS_FS_INVALID;
    *variant = 12u;
    return IFS_FS_OK;
}
static const IfsFsCoreOpsV2 core = {
    .header = {IFS_FS_GROUP_ABI_V2, sizeof(IfsFsCoreOpsV2),
               IFS_FS_GROUP_CORE, 0u, IFS_FS_CORE_PROBE},
    .probe = probe
};
static const IfsFsGroupHeaderV2 *query(
    uint32_t group_id, uint32_t minimum, uint32_t maximum)
{
    return group_id == IFS_FS_GROUP_CORE &&
        minimum <= IFS_FS_GROUP_ABI_V2 &&
        maximum >= IFS_FS_GROUP_ABI_V2 ? &core.header : NULL;
}
static const IfsFsModuleApiV2 api = {
    .abi_version = IFS_FS_COMMON_ABI_V2,
    .structure_bytes = sizeof(IfsFsModuleApiV2),
    .format_id = "fat12",
    .runtime_profiles = IFS_FS_PROFILE_HOSTED,
#if defined(IFS_FIXTURE_INVALID_V2)
    .group_mask = IFS_FS_GROUP_BIT(IFS_FS_GROUP_CORE) |
                  IFS_FS_GROUP_BIT(IFS_FS_GROUP_HANDLES),
#else
    .group_mask = IFS_FS_GROUP_BIT(IFS_FS_GROUP_CORE),
#endif
    .query_group = query
};
const IfsFsModuleApiV2 *infiltrator_filesystem_api_v2(void)
{
    return &api;
}
