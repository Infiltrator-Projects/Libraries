/* SPDX-License-Identifier: GPL-3.0-or-later
 * Optional, size-negotiated tail on the canonical VOLUME group. Namespace-free
 * storage formats can expose offline management without inventing file handles
 * or passing a host pointer as a mounted volume. Older V2 hosts read the normal
 * volume prefix, including its unchanged capabilities, and ignore the tail.
 */
#ifndef INFILTRATR_FILESYSTEM_MEDIA_V2_H
#define INFILTRATR_FILESYSTEM_MEDIA_V2_H
#include <infiltratr/filesystem_module_v2.h>

#define IFS_FS_MEDIA_INSPECT    (UINT64_C(1) << 0)
#define IFS_FS_MEDIA_MAP        (UINT64_C(1) << 1)
#define IFS_FS_MEDIA_SET_LABEL  (UINT64_C(1) << 2)
#define IFS_FS_MEDIA_RESIZE     (UINT64_C(1) << 3)
#define IFS_FS_MEDIA_CHECK      (UINT64_C(1) << 4)
#define IFS_FS_MEDIA_REPAIR     (UINT64_C(1) << 5)
#define IFS_FS_MEDIA_RETIRE     (UINT64_C(1) << 6)
#define IFS_FS_MEDIA_TRIM       (UINT64_C(1) << 7)
#define IFS_FS_MEDIA_ALL        ((UINT64_C(1) << 8) - 1u)

#define IFS_FS_MEDIA_HAS_LABEL  (UINT32_C(1) << 0)
#define IFS_FS_MEDIA_HAS_UUID   (UINT32_C(1) << 1)
#define IFS_FS_MEDIA_HAS_FREE   (UINT32_C(1) << 2)
typedef struct {
    uint32_t structure_bytes, variant, unit_bytes, flags;
    uint64_t managed_bytes, usable_bytes, free_bytes, unusable_units;
    uint8_t uuid[16];
    uint32_t byte_order, label_bytes;
    uint8_t label[256];
} IfsFsMediaInfoV2;

/* Zero requests automatic recognition. A nonzero variant is an explicit
 * producer/layout hint for historical media with no discriminator. */
typedef struct {
    const IfsFsHostRuntimeV2 *host;
    uint32_t variant;
} IfsFsMediaTargetV2;
#define IFS_FS_MEDIA_ORDER_UNSPECIFIED 0u
#define IFS_FS_MEDIA_ORDER_LITTLE 1u
#define IFS_FS_MEDIA_ORDER_BIG 2u

typedef struct {
    IfsFsVolumeOpsV2 volume;
    uint32_t media_abi, media_reserved;
    uint64_t media_capabilities;
    int (*inspect)(const IfsFsMediaTargetV2 *, IfsFsMediaInfoV2 *);
    int (*map)(const IfsFsMediaTargetV2 *, uint64_t first_byte,
               IfsFsAllocationRangeV2 *, uint32_t capacity, uint32_t *written);
    int (*set_label)(const IfsFsMediaTargetV2 *, const uint8_t *, uint32_t bytes);
    int (*resize)(const IfsFsMediaTargetV2 *, uint64_t new_managed_bytes);
    int (*check)(const IfsFsMediaTargetV2 *, uint64_t *errors);
    int (*repair)(const IfsFsMediaTargetV2 *, uint64_t *repairs);
    int (*retire)(const IfsFsMediaTargetV2 *, uint64_t offset, uint64_t bytes);
    int (*trim)(const IfsFsMediaTargetV2 *, uint64_t offset, uint64_t bytes);
} IfsFsVolumeMediaOpsV2;

/* Never inspect the extension of a base-only provider, including providers
 * compiled against older releases. A capability is valid only if its callback
 * exists. Per-format applicability is established by inspecting real media. */
static inline const IfsFsVolumeMediaOpsV2 *
ifs_fs_volume_media_v2(const IfsFsModuleApiV2 *api)
{
    const IfsFsGroupHeaderV2 *group=ifs_fs_group_v2(api,IFS_FS_GROUP_VOLUME);
    if (!group || group->structure_bytes<sizeof(IfsFsVolumeMediaOpsV2)) return NULL;
    const IfsFsVolumeMediaOpsV2 *p=(const IfsFsVolumeMediaOpsV2 *)group;
    uint64_t mask=p->media_capabilities;
    if (p->media_abi!=IFS_FS_GROUP_ABI_V2 || p->media_reserved || !mask ||
        (mask&~IFS_FS_MEDIA_ALL)) return NULL;
#define IFS_MEDIA_MATCH(bit, callback) (((mask&(bit))!=0)==(p->callback!=NULL))
    if (!IFS_MEDIA_MATCH(IFS_FS_MEDIA_INSPECT,inspect) ||
        !IFS_MEDIA_MATCH(IFS_FS_MEDIA_MAP,map) ||
        !IFS_MEDIA_MATCH(IFS_FS_MEDIA_SET_LABEL,set_label) ||
        !IFS_MEDIA_MATCH(IFS_FS_MEDIA_RESIZE,resize) ||
        !IFS_MEDIA_MATCH(IFS_FS_MEDIA_CHECK,check) ||
        !IFS_MEDIA_MATCH(IFS_FS_MEDIA_REPAIR,repair) ||
        !IFS_MEDIA_MATCH(IFS_FS_MEDIA_RETIRE,retire) ||
        !IFS_MEDIA_MATCH(IFS_FS_MEDIA_TRIM,trim)) return NULL;
#undef IFS_MEDIA_MATCH
    return p;
}
#endif
