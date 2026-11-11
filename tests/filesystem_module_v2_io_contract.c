/* SPDX-License-Identifier: GPL-3.0-or-later
 * Prove strict V2 metadata byte reads only invoke sector-aligned host I/O.
 */
#include <infiltratr/filesystem_module_v2.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

enum { MEDIUM = 4 * 65536 };
typedef struct {
    uint8_t image[MEDIUM];
    uint32_t sector, maximum, calls;
    uint64_t fail_at;
    int fail;
} Media;
static int read_at(void *ctx, uint64_t at, void *out, uint32_t bytes)
{
    Media *m = (Media *)ctx;
    assert(!(at % m->sector));
    assert(!(bytes % m->sector));
    assert(bytes && bytes <= m->maximum);
    assert(at <= MEDIUM && bytes <= MEDIUM - at);
    ++m->calls;
    if (m->fail && at == m->fail_at) return -1;
    memcpy(out, m->image + (size_t)at, bytes);
    return 0;
}
static void *allocate(void *ctx,size_t bytes)
{
    (void)ctx;
    return malloc(bytes);
}
static void release(void *ctx,void *ptr)
{
    (void)ctx;
    free(ptr);
}
static void run_sector(uint32_t sector, uint32_t maximum)
{
    Media *m = calloc(1u,sizeof(*m));
    uint8_t result[9600], snapshot[9600];
    IfsFsHostRuntimeV2 h = {0};
    uint32_t i;
    assert(m);
    m->sector = sector;
    m->maximum = maximum;
    for (i=0;i<MEDIUM;++i)m->image[i]=(uint8_t)(i*41u+17u);
    h.abi_version=IFS_FS_HOST_ABI_V2;
    h.structure_bytes=sizeof(h);
    h.capabilities=IFS_FS_HOST_READ_ONLY;
    h.volume_bytes=MEDIUM;
    h.logical_sector_bytes=sector;
    h.physical_sector_bytes=sector;
    h.maximum_transfer_bytes=maximum;
    h.context=m;
    h.read_at=read_at;
    h.allocate=allocate;
    h.release=release;
    assert(ifs_fs_host_v2_valid(&h));
    assert(ifs_fs_host_read_bytes_v2(&h,1024u,result,512u)==IFS_FS_OK);
    assert(memcmp(result,m->image+1024u,512u)==0);
    assert(m->calls>0u);
    assert(ifs_fs_host_read_bytes_v2(&h,4090u,result,9000u)==IFS_FS_OK);
    assert(memcmp(result,m->image+4090u,9000u)==0);
    assert(ifs_fs_host_read_bytes_v2(&h,0u,result,8192u)==IFS_FS_OK);
    assert(memcmp(result,m->image,8192u)==0);
    memset(snapshot,0x5a,sizeof(snapshot));
    memcpy(result,snapshot,sizeof(result));
    assert(ifs_fs_host_read_bytes_v2(&h,MEDIUM-32u,result,33u)==IFS_FS_INVALID);
    assert(memcmp(result,snapshot,sizeof(result))==0);
    assert(ifs_fs_host_read_bytes_v2(&h,0u,NULL,1u)==IFS_FS_INVALID);
    assert(ifs_fs_host_read_bytes_v2(&h,MEDIUM,NULL,0u)==IFS_FS_OK);
    m->fail=1;
    m->fail_at=1024u - (1024u % sector);
    assert(ifs_fs_host_read_bytes_v2(&h,1024u,result,512u)==IFS_FS_IO_ERROR);
    m->fail=0;
    if(sector==65536u) {
        h.volume_bytes=65000u;
        assert(ifs_fs_host_read_bytes_v2(&h,1024u,result,512u)==IFS_FS_INVALID);
    }
    free(m);
}
int main(void)
{
    run_sector(512u,512u);
    run_sector(512u,4096u);
    run_sector(4096u,4096u);
    run_sector(4096u,8192u);
    run_sector(65536u,65536u);
    return 0;
}
