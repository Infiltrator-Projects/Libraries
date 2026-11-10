# Infiltratr Common filesystem ABI V2 — universal extension contract

V1 (`infiltrator_filesystem_api_v1`) is **binary immutable**. This release introduces `include/infiltratr/filesystem_module_v2.h` and the independently discoverable `infiltrator_filesystem_api_v2` descriptor. A V2 provider MUST still export a truthful, complete V1 table. V2 provides extension groups; it does **not** rebrand format-specific APIs, duplicate disk engines or claim all operations are implemented.

## Shared consumer interface

| Contract | Owners / intended consumers |
| --- | --- |
| V1 probe, mount, stat, list, read, write, create, remove, rename, truncate, format, check, recover | Files, Disks, Setup, BOOT, kernel and test tools |
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

Every group has an **immutable group ID**, a versioned and size-tagged header, its own capabilities and typed callbacks. `group_mask` advertises **only implemented** groups. A host authenticates an ELF file and validates its architecture and execution profile **before loading/calling the entry point**; `ifs_fs_api_v2_valid` then checks V1 identity, every claimed group and callback consistency. A bogus V2 descriptor must fail closed, never silently downgrade to V1. V1-only and legacy modules can still be inspected by the original API.

The `IfsFsHostRuntimeV2` structure has fixed-size integer and function-pointer fields, no Linux/EFI platform types, and no userspace runtime dependency. All block offsets are partition-relative. Destructive I/O requires the **host** to have obtained a verified exclusive, unmounted lease, complete geometry and durable flush. An unchecked boolean flag is not authorisation. Cancellation is cooperative and must not interrupt an uncommitted transaction. The host never gets to rewrite on-disk algorithms or silently emulate unsupported capabilities.

**Profiles are declarations, not proof.** A V2 descriptor saying `FREESTANDING` does not mean an ordinary ELF can run in UEFI or kernel mode. BOOT requires its independently qualified ELF loader. The Kernel requires a trusted format-neutral in-kernel loader, symbol/relocation and memory-ownership policy, VFS adapter and pre-root bootstrap. Neither may substitute FUSE or a userspace filesystem proxy to claim the single-SO goal. Existing kernel modules and Boot/EFI integrations remain in service until real equivalent tests pass.

## Adoption gates (must not be conflated)

1. Common V2 header, freestanding compilation, strict negotiation/host validation and hostile fixture tests.
2. FAT12 V2 discovery and V1-backed real media tests, without fake extension groups.
3. Real implementations of the optional operations, with independent image tests and correct failure/rollback behaviour per filesystem.
4. Files/Disks/Setup integration, resolving right-click items to the actual owning volume and verifying exclusive raw-media access before destructive actions.
5. BOOT's actual signed/verified ELF and firmware interface evidence; Kernel's **direct in-kernel** ELF/VFS load and root-mount evidence.
6. Cross-platform, storage, crash/fault injection and release qualification before retiring any compatibility path.

Kernel, BOOT, Files and Defragmenter consume **the same Common descriptor and feature IDs**, but have separate environment adapters. No per-format filesystem parser belongs in those consumers.

**Current implementation scope:** V2 defines and validates operation tables and host services. It is not a completed kernel loader, Files formatter, universal defragmentation engine or published 23-format interoperability qualification.
