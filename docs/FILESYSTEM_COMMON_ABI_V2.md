# Infiltratr Common filesystem ABI V2 — standalone provider contract

**Target: one Common V2 ABI, with no V1 dependency or per-format shims in consumers.** Each filesystem owns its one implementation in its own shared object. The previous V2 design incorrectly required a Common V1 function table and made V2 only an extension wrapper. V2 now supports direct core operations through the CORE group and permits `base_v1 == NULL`. Existing V1-backed providers remain readable only during controlled migration; they do **not** pass the strict native-V2 qualification predicate. V1 layouts are unchanged to avoid corrupting installations already using them.

## Shared consumer interface

| Contract | Owners / intended consumers |
| --- | --- |
| V2 CORE probe, mount, stat, list, read, write, create, remove, rename and truncate | Files, Disks, Setup, BOOT, Kernel and test tools |
| V2 VOLUME format, check, repair, scrub, sync, size, label and trim | Files, Disks, Setup, maintenance and Kernel |
| V2 persistent file handles, per-handle I/O, directory cursors and sync | Kernel VFS, Files |
| V2 ownership, permissions, timestamps, links, xattrs and ACL | Kernel VFS, Files |
| V2 volume metadata, label, flush, resize, scrub, repair, trim and format | Disks, Setup, Files, maintenance |
| V2 extent maps, provider-owned plans, moves, verification and recovery | Defragmenter, Kernel |
| V2 named streams | Files, Kernel |
| V2 snapshots | Files, Kernel, maintenance |
| V2 host runtime: bounded partition I/O, explicit geometry, durable flush, allocation, locks, clock, cancellation and diagnostics | Linux kernel, UEFI BOOT, desktop application hosts |

### Portable VFS handle arguments

The V2 `Handles.open` `flags` argument is a **Common bitmask**, not Linux `O_*`, EFI or Windows flags. Hosts translate their platform's intentions into `IFS_FS_OPEN_READ`, `WRITE`, `CREATE`, `EXCLUSIVE`, `TRUNCATE` and `NOFOLLOW` before calling providers. `READ` or `WRITE` must be present; `CREATE`/`TRUNCATE` require `WRITE`, and `EXCLUSIVE` requires `CREATE`. The header's `ifs_fs_open_flags_valid()` checks these structural constraints. A provider must still enforce filesystem permissions, actual mount read-only state, mount-generation ownership, and its own format limits. Unknown bits are `IFS_FS_INVALID`; valid but unsupported options are `IFS_FS_UNSUPPORTED`. `NOFOLLOW` must never silently follow a symlink.

`Handles.sync` flags are 0 for full durable data/metadata synchronization or `IFS_FS_SYNC_DATA_ONLY` to permit omission of unrelated metadata. A provider must reject unknown flags and cannot claim durability without the host's actual durable-flush semantics.

For `pread` and `pwrite`, the output byte count is mandatory on success and cannot exceed the requested count. A successful zero-byte `pread` means EOF, while `next_dir` uses `IFS_FS_END`. All handles/cursors are invalid after unmount or mount-generation changes. Providers must not reinterpret these flags as their own format-specific capabilities.

These definitions do **not** change any V1 or V2 structure layout. They complete a previously unspecified interoperation contract; they do not make a provider implement V2 handles merely by including the header.

Every group has an **immutable group ID**, a versioned and size-tagged header, its own capabilities and typed callbacks. `group_mask` advertises **only implemented** groups. Group revision 1 is NOT Common ABI V1. Group 7 (`IFS_FS_GROUP_CORE`) provides primary filesystem operations directly with `IfsFsHostRuntimeV2`, `IfsFsEntryV2` and `IfsFsVolumeInfoV2`, with no Common V1 descriptor. A V2-only module must advertise a working CORE probe, even if (like Swap) it has no mountable directory tree.

The host authenticates the binary and validates its architecture and execution profile **before loading/calling its entry point**. `ifs_fs_api_v2_valid` accepts either a qualified transitional V1-backed descriptor or a genuine V2-only CORE descriptor. `ifs_fs_api_v2_native_valid` accepts **only the latter**. The strict loader `infiltratr_filesystem_module_inspect_loaded_v2_native` never returns a V1/legacy module and never silently downgrades an invalid V2. All claimed operations must match real callback pointers. Keep the older loader available only until its consumers are converted; do not use it in new filesystem consumers.

The `IfsFsHostRuntimeV2` structure has fixed-size integer and function-pointer fields, no Linux/EFI platform types, and no userspace runtime dependency. All block offsets are partition-relative. Destructive I/O requires the **host** to have obtained a verified exclusive, unmounted lease, complete geometry and durable flush. An unchecked boolean flag is not authorisation. Cancellation is cooperative and must not interrupt an uncommitted transaction. The host never gets to rewrite on-disk algorithms or silently emulate unsupported capabilities.

**Profiles are declarations, not proof.** A V2 descriptor saying `FREESTANDING` does not mean an ordinary ELF can run in UEFI or kernel mode. BOOT requires its independently qualified ELF loader. The Kernel requires a trusted format-neutral in-kernel loader, symbol/relocation and memory-ownership policy, VFS adapter and pre-root bootstrap. Neither may substitute FUSE or a userspace filesystem proxy to claim the single-SO goal. Existing kernel modules and Boot/EFI integrations remain in service until real equivalent tests pass.

## Adoption gates (must not be conflated)

1. Common V2 header, freestanding compilation, strict negotiation/host validation and hostile fixture tests.
2. Standalone Common V2 CORE integration: V2-only export, `base_v1 == NULL`, real media fixtures and strict loader acceptance; no fake operation tables.
3. Real implementations of the optional operations, with independent image tests and correct failure/rollback behaviour per filesystem.
4. Files/Disks/Setup integration, resolving right-click items to the actual owning volume and verifying exclusive raw-media access before destructive actions.
5. BOOT's actual signed/verified ELF and firmware interface evidence; Kernel's **direct in-kernel** ELF/VFS load and root-mount evidence.
6. Cross-platform, storage, crash/fault injection and release qualification **per provider** before retiring its V1 export or the transitional loader. Do not remove a working provider from service just to satisfy a source-only migration checklist.

Kernel, BOOT, Files and Defragmenter consume **the same Common descriptor and feature IDs**, but have separate environment adapters. No per-format filesystem parser belongs in those consumers.

**Current implementation scope:** V2 defines and validates operation tables and host services. It is not a completed kernel loader, Files formatter, universal defragmentation engine or published 23-format interoperability qualification.

## Provider migration rule

Do not replace V1 with a blank V2 shim. Each provider migrates the actual
native filesystem engine, and must pass valid and hostile on-media fixtures
for every claimed V2 callback. FORMAT, CHECK and REPAIR are operations of
the VOLUME group; the optional HANDLE, METADATA, DEFRAG, STREAMS and
SNAPSHOTS groups express only implemented features. A Swap provider can
implement CORE probe plus VOLUME format without claiming mount or file I/O.
A provider with only a qualified probe can make no claim about mounting,
writing, formatting or operating from firmware/kernel context.

The migration finishes only when every independent provider and every
consumer uses native V2, the old V1 export can be removed from provider
packages, and transitional discovery can be retired. Merging a new
descriptor or updating a dependency pin is **not** evidence of completion.
