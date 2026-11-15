/* SPDX-License-Identifier: GPL-3.0-or-later
 * Common V2 read-only volume-session registry for metadata-only providers.
 *
 * This is NOT a VFS namespace adapter: it owns only lifecycle and volume
 * information. Providers must advertise STAT/LIST/READ separately only when
 * those operations really exist. No global one-volume singleton is allowed.
 */
#ifndef INFILTRATR_FILESYSTEM_RO_SESSIONS_V2_H
#define INFILTRATR_FILESYSTEM_RO_SESSIONS_V2_H
#include <infiltratr/filesystem_module_v2.h>
#include <stdint.h>
#if defined(_MSC_VER) && !defined(__clang__)
/* MSVC's /std:c11 mode does not enable <stdatomic.h> for this target.
 * InterlockedExchange is a full-barrier atomic exchange on supported CPUs. */
#include <intrin.h>
typedef volatile long IfsFsRoGuardV2;
#define IFS_FS_RO_GUARD_INIT_V2 0
#else
#include <stdatomic.h>
typedef atomic_flag IfsFsRoGuardV2;
#define IFS_FS_RO_GUARD_INIT_V2 ATOMIC_FLAG_INIT
#endif

#define IFS_FS_RO_MAX_SESSIONS_V2 32u

typedef struct {
    uintptr_t cookie;
    IfsFsVolumeInfoV2 info;
} IfsFsReadOnlySessionSlotV2;

typedef struct {
    IfsFsRoGuardV2 guard;
    uintptr_t next;
    IfsFsReadOnlySessionSlotV2 slots[IFS_FS_RO_MAX_SESSIONS_V2];
} IfsFsReadOnlySessionsV2;

#define IFS_FS_RO_SESSIONS_INIT_V2 { .guard = IFS_FS_RO_GUARD_INIT_V2 }

static inline void ifs_fs_ro_sessions_lock_v2(IfsFsReadOnlySessionsV2 *r)
{
#if defined(_MSC_VER) && !defined(__clang__)
    while (_InterlockedExchange(&r->guard, 1) != 0) {}
#else
    while (atomic_flag_test_and_set_explicit(&r->guard, memory_order_acquire)) {}
#endif
}

static inline void ifs_fs_ro_sessions_unlock_v2(IfsFsReadOnlySessionsV2 *r)
{
#if defined(_MSC_VER) && !defined(__clang__)
    (void)_InterlockedExchange(&r->guard, 0);
#else
    atomic_flag_clear_explicit(&r->guard, memory_order_release);
#endif
}

/* The opaque cookie is never dereferenced, and is never reused. Salt it
 * with this registry's address so different independent modules cannot
 * normally accept each other's handles. Exhaustion fails closed. */
static inline int ifs_fs_ro_session_open_v2(
    IfsFsReadOnlySessionsV2 *r, const IfsFsVolumeInfoV2 *measured,
    void **volume, IfsFsVolumeInfoV2 *info)
{
    uint32_t i;
    uintptr_t cookie;
    if (!volume || !info || info->structure_bytes < sizeof(*info) ||
        !r || !measured || measured->structure_bytes < sizeof(*measured))
        return IFS_FS_INVALID;
    *volume = NULL;
    ifs_fs_ro_sessions_lock_v2(r);
    if (r->next == UINTPTR_MAX) {
        ifs_fs_ro_sessions_unlock_v2(r);
        return IFS_FS_BUSY;
    }
    for (i = 0; i < IFS_FS_RO_MAX_SESSIONS_V2; ++i)
        if (!r->slots[i].cookie) break;
    if (i == IFS_FS_RO_MAX_SESSIONS_V2) {
        ifs_fs_ro_sessions_unlock_v2(r);
        return IFS_FS_BUSY;
    }
    /* Both the strictly increasing sequence and fixed salt are preserved.
     * A zero cookie is skipped without ever giving it to a caller. */
    do {
        if (r->next == UINTPTR_MAX) {
            ifs_fs_ro_sessions_unlock_v2(r);
            return IFS_FS_BUSY;
        }
        cookie = (++r->next) ^ (uintptr_t)(void *)r;
    } while (!cookie);
    r->slots[i].info = *measured;
    r->slots[i].cookie = cookie;
    *info = *measured;
    *volume = (void *)cookie;
    ifs_fs_ro_sessions_unlock_v2(r);
    return IFS_FS_OK;
}

static inline int ifs_fs_ro_session_close_v2(
    IfsFsReadOnlySessionsV2 *r, void *volume)
{
    uint32_t i;
    uintptr_t cookie = (uintptr_t)volume;
    if (!r || !cookie) return IFS_FS_INVALID;
    ifs_fs_ro_sessions_lock_v2(r);
    for (i = 0; i < IFS_FS_RO_MAX_SESSIONS_V2; ++i)
        if (r->slots[i].cookie == cookie) {
            r->slots[i].cookie = 0;
            r->slots[i].info = (IfsFsVolumeInfoV2){0};
            ifs_fs_ro_sessions_unlock_v2(r);
            return IFS_FS_OK;
        }
    ifs_fs_ro_sessions_unlock_v2(r);
    return IFS_FS_INVALID;
}

static inline int ifs_fs_ro_session_query_v2(
    IfsFsReadOnlySessionsV2 *r, void *volume, IfsFsVolumeInfoV2 *info)
{
    uint32_t i;
    uintptr_t cookie = (uintptr_t)volume;
    if (!r || !cookie || !info || info->structure_bytes < sizeof(*info))
        return IFS_FS_INVALID;
    ifs_fs_ro_sessions_lock_v2(r);
    for (i = 0; i < IFS_FS_RO_MAX_SESSIONS_V2; ++i)
        if (r->slots[i].cookie == cookie) {
            *info = r->slots[i].info;
            ifs_fs_ro_sessions_unlock_v2(r);
            return IFS_FS_OK;
        }
    ifs_fs_ro_sessions_unlock_v2(r);
    return IFS_FS_INVALID;
}
#endif
