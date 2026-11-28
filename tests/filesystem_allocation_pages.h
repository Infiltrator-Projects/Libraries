/* SPDX-License-Identifier: GPL-3.0-or-later
 * Cross-check each reported block against the provider test's on-disk fixture
 * encoder. Starting inside a block exercises byte-cursor continuation.
 */
#ifndef INFILTRATR_TEST_FILESYSTEM_ALLOCATION_PAGES_H
#define INFILTRATR_TEST_FILESYSTEM_ALLOCATION_PAGES_H
#include <infiltratr/filesystem_module_v2.h>
#include <assert.h>

static void check_allocation_pages(const IfsFsAllocationOpsV2 *ops, void *volume,
    uint32_t unit, uint64_t bytes, uint64_t free_bytes, int (*used)(uint64_t))
{
    uint64_t cursor=0, free_total=0;
    IfsFsAllocationRangeV2 page[3];
    uint32_t n;
    assert(ops && ops->header.capabilities==IFS_FS_ALLOCATION_MAP);
    while (cursor<bytes) {
        assert(ops->map(volume,cursor,page,3,&n)==IFS_FS_OK && n && n<=3);
        for (uint32_t i=0;i<n;++i) {
            assert(page[i].physical_offset==cursor && !page[i].reserved);
            assert(page[i].length && page[i].length<=bytes-cursor && page[i].length%unit==0);
            for (uint64_t b=cursor/unit; b<(cursor+page[i].length)/unit; ++b)
                assert(page[i].state==(used(b)?IFS_FS_SPACE_ALLOCATED:IFS_FS_SPACE_FREE));
            if (page[i].state==IFS_FS_SPACE_FREE) free_total+=page[i].length;
            cursor+=page[i].length;
        }
    }
    assert(free_total==free_bytes);
    assert(ops->map(volume,bytes,page,1,&n)==IFS_FS_END && !n);
    assert(ops->map(volume,bytes+1,page,1,&n)==IFS_FS_INVALID && !n);
    assert(ops->map(volume,unit+7,page,1,&n)==IFS_FS_OK && n==1);
    assert(page[0].physical_offset==unit+7 && (page[0].length+7)%unit==0);
}
#endif
