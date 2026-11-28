/* SPDX-License-Identifier: GPL-3.0-or-later
 * Black-box qualification through a production Common V2 shared object.
 * Each disposable disk deliberately uses the same path with different bytes.
 * The optional second module can be its read-only freestanding BOOT profile.
 */
#define _POSIX_C_SOURCE 200809L
#include <infiltratr/filesystem_module_v2.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define REQUIRE(x) do { if (!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x); exit(1); } } while (0)
typedef struct {
    FILE *file;
    uint32_t sector;
    uint64_t size;
    unsigned writes, allocations;
    int fail_allocation;
} Image;
typedef struct {
    void *library;
    const IfsFsCoreOpsV2 *core;
    const IfsFsVolumeOpsV2 *volume;
} Provider;

static int aligned(const Image *disk,uint64_t at,uint32_t bytes)
{
    return !(at%disk->sector) && !(bytes%disk->sector) && at<=disk->size &&
           bytes<=disk->size-at && bytes<=65536u;
}
static int read_at(void *ctx,uint64_t at,void *p,uint32_t n)
{
    Image *d=ctx;
    REQUIRE(aligned(d,at,n));
    return pread(fileno(d->file),p,n,(off_t)at)==(ssize_t)n ? 0 : -1;
}
static int write_at(void *ctx,uint64_t at,const void *p,uint32_t n)
{
    Image *d=ctx;
    REQUIRE(aligned(d,at,n));
    ++d->writes;
    return pwrite(fileno(d->file),p,n,(off_t)at)==(ssize_t)n ? 0 : -1;
}
static int flush_at(void *ctx) { return fsync(fileno(((Image *)ctx)->file)); }
static void *allocate_at(void *ctx,size_t size)
{
    Image *d=ctx;
    if (d->fail_allocation) return NULL;
    void *p=malloc(size);
    if (p) ++d->allocations;
    return p;
}
static void release_at(void *ctx,void *p)
{
    Image *d=ctx;
    REQUIRE(p && d->allocations);
    --d->allocations;
    free(p);
}
static Provider open_provider(const char *module)
{
    Provider p={0};
    p.library=dlopen(module,RTLD_NOW|RTLD_LOCAL);
    if (!p.library) fprintf(stderr,"%s\n",dlerror());
    REQUIRE(p.library);
    const IfsFsModuleApiV2 *(*api)(void);
    void *symbol=dlsym(p.library,"infiltrator_filesystem_api_v2");
    REQUIRE(symbol && sizeof(api)==sizeof(symbol));
    memcpy(&api,&symbol,sizeof(api));
    const IfsFsModuleApiV2 *descriptor=api();
    p.core=(const IfsFsCoreOpsV2 *)ifs_fs_group_v2(descriptor,IFS_FS_GROUP_CORE);
    p.volume=(const IfsFsVolumeOpsV2 *)ifs_fs_group_v2(descriptor,IFS_FS_GROUP_VOLUME);
    REQUIRE(p.core && p.core->mount && p.core->unmount && p.core->read);
    return p;
}
static void verify_file(Provider *p,void *handle,unsigned index,const uint8_t payload[2][513])
{
    uint8_t got[513];
    REQUIRE(p->core->read(handle,"/SAME.BIN",0,got,sizeof(got))==IFS_FS_OK);
    REQUIRE(!memcmp(got,payload[index],sizeof(got)));
}
int main(int argc,char **argv)
{
    REQUIRE(argc==4 || argc==5);
    const uint32_t variant=(uint32_t)strtoul(argv[2],NULL,10);
    const uint64_t size=strtoull(argv[3],NULL,10);
    REQUIRE(variant && size>=1048576 && size%4096u==0u);
    Provider writer=open_provider(argv[1]);
    REQUIRE(writer.volume && writer.volume->format && writer.core->create && writer.core->write);
    Image media[2]={0};
    IfsFsHostRuntimeV2 hosts[2]={0};
    IfsFsVolumeInfoV2 info={.structure_bytes=sizeof(info)};
    void *handles[2]={0};
    uint8_t payload[2][513];
    for (unsigned i=0;i<2;++i) {
        for (size_t k=0;k<sizeof(payload[i]);++k) payload[i][k]=(uint8_t)(k*37u+i*115u);
        media[i].file=tmpfile();
        media[i].size=size;
        media[i].sector=512;
        REQUIRE(media[i].file && ftruncate(fileno(media[i].file),(off_t)size)==0);
        hosts[i]=(IfsFsHostRuntimeV2){
            .abi_version=IFS_FS_HOST_ABI_V2,.structure_bytes=sizeof(hosts[i]),
            .capabilities=IFS_FS_HOST_EXCLUSIVE|IFS_FS_HOST_DURABLE,
            .logical_sector_bytes=512,.physical_sector_bytes=4096,.maximum_transfer_bytes=65536,
            .volume_bytes=size,.context=&media[i],.read_at=read_at,.write_at=write_at,.flush=flush_at,
            .allocate=allocate_at,.release=release_at};
        REQUIRE(writer.volume->format(&hosts[i],variant,"",0)==IFS_FS_OK);
        /* Format disk two while the first remains mounted. */
        media[i].sector=hosts[i].logical_sector_bytes=i?4096u:512u;
        unsigned initial=media[i].writes;
        media[i].fail_allocation=1;
        REQUIRE(writer.core->mount(&hosts[i],&handles[i],&info)==IFS_FS_NO_MEMORY);
        REQUIRE(!handles[i] && media[i].writes==initial && !media[i].allocations);
        media[i].fail_allocation=0;
        REQUIRE(writer.core->mount(&hosts[i],&handles[i],&info)==IFS_FS_OK);
        REQUIRE(handles[i] && info.variant==variant && info.free_bytes<=info.total_bytes);
        REQUIRE(writer.core->create(handles[i],"/SAME.BIN",IFS_FS_ENTRY_FILE)==IFS_FS_OK);
        REQUIRE(writer.core->write(handles[i],"/SAME.BIN",0,payload[i],sizeof(payload[i]))==IFS_FS_OK);
    }
    REQUIRE(handles[0]!=handles[1]);
    for (unsigned k=0;k<8;++k) verify_file(&writer,handles[k%2],k%2,payload);
    for (unsigned i=0;i<2;++i) {
        void *duplicate=(void *)(uintptr_t)1;
        unsigned before=media[i].writes;
        REQUIRE(writer.core->mount(&hosts[i],&duplicate,&info)==IFS_FS_BUSY && !duplicate);
        REQUIRE(writer.volume->format(&hosts[i],variant,"",0)==IFS_FS_BUSY);
        REQUIRE(media[i].writes==before);
    }
    void *stale=handles[0];
    REQUIRE(writer.core->unmount(handles[0])==IFS_FS_OK);
    REQUIRE(writer.core->unmount(stale)==IFS_FS_INVALID && !media[0].allocations);
    verify_file(&writer,handles[1],1,payload);
    REQUIRE(writer.core->unmount(handles[1])==IFS_FS_OK && !media[1].allocations);
    /* Test stale handle rejection within the same module instance. */
    REQUIRE(writer.core->mount(&hosts[0],&handles[0],&info)==IFS_FS_OK && stale!=handles[0]);
    REQUIRE(writer.core->unmount(stale)==IFS_FS_INVALID);
    REQUIRE(writer.core->unmount(handles[0])==IFS_FS_OK && !media[0].allocations);
    REQUIRE(dlclose(writer.library)==0);
    Provider reader=open_provider(argc==5?argv[4]:argv[1]);
    unsigned writes[2]={media[0].writes,media[1].writes};
    for (unsigned i=0;i<2;++i) {
        hosts[i].capabilities=IFS_FS_HOST_READ_ONLY;
        hosts[i].write_at=NULL;
        hosts[i].flush=NULL;
        REQUIRE(reader.core->mount(&hosts[i],&handles[i],&info)==IFS_FS_OK && handles[i]);
    }
    for (unsigned k=0;k<8;++k) verify_file(&reader,handles[k%2],k%2,payload);
    REQUIRE(reader.core->unmount(handles[1])==IFS_FS_OK);
    verify_file(&reader,handles[0],0,payload);
    REQUIRE(reader.core->unmount(handles[0])==IFS_FS_OK);
    for (unsigned i=0;i<2;++i) {
        REQUIRE(writes[i]==media[i].writes && !media[i].allocations);
        REQUIRE(fclose(media[i].file)==0);
    }
    REQUIRE(dlclose(reader.library)==0);
    puts("PASS: two independent writable mounts, cold read-only remount, lease exclusion and balanced allocation");
    return 0;
}
