# Filesystem ABI V2 — exclusive system contract

**Only Common V2 exists.** No historical API, compatibility export,
fallback, alias, function-table shim or dependency on any prior ABI may
be shipped by Libraries, its filesystem providers, or its consumers.
A future V3 will replace V2 in a clean break, not inherit an implicit
fallback. This is an engineering rule, not a recommendation.

Canonical source: `include/infiltratr/filesystem_module_v2.h`.
Sole entry: `infiltrator_filesystem_api_v2()`.
All public descriptors and structures are version 2; group revision is
`IFS_FS_GROUP_ABI_V2`. A module **must** export a valid CORE group with
a real PROBE callback. All other callbacks and capabilities are optional
and must represent code that actually works on qualified media.

## System boundaries

A provider is the single authoritative implementation of its on-disk
format. Its `.so` owns probing, mounting, directory and file operations,
creation, formatting, verification, repair, relocation, metadata, streams
and snapshots where supported. Optional implementations are expressed by
missing capability bits and NULL callbacks—not simulated by consumers.
The same source may be built for more than one verified execution profile,
but no profile is implied just because a common header compiled.

The host supplies partition-relative bounded I/O, exact physical/logical
sector geometry, allocation, durable barriers and optional service hooks
in `IfsFsHostRuntimeV2`. The host obtains real exclusive unmounted
device ownership before writing: the descriptor flags alone never prove
exclusivity. Every callback must guard bounds and mount lifetimes. The
format must never be guessed, and no external formatter or userspace
proxy may stand in for provider-owned filesystem logic.

**CORE group:** probe; mount/unmount; stat; directory entry; read/write;
create/remove/rename/truncate. Mount operations are absent for volumes
without a file namespace (e.g. Swap).

**VOLUME group:** query, label, sync, resize, check, repair, scrub, trim
and format. Format requires independently qualified exclusive access.

**HANDLES, METADATA, DEFRAG, STREAMS, SNAPSHOTS:** optional groups,
each versioned separately. Defragmenter plans and moves via the provider's
relocation engine; consumers do not interpret filesystem block allocation.

## Consumers

Files, Filesystems, Defragmenter, Boot and Kernel discover only the
Common V2 export. The loader inspects only an authenticated, already
open binary and rejects an invalid, absent, incomplete or mismatched V2
descriptor. There is no alternate search path and no silent downgrade.

The Kernel still needs actual in-kernel ELF loading, memory/relocation
and symbol policy, VFS glue and pre-root bootstrap qualification before
any installed `.so` can be treated as a bootable root driver. Boot must
independently prove its firmware loader and read-only device integration.
Neither may claim a native execution profile from a desktop-only test.

## Qualification and release

A provider cannot be called complete merely because the descriptor
compiles. It must pass on-media positive and hostile fixtures for each
advertised operation, fault and durability tests for writes, exact
architectural and execution-profile checks and consumer integration.
A release must have no historical ABI exports or fallback code.

**This specification describes the strict intended architecture.**
The independent repository migrations must still be qualified before
deployed software can safely replace an older release.

## Sector-aligned host reads

`ifs_fs_host_read_bytes_v2` reads arbitrary metadata byte ranges using
only complete, aligned host logical-sector transfers within the authenticated
partition. The callback never receives a request larger than its advertised
maximum transfer. Any sub-sector read uses host-owned scratch memory;
failed reads return an error and must not be treated as a valid signature.
Providers should use this helper for metadata at offsets such as 1024 on
4K-logical-sector media instead of directly calling `read_at(1024, 512)`.
The helper adds no platform or previous-ABI dependency.
