/* SPDX-License-Identifier: GPL-3.0-or-later
 * Common V2 pagination for a provider's verified, fixed-unit allocator.
 * Each provider interprets its own native allocator and supplies the state
 * of a unit. Prefix and tail bytes outside that allocator are reserved.
 */
#ifndef INFILTRATR_FILESYSTEM_BLOCK_MAP_V2_H
#define INFILTRATR_FILESYSTEM_BLOCK_MAP_V2_H
#include <infiltratr/filesystem_module_v2.h>

typedef uint32_t (*IfsFsBlockStateV2)(const void *, uint64_t);

static inline int ifs_fs_block_map_page_v2(const IfsFsHostRuntimeV2 *host,
    uint64_t base, uint64_t blocks, uint32_t unit, const void *volume,
    IfsFsBlockStateV2 classify, uint64_t cursor, IfsFsAllocationRangeV2 *out,
    uint32_t capacity, uint32_t *written)
{
    if (!written) return IFS_FS_INVALID;
    *written = 0;
    if (!host || !unit || !classify || !volume || !out || !capacity ||
        cursor > host->volume_bytes) return IFS_FS_INVALID;
    if (base > host->volume_bytes || blocks > (host->volume_bytes-base)/unit)
        return IFS_FS_CORRUPT;
    if (cursor == host->volume_bytes) return IFS_FS_END;
    uint64_t limit = base + blocks * unit;
    uint32_t count = 0;
    /* Bound work per page even on a terabyte-long homogeneous run. A
     * consumer resumes at the byte after the last result. Adjacent pages
     * may consequently meet in the middle of one allocation run. */
    for (unsigned visited=0; cursor<host->volume_bytes && visited<65536u; ++visited) {
        if ((visited % 256u)==0 && host->cancelled && host->cancelled(host->context))
            return IFS_FS_BUSY;
        uint32_t state = IFS_FS_SPACE_RESERVED;
        uint64_t end;
        if (cursor < base) end = base;
        else if (cursor >= limit) end = host->volume_bytes;
        else {
            uint64_t block = (cursor-base)/unit;
            end = base+(block+1)*unit;
            state = classify(volume, block);
            if (state > IFS_FS_SPACE_RESERVED) return IFS_FS_CORRUPT;
        }
        if (count && out[count-1].state==state) out[count-1].length+=end-cursor;
        else {
            if (count==capacity) break;
            out[count++] = (IfsFsAllocationRangeV2){cursor,end-cursor,state,0};
        }
        cursor=end;
    }
    *written=count;
    return IFS_FS_OK;
}
#endif
