/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <infiltratr/filesystem_ro_sessions_v2.h>
#include <stdint.h>
#include <stdio.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "V2 sessions: %s\n", #x); return 1; } } while (0)
int main(void)
{
    static IfsFsReadOnlySessionsV2 registry = IFS_FS_RO_SESSIONS_INIT_V2;
    static IfsFsReadOnlySessionsV2 other = IFS_FS_RO_SESSIONS_INIT_V2;
    IfsFsVolumeInfoV2 template = {
        .structure_bytes = sizeof(IfsFsVolumeInfoV2),
        .variant = 4u, .block_bytes = 4096u,
        .total_bytes = 1024u * 1024u, .free_bytes = 4096u
    };
    IfsFsVolumeInfoV2 info = { .structure_bytes = sizeof(info) };
    void *handles[IFS_FS_RO_MAX_SESSIONS_V2 + 1u] = {0};
    void *stale;
    for (unsigned i = 0u; i < IFS_FS_RO_MAX_SESSIONS_V2; ++i) {
        template.free_bytes = i * 4096u;
        CHECK(ifs_fs_ro_session_open_v2(&registry, &template, &handles[i],
                                         &info) == IFS_FS_OK);
        CHECK(handles[i] != NULL && info.free_bytes == i * 4096u);
    }
    CHECK(ifs_fs_ro_session_open_v2(&registry, &template,
                                     &handles[IFS_FS_RO_MAX_SESSIONS_V2],
                                     &info) == IFS_FS_BUSY);
    CHECK(handles[IFS_FS_RO_MAX_SESSIONS_V2] == NULL);
    info.structure_bytes = sizeof(info);
    CHECK(ifs_fs_ro_session_query_v2(&registry, handles[5], &info) == IFS_FS_OK);
    CHECK(info.free_bytes == 5u * 4096u);
    CHECK(ifs_fs_ro_session_query_v2(&other, handles[5], &info) == IFS_FS_INVALID);
    stale = handles[5];
    CHECK(ifs_fs_ro_session_close_v2(&registry, stale) == IFS_FS_OK);
    CHECK(ifs_fs_ro_session_close_v2(&registry, stale) == IFS_FS_INVALID);
    CHECK(ifs_fs_ro_session_query_v2(&registry, stale, &info) == IFS_FS_INVALID);
    CHECK(ifs_fs_ro_session_open_v2(&registry, &template, &handles[5], &info) == IFS_FS_OK);
    CHECK(handles[5] != stale);
    CHECK(ifs_fs_ro_session_query_v2(&registry, stale, &info) == IFS_FS_INVALID);
    CHECK(ifs_fs_ro_session_close_v2(&registry, handles[5]) == IFS_FS_OK);
    for (unsigned i = 0u; i < IFS_FS_RO_MAX_SESSIONS_V2; ++i)
        if (i != 5u)
            CHECK(ifs_fs_ro_session_close_v2(&registry, handles[i]) == IFS_FS_OK);
    CHECK(ifs_fs_ro_session_query_v2(&registry, handles[0], &info) == IFS_FS_INVALID);
    return 0;
}
