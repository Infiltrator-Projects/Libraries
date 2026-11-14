/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <infiltratr/filesystem_module_v2.h>
#include <assert.h>
#include <string.h>

static int read_at(void *ctx,uint64_t at,void *dest,uint32_t length) {
    (void)ctx;(void)at;
    if(dest && length) memset(dest,0,length);
    return 0;
}
static int write_at(void *ctx,uint64_t at,const void *src,uint32_t length) {
    (void)ctx;(void)at;(void)src;(void)length;
    return 0;
}
static int flush(void *ctx) {(void)ctx;return 0;}
static uint64_t clock_ns(void *ctx) {(void)ctx;return 1u;}
static void *alloc(void *ctx,size_t n) {
    static unsigned char bytes[4096];(void)ctx;
    return n<=sizeof(bytes)?bytes:NULL;
}
static void release(void *ctx,void *ptr) {(void)ctx;(void)ptr;}
static int probe(const IfsFsHostRuntimeV2 *host,uint32_t *variant) {
    if(!ifs_fs_host_v2_valid(host) || !variant)return IFS_FS_INVALID;
    *variant=12u;return IFS_FS_OK;
}
static int format(const IfsFsHostRuntimeV2 *host,uint32_t variant,
                  const char *label,uint32_t flags) {
    (void)label;
    if(!ifs_fs_host_v2_valid(host) || flags || variant!=12u)
        return IFS_FS_INVALID;
    return host->capabilities & IFS_FS_HOST_READ_ONLY ?
        IFS_FS_ACCESS:IFS_FS_OK;
}
static IfsFsCoreOpsV2 core={
    .header={IFS_FS_GROUP_ABI_V2,sizeof(IfsFsCoreOpsV2),
        IFS_FS_GROUP_CORE,0u,IFS_FS_CORE_PROBE},
    .probe=probe
};
static IfsFsVolumeOpsV2 volume={
    .header={IFS_FS_GROUP_ABI_V2,sizeof(IfsFsVolumeOpsV2),
        IFS_FS_GROUP_VOLUME,0u,IFS_FS_VOLUME_FORMAT},
    .format=format
};
static const IfsFsGroupHeaderV2 *query(
    uint32_t id,uint32_t min,uint32_t max) {
    if(min>IFS_FS_GROUP_ABI_V2 || max<IFS_FS_GROUP_ABI_V2)return NULL;
    if(id==IFS_FS_GROUP_CORE)return &core.header;
    if(id==IFS_FS_GROUP_VOLUME)return &volume.header;
    return NULL;
}
static int mounted_volume_query(void *mount_context, IfsFsVolumeInfoV2 *info)
{
    (void)mount_context;
    if (!info || info->structure_bytes < sizeof(*info)) return IFS_FS_INVALID;
    return IFS_FS_OK;
}

/* A descriptor must not hide live callbacks behind absent capability bits.
 * In particular, mount/open/dir/plan callback groups are indivisible. */
static int orphan_unmount(void *volume_context) {
    (void)volume_context;
    return IFS_FS_OK;
}
static int mock_open(void *volume_context,const char *path,uint32_t flags,void **file) {
    (void)volume_context;(void)path;(void)flags;(void)file;
    return IFS_FS_UNSUPPORTED;
}
static int mock_close(void *volume_context,void *file) {
    (void)volume_context;(void)file;
    return IFS_FS_OK;
}
static int mock_open_dir(void *volume_context,const char *path,void **cursor) {
    (void)volume_context;(void)path;(void)cursor;
    return IFS_FS_UNSUPPORTED;
}
static int mock_map(void *volume_context,const char *path,uint64_t first,
                    IfsFsExtentV2 *extents,uint32_t cap,uint32_t *written) {
    (void)volume_context;(void)path;(void)first;(void)extents;(void)cap;(void)written;
    return IFS_FS_UNSUPPORTED;
}
static void mock_release_plan(void *volume_context,void *plan) {
    (void)volume_context;(void)plan;
}

int main(void) {
    IfsFsModuleApiV2 api={
        .abi_version=IFS_FS_COMMON_ABI_V2,
        .structure_bytes=sizeof(IfsFsModuleApiV2),
        .format_id="fat12",
        .runtime_profiles=IFS_FS_PROFILE_HOSTED,
        .group_mask=IFS_FS_GROUP_BIT(IFS_FS_GROUP_CORE)|
                    IFS_FS_GROUP_BIT(IFS_FS_GROUP_VOLUME),
        .query_group=query
    };
    IfsFsHostRuntimeV2 host={0};
    uint32_t variant=0u;
    assert(ifs_fs_api_v2_valid(&api,"fat12"));
    assert(!ifs_fs_api_v2_valid(&api,"ntfs"));
    assert(ifs_fs_group_v2(&api,IFS_FS_GROUP_CORE)==&core.header);
    assert(ifs_fs_group_v2(&api,IFS_FS_GROUP_VOLUME)==&volume.header);
    api.group_mask=0;
    assert(!ifs_fs_api_v2_valid(&api,"fat12")); /* no CORE: reject */
    api.group_mask=IFS_FS_GROUP_BIT(IFS_FS_GROUP_CORE);
    assert(ifs_fs_api_v2_valid(&api,"fat12"));
    core.header.capabilities=IFS_FS_CORE_PROBE|IFS_FS_CORE_MOUNT;
    assert(!ifs_fs_api_v2_valid(&api,"fat12")); /* fake mount */
    core.header.capabilities=IFS_FS_CORE_PROBE;
    core.unmount=orphan_unmount;
    assert(!ifs_fs_api_v2_valid(&api,"fat12")); /* unadvertised unmount */
    core.unmount=NULL;
    assert(ifs_fs_api_v2_valid(&api,"fat12"));

    IfsFsHandlesV2 handles={0};
    handles.header=(IfsFsGroupHeaderV2){IFS_FS_GROUP_ABI_V2,
        sizeof(IfsFsHandlesV2),IFS_FS_GROUP_HANDLES,0u,IFS_FS_HANDLE_OPEN};
    handles.open=mock_open;
    handles.close=mock_close;
    assert(ifs_fs_group_v2_callbacks_valid(&handles.header));
    handles.open_dir=mock_open_dir;
    assert(!ifs_fs_group_v2_callbacks_valid(&handles.header)); /* orphan directory */
    handles.open_dir=NULL;
    handles.header.capabilities=IFS_FS_HANDLE_DIR;
    handles.open=NULL;
    assert(!ifs_fs_group_v2_callbacks_valid(&handles.header)); /* orphan close */

    IfsFsDefragOpsV2 defrag={0};
    defrag.header=(IfsFsGroupHeaderV2){IFS_FS_GROUP_ABI_V2,
        sizeof(IfsFsDefragOpsV2),IFS_FS_GROUP_DEFRAG,0u,IFS_FS_DEFRAG_MAP};
    defrag.map=mock_map;
    assert(ifs_fs_group_v2_callbacks_valid(&defrag.header));
    defrag.release_plan=mock_release_plan;
    assert(!ifs_fs_group_v2_callbacks_valid(&defrag.header)); /* orphan plan release */

    core.header.abi_version=1u;
    assert(!ifs_fs_api_v2_valid(&api,"fat12")); /* previous group version */
    core.header.abi_version=IFS_FS_GROUP_ABI_V2;
    api.group_mask|=IFS_FS_GROUP_BIT(IFS_FS_GROUP_VOLUME);
    volume.header.capabilities|=IFS_FS_VOLUME_REPAIR;
    assert(!ifs_fs_api_v2_valid(&api,"fat12")); /* fake repair */
    volume.header.capabilities=IFS_FS_VOLUME_FORMAT;
    assert(ifs_fs_api_v2_valid(&api,"fat12"));
    volume.query=mounted_volume_query;
    volume.header.capabilities|=IFS_FS_VOLUME_QUERY;
    assert(!ifs_fs_api_v2_valid(&api,"fat12")); /* no CORE mount lifecycle */
    volume.query=NULL;
    volume.header.capabilities=IFS_FS_VOLUME_FORMAT;
    assert(ifs_fs_api_v2_valid(&api,"fat12")); /* standalone format is valid */
    api.runtime_profiles=UINT64_C(1)<<17;
    assert(!ifs_fs_api_v2_valid(&api,"fat12"));
    api.runtime_profiles=IFS_FS_PROFILE_HOSTED;
    api.abi_version=1u;
    assert(!ifs_fs_api_v2_valid(&api,"fat12"));
    api.abi_version=IFS_FS_COMMON_ABI_V2;

    host.abi_version=IFS_FS_HOST_ABI_V2;
    host.structure_bytes=sizeof(host);
    host.capabilities=IFS_FS_HOST_READ_ONLY;
    host.volume_bytes=1024u*1024u;
    host.logical_sector_bytes=512u;
    host.physical_sector_bytes=4096u;
    host.maximum_transfer_bytes=65536u;
    host.read_at=read_at;
    host.allocate=alloc;
    host.release=release;
    assert(ifs_fs_host_v2_valid(&host));
    host.monotonic_ns=clock_ns;
    assert(!ifs_fs_host_v2_valid(&host)); /* unadvertised orphan clock */
    host.monotonic_ns=NULL;
    host.realtime_ns=clock_ns;
    assert(!ifs_fs_host_v2_valid(&host)); /* opposite orphan clock */
    host.capabilities|=IFS_FS_HOST_CLOCK;
    assert(!ifs_fs_host_v2_valid(&host)); /* advertised but incomplete clocks */
    host.monotonic_ns=clock_ns;
    assert(ifs_fs_host_v2_valid(&host));
    host.capabilities&=~IFS_FS_HOST_CLOCK;
    host.monotonic_ns=NULL;host.realtime_ns=NULL;
    assert(ifs_fs_host_v2_valid(&host));
    assert(core.probe(&host,&variant)==IFS_FS_OK && variant==12u);
    assert(volume.format(&host,12u,NULL,0u)==IFS_FS_ACCESS);
    host.capabilities=IFS_FS_HOST_EXCLUSIVE|IFS_FS_HOST_DURABLE;
    assert(!ifs_fs_host_v2_valid(&host)); /* incomplete exclusive lease */
    host.write_at=write_at;
    host.flush=flush;
    assert(ifs_fs_host_v2_valid(&host));
    assert(volume.format(&host,12u,NULL,0u)==IFS_FS_OK);
    host.capabilities=IFS_FS_HOST_READ_ONLY;
    assert(!ifs_fs_host_v2_valid(&host)); /* no writes on RO lease */
    host.write_at=NULL;host.flush=NULL;
    assert(ifs_fs_host_v2_valid(&host));
    host.logical_sector_bytes=1000u;
    assert(!ifs_fs_host_v2_valid(&host)); /* invalid geometry */
    return 0;
}
