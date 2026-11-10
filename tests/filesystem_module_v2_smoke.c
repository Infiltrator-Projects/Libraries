/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <infiltratr/filesystem_module_v2.h>
#include <assert.h>
#include <string.h>

static int probe(const IfsFsHostIoV1 *h, uint32_t *v) {
    (void)h; if (v) *v=12u;return IFS_FS_OK;
}
static int host_read(void *ctx,uint64_t at,void *b,uint32_t n) {
    (void)ctx;(void)at;(void)b;(void)n;return 0;
}
static int host_write(void *ctx,uint64_t at,const void *b,uint32_t n) {
    (void)ctx;(void)at;(void)b;(void)n;return 0;
}
static int flush(void *ctx){(void)ctx;return 0;}
static void *allocate(void *ctx,size_t bytes){(void)ctx;(void)bytes;return (void *)1;}
static void release(void *ctx,void *p){(void)ctx;(void)p;}
static int scrub(void *v,uint32_t f,uint64_t *errors){
    (void)v;(void)f;if(errors)*errors=0;return IFS_FS_OK;
}
static const IfsFsModuleApiV1 v1={
    .abi_version=IFS_FS_COMMON_ABI_V1,.structure_bytes=sizeof(IfsFsModuleApiV1),
    .format_id="fat12",.capabilities=IFS_FS_CAP_PROBE,.probe=probe
};
static IfsFsVolumeOpsV1 volume={
    .header={IFS_FS_GROUP_ABI_V1,sizeof(IfsFsVolumeOpsV1),IFS_FS_GROUP_VOLUME,0u,
             IFS_FS_VOLUME_SCRUB},
    .scrub=scrub
};
static const IfsFsGroupHeaderV2 *query(uint32_t id,uint32_t lo,uint32_t hi){
    if(id!=IFS_FS_GROUP_VOLUME || lo>1u || hi<1u)return NULL;
    return &volume.header;
}
int main(void) {
    IfsFsHostRuntimeV2 h={0};
    IfsFsModuleApiV2 a={IFS_FS_COMMON_ABI_V2,sizeof(a),"fat12",
        IFS_FS_PROFILE_HOSTED,IFS_FS_GROUP_BIT(IFS_FS_GROUP_VOLUME),&v1,query};
    assert(sizeof(IfsFsHostIoV1)==offsetof(IfsFsHostFormatIoV1,logical_sector_bytes));
    assert(ifs_fs_api_v2_valid(&a,"fat12"));
    assert(!ifs_fs_api_v2_valid(&a,"ntfs"));
    assert(ifs_fs_group_v2(&a,IFS_FS_GROUP_HANDLES)==NULL);
    assert(ifs_fs_group_v2(&a,IFS_FS_GROUP_VOLUME)==&volume.header);
    assert(query(IFS_FS_GROUP_VOLUME,2u,2u)==NULL);
    a.group_mask|=IFS_FS_GROUP_BIT(IFS_FS_GROUP_HANDLES);
    assert(!ifs_fs_api_v2_valid(&a,"fat12"));
    a.group_mask=IFS_FS_GROUP_BIT(IFS_FS_GROUP_VOLUME);
    volume.header.capabilities|=IFS_FS_VOLUME_REPAIR;
    assert(!ifs_fs_api_v2_valid(&a,"fat12")); /* fake repair rejected */
    volume.header.capabilities=IFS_FS_VOLUME_SCRUB;
    volume.header.structure_bytes=sizeof(IfsFsGroupHeaderV2);
    assert(!ifs_fs_api_v2_valid(&a,"fat12")); /* undersized group rejected */
    volume.header.structure_bytes=sizeof(volume);
    volume.header.group_id=IFS_FS_GROUP_HANDLES;
    assert(!ifs_fs_api_v2_valid(&a,"fat12")); /* wrong group rejected */
    volume.header.group_id=IFS_FS_GROUP_VOLUME;
    a.runtime_profiles=IFS_FS_PROFILE_KERNEL|UINT64_C(0x100);
    assert(!ifs_fs_api_v2_valid(&a,"fat12"));
    a.runtime_profiles=IFS_FS_PROFILE_HOSTED;
    a.group_mask=0;assert(!ifs_fs_api_v2_valid(&a,"fat12"));
    a.query_group=NULL;assert(ifs_fs_api_v2_valid(&a,"fat12"));
    a.base_v1=NULL;assert(!ifs_fs_api_v2_valid(&a,"fat12"));
    a.base_v1=&v1;assert(ifs_fs_api_v2_valid(&a,"fat12"));
    a.abi_version=1u;assert(!ifs_fs_api_v2_valid(&a,"fat12"));

    h.abi_version=IFS_FS_HOST_ABI_V2;h.structure_bytes=sizeof(h);
    h.capabilities=IFS_FS_HOST_READ_ONLY;
    h.volume_bytes=1024u*1024u;h.logical_sector_bytes=512u;
    h.physical_sector_bytes=4096u;h.maximum_transfer_bytes=65536u;
    h.read_at=host_read;h.allocate=allocate;h.release=release;
    assert(ifs_fs_host_v2_valid(&h));
    h.capabilities=IFS_FS_HOST_EXCLUSIVE|IFS_FS_HOST_DURABLE;
    assert(!ifs_fs_host_v2_valid(&h)); /* no write lease */
    h.write_at=host_write;h.flush=flush;
    assert(ifs_fs_host_v2_valid(&h));
    h.capabilities=IFS_FS_HOST_EXCLUSIVE;
    assert(!ifs_fs_host_v2_valid(&h)); /* missing durability */
    h.capabilities=IFS_FS_HOST_READ_ONLY;
    assert(!ifs_fs_host_v2_valid(&h)); /* read-only with write callback */
    h.write_at=NULL;h.flush=NULL;
    assert(ifs_fs_host_v2_valid(&h));
    h.logical_sector_bytes=1000u;
    assert(!ifs_fs_host_v2_valid(&h)); /* untrusted sector geometry */
    h.logical_sector_bytes=512u;h.reserved=1u;
    assert(!ifs_fs_host_v2_valid(&h));
    h.reserved=0u;
    {
        IfsFsHostIoV1 old={0};
        IfsFsHostFormatIoV1 formatting={0};
        assert(ifs_fs_host_v2_as_v1(&h,&old)==IFS_FS_OK);
        assert(old.structure_bytes==sizeof(old));
        assert(old.flags==IFS_FS_IO_READ_ONLY);
        assert(old.read_at==host_read && old.write_at==NULL && old.flush==NULL);
        assert(ifs_fs_host_v2_as_format_v1(&h,&formatting)==IFS_FS_OK);
        assert(formatting.host.structure_bytes==sizeof(formatting));
        assert(formatting.logical_sector_bytes==512u && formatting.reserved==0u);
        h.capabilities=IFS_FS_HOST_EXCLUSIVE|IFS_FS_HOST_DURABLE;
        h.write_at=host_write; h.flush=flush;
        assert(ifs_fs_host_v2_as_format_v1(&h,&formatting)==IFS_FS_OK);
        assert(formatting.host.flags==IFS_FS_IO_EXCLUSIVE);
        assert(formatting.host.write_at==host_write && formatting.host.flush==flush);
        h.capabilities=IFS_FS_HOST_EXCLUSIVE;
        assert(ifs_fs_host_v2_as_v1(&h,&old)==IFS_FS_INVALID);
        h.capabilities=IFS_FS_HOST_EXCLUSIVE|IFS_FS_HOST_DURABLE;
    }
    return 0;
}
