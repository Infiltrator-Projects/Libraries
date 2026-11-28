/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <infiltratr/filesystem_block_map_v2.h>
#include <stdio.h>
#define CHECK(test) do { if (!(test)) { fprintf(stderr, "%d: %s\n", __LINE__, #test); return 1; } } while (0)

static uint32_t state(const void *volume,uint64_t block)
{
    (void)volume;
    return block==1 ? IFS_FS_SPACE_FREE : IFS_FS_SPACE_ALLOCATED;
}
static int cancelled(void *context) { (void)context; return 1; }

int main(void)
{
    IfsFsHostRuntimeV2 host = {.volume_bytes=4096};
    IfsFsAllocationRangeV2 ranges[5];
    uint32_t n;
    CHECK(ifs_fs_block_map_page_v2(&host,512,3,512,&host,state,7,ranges,5,&n)==0 && n==5);
    CHECK(ranges[0].physical_offset==7 && ranges[0].length==505 && ranges[0].state==IFS_FS_SPACE_RESERVED);
    CHECK(ranges[2].physical_offset==1024 && ranges[2].length==512 && ranges[2].state==IFS_FS_SPACE_FREE);
    CHECK(ranges[4].physical_offset==2048 && ranges[4].length==2048 && ranges[4].state==IFS_FS_SPACE_RESERVED);
    CHECK(ifs_fs_block_map_page_v2(&host,512,3,512,&host,state,1600,ranges,1,&n)==0 && n==1 && ranges[0].length==448);
    CHECK(ifs_fs_block_map_page_v2(&host,512,3,512,&host,state,4096,ranges,1,&n)==IFS_FS_END && !n);
    CHECK(ifs_fs_block_map_page_v2(&host,512,UINT64_MAX,512,&host,state,0,ranges,1,&n)==IFS_FS_CORRUPT && !n);
    host.volume_bytes=UINT64_MAX;
    CHECK(ifs_fs_block_map_page_v2(&host,UINT64_MAX-512,1,512,&host,state,UINT64_MAX-2,ranges,1,&n)==0 && n==1 && ranges[0].length==2);
    host.cancelled=cancelled;
    CHECK(ifs_fs_block_map_page_v2(&host,0,1,512,&host,state,0,ranges,1,&n)==IFS_FS_BUSY && !n);
    return 0;
}
