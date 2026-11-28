/* SPDX-License-Identifier: GPL-3.0-or-later
 * Lifetime management for provider-owned mount state. A module embeds one
 * link in each private volume and owns one registry per module instance.
 * The public handle is an opaque generation, never an allocation address.
 */
#ifndef INFILTRATR_FILESYSTEM_SESSION_V2_H
#define INFILTRATR_FILESYSTEM_SESSION_V2_H
#include <infiltratr/filesystem_module_v2.h>

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
typedef volatile long IfsFsSessionGuardV2;
#define IFS_FS_SESSION_GUARD_INIT_V2 0
static inline void ifs_fs_session_lock_v2(IfsFsSessionGuardV2 *guard)
{
    while (_InterlockedExchange(guard, 1) != 0) {}
}
static inline void ifs_fs_session_unlock_v2(IfsFsSessionGuardV2 *guard)
{
    (void)_InterlockedExchange(guard, 0);
}
#else
#include <stdatomic.h>
typedef atomic_flag IfsFsSessionGuardV2;
#define IFS_FS_SESSION_GUARD_INIT_V2 ATOMIC_FLAG_INIT
static inline void ifs_fs_session_lock_v2(IfsFsSessionGuardV2 *guard)
{
    while (atomic_flag_test_and_set_explicit(guard, memory_order_acquire)) {}
}
static inline void ifs_fs_session_unlock_v2(IfsFsSessionGuardV2 *guard)
{
    atomic_flag_clear_explicit(guard, memory_order_release);
}
#endif

typedef struct IfsFsSessionLinkV2 {
    struct IfsFsSessionLinkV2 *next;
    uintptr_t token;
    void *volume;
    unsigned active;
} IfsFsSessionLinkV2;

typedef struct {
    IfsFsSessionGuardV2 guard;
    uintptr_t generation;
    IfsFsSessionLinkV2 *head;
} IfsFsSessionRegistryV2;

#define IFS_FS_SESSION_REGISTRY_INIT_V2 { .guard = IFS_FS_SESSION_GUARD_INIT_V2 }

/* A successful mount can outlive the descriptor passed by the caller. Its
 * context and services remain host-owned. Copy exactly the supplied prefix
 * so a caller built before the optional entropy field needs no tail storage.
 * The provider calls this after validating the incoming host. */
static inline void ifs_fs_session_copy_host_v2(IfsFsHostRuntimeV2 *owned,
                                                const IfsFsHostRuntimeV2 *host)
{
    size_t length = host->structure_bytes;
    if (length > sizeof(*owned)) length = sizeof(*owned);
    *owned = (IfsFsHostRuntimeV2){0};
    for (size_t i = 0; i < length; ++i)
        ((unsigned char *)owned)[i] = ((const unsigned char *)host)[i];
    owned->structure_bytes = (uint32_t)length;
}

/* Publication occurs only after discovery succeeds. The provider owns the
 * link until removal. No registry operation allocates, calls the host or
 * holds the guard during file I/O. Exhausted generations are never reused. */
static inline int ifs_fs_session_publish_v2(IfsFsSessionRegistryV2 *registry,
        IfsFsSessionLinkV2 *link, void *volume, void **handle)
{
    uintptr_t token;
    *handle = NULL;
    ifs_fs_session_lock_v2(&registry->guard);
    do {
        if (registry->generation == UINTPTR_MAX) {
            ifs_fs_session_unlock_v2(&registry->guard);
            return IFS_FS_BUSY;
        }
        token = ++registry->generation ^ (uintptr_t)(void *)registry;
    } while (token == 0);
    *link = (IfsFsSessionLinkV2){registry->head, token, volume, 0};
    registry->head = link;
    *handle = (void *)token;
    ifs_fs_session_unlock_v2(&registry->guard);
    return IFS_FS_OK;
}

/* A checkout serializes accesses to this mount, including its mutable
 * caches. Detach competes for the same reservation. If a callback is in
 * flight the caller may retry unmount; the live object remains intact.
 * Different mounts only share the short list lookup. Incoming handles
 * are compared as values and are never dereferenced. */
static inline int ifs_fs_session_lookup_v2(IfsFsSessionRegistryV2 *registry,
        void *handle, int detach, void **volume)
{
    int rc = IFS_FS_INVALID;
    *volume = NULL;
    if (!handle) return rc;
    ifs_fs_session_lock_v2(&registry->guard);
    for (IfsFsSessionLinkV2 **at = &registry->head; *at; at = &(*at)->next) {
        IfsFsSessionLinkV2 *link = *at;
        if (link->token != (uintptr_t)handle) continue;
        if (link->active) {
            rc = IFS_FS_BUSY;
        } else {
            if (detach) *at = link->next;
            else link->active = 1;
            *volume = link->volume;
            rc = IFS_FS_OK;
        }
        break;
    }
    ifs_fs_session_unlock_v2(&registry->guard);
    return rc;
}

static inline void ifs_fs_session_return_v2(IfsFsSessionRegistryV2 *registry,
                                             IfsFsSessionLinkV2 *link)
{
    ifs_fs_session_lock_v2(&registry->guard);
    link->active = 0;
    ifs_fs_session_unlock_v2(&registry->guard);
}
#endif
