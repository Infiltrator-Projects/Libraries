/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <infiltratr/filesystem_session_v2.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(test) do { if (!(test)) { fprintf(stderr, "%d: %s\n", __LINE__, #test); return 1; } } while (0)

int main(void)
{
    IfsFsSessionRegistryV2 registry = IFS_FS_SESSION_REGISTRY_INIT_V2;
    IfsFsSessionLinkV2 first, second;
    void *a, *b, *old, *volume;
    CHECK(!ifs_fs_session_publish_v2(&registry, &first, &first, &a));
    CHECK(!ifs_fs_session_publish_v2(&registry, &second, &second, &b));
    CHECK(a && b && a != b);
    CHECK(!ifs_fs_session_lookup_v2(&registry, a, 0, &volume) && volume == &first);
    CHECK(ifs_fs_session_lookup_v2(&registry, a, 0, &volume) == IFS_FS_BUSY && !volume);
    CHECK(ifs_fs_session_lookup_v2(&registry, a, 1, &volume) == IFS_FS_BUSY && !volume);
    CHECK(!ifs_fs_session_lookup_v2(&registry, b, 1, &volume) && volume == &second);
    ifs_fs_session_return_v2(&registry, &first);
    CHECK(!ifs_fs_session_lookup_v2(&registry, a, 1, &volume) && volume == &first);
    old = a;
    CHECK(!ifs_fs_session_publish_v2(&registry, &first, &first, &a) && a != old);
    CHECK(ifs_fs_session_lookup_v2(&registry, old, 0, &volume) == IFS_FS_INVALID && !volume);
    CHECK(!ifs_fs_session_lookup_v2(&registry, a, 1, &volume));
    registry.generation = UINTPTR_MAX;
    CHECK(ifs_fs_session_publish_v2(&registry, &second, &second, &b) == IFS_FS_BUSY && !b);

    /* Use a genuinely short allocation, so ASan also checks the source
     * boundary when a pre-entropy host is retained across calls. */
    IfsFsHostRuntimeV2 host = {0}, owned;
    host.abi_version = IFS_FS_HOST_ABI_V2;
    host.structure_bytes = IFS_FS_HOST_V2_BASE_BYTES;
    host.context = &host;
    void *prefix = malloc(host.structure_bytes);
    CHECK(prefix);
    memcpy(prefix, &host, host.structure_bytes);
    ifs_fs_session_copy_host_v2(&owned, prefix);
    free(prefix);
    CHECK(owned.structure_bytes == host.structure_bytes && owned.context == &host && !owned.random_bytes);
    host.structure_bytes = UINT32_MAX;
    ifs_fs_session_copy_host_v2(&owned, &host);
    CHECK(owned.structure_bytes == sizeof(owned));
    return 0;
}
