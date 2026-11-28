/* SPDX-License-Identifier: GPL-3.0-or-later
 * Hosted contract test for a mounted, read-only Common V2 module. The
 * backing image and its native qualification fixture belong to the provider.
 * Stop a real provider read inside host I/O and exercise the other mount
 * while the first one's volume memory must remain live.
 */
#ifndef INFILTRATR_TEST_FILESYSTEM_READER_LIFETIME_H
#define INFILTRATR_TEST_FILESYSTEM_READER_LIFETIME_H
#include <infiltratr/filesystem_module_v2.h>
#include <assert.h>
#include <pthread.h>

static pthread_mutex_t lifetime_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t lifetime_changed = PTHREAD_COND_INITIALIZER;
static int lifetime_armed, lifetime_waiting, lifetime_finished;
static int (*lifetime_read)(void *, uint64_t, void *, uint32_t);

static int lifetime_block_read(void *context, uint64_t offset, void *out, uint32_t bytes)
{
    pthread_mutex_lock(&lifetime_mutex);
    if (lifetime_armed) {
        lifetime_waiting = 1;
        pthread_cond_broadcast(&lifetime_changed);
        while (lifetime_armed) pthread_cond_wait(&lifetime_changed, &lifetime_mutex);
    }
    pthread_mutex_unlock(&lifetime_mutex);
    return lifetime_read(context, offset, out, bytes);
}

struct lifetime_request {
    const IfsFsCoreOpsV2 *core;
    void *handle;
    int result;
};

static void *lifetime_directory_thread(void *context)
{
    struct lifetime_request *request = context;
    char name[256];
    IfsFsEntryV2 entry = {.structure_bytes=sizeof(entry), .name=name, .name_capacity=sizeof(name)};
    request->result = request->core->directory_entry(request->handle, "/", 0, &entry);
    pthread_mutex_lock(&lifetime_mutex);
    lifetime_finished = 1;
    pthread_cond_broadcast(&lifetime_changed);
    pthread_mutex_unlock(&lifetime_mutex);
    return NULL;
}

static void check_reader_lifetime(const IfsFsCoreOpsV2 *core, const IfsFsHostRuntimeV2 *host)
{
    IfsFsHostRuntimeV2 temporary = *host;
    IfsFsVolumeInfoV2 info = {.structure_bytes=sizeof(info)};
    char name[256];
    IfsFsEntryV2 entry = {.structure_bytes=sizeof(entry), .name=name, .name_capacity=sizeof(name)};
    struct lifetime_request request = {.core=core};
    void *other, *reopened;
    pthread_t thread;

    lifetime_read = host->read_at;
    temporary.read_at = lifetime_block_read;
    assert(core->mount(&temporary, &request.handle, &info) == IFS_FS_OK);
    /* Changing the caller's descriptor after mount must have no effect. */
    temporary.volume_bytes = 0;
    assert(core->mount(host, &other, &info) == IFS_FS_OK && other != request.handle);
    lifetime_armed = 1;
    lifetime_finished = lifetime_waiting = 0;
    assert(pthread_create(&thread, NULL, lifetime_directory_thread, &request) == 0);
    pthread_mutex_lock(&lifetime_mutex);
    while (!lifetime_waiting && !lifetime_finished)
        pthread_cond_wait(&lifetime_changed, &lifetime_mutex);
    assert(lifetime_waiting && !lifetime_finished);
    pthread_mutex_unlock(&lifetime_mutex);

    assert(core->unmount(request.handle) == IFS_FS_BUSY);
    assert(core->stat(request.handle, "/", &entry) == IFS_FS_BUSY);
    assert(core->stat(other, "/", &entry) == IFS_FS_OK);
    assert(core->unmount(other) == IFS_FS_OK);
    pthread_mutex_lock(&lifetime_mutex);
    lifetime_armed = 0;
    pthread_cond_broadcast(&lifetime_changed);
    pthread_mutex_unlock(&lifetime_mutex);
    assert(pthread_join(thread, NULL) == 0);
    assert(request.result == IFS_FS_OK || request.result == IFS_FS_END);
    assert(core->unmount(request.handle) == IFS_FS_OK);
    assert(core->mount(host, &reopened, &info) == IFS_FS_OK && reopened != request.handle);
    assert(core->stat(request.handle, "/", &entry) == IFS_FS_INVALID);
    assert(core->directory_entry(request.handle, "/", 0, &entry) == IFS_FS_INVALID);
    assert(core->read(request.handle, "/", 0, name, 1) == IFS_FS_INVALID);
    assert(core->unmount(request.handle) == IFS_FS_INVALID);
    assert(core->stat(reopened, "/", &entry) == IFS_FS_OK);
    assert(core->unmount(reopened) == IFS_FS_OK);
}
#endif
