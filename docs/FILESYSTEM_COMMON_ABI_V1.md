# Filesystem common ABI — first implementation pass (2026-10-30)

Common owns `include/infiltratr/filesystem_module.h`, the format-neutral V1 contract for filesystem .so modules. Format-specific parsing and mutation MUST remain in independent filesystem repositories; Common must not import an OFS, FAT or exFAT parser.

The audited common legacy prefix is `{uint32_t abi_version, uint32_t structure_bytes, const char *format_id}`; only these three fields are layout-compatible across current FAT12/FAT16/FAT32/OFS/FFS/exFAT/EXT2/BFS/Minix drop-ins. In particular, the existing native capability integer is NOT portable.

A new provider may export `infiltrator_filesystem_api_v1()` returning a pointer to `IfsFsModuleApiV1`. Its capability bits and operation signatures have stable meanings irrespective of format. Optional operations have null pointers and absent capability bits. The host supplies partition-relative I/O, controls read/write privileges, and owns the validated module loader. Each mounted volume reports its detected on-disk variant and filename/block-size constraints; the disk format cannot be silently upgraded because a library supports newer variants.

`infiltratr_filesystem_module_inspect_loaded()` uses Common's existing `dynlib` symbol mechanism. A caller **must first authenticate the binary and open it according to a reviewed host policy**. The inspector never opens paths or assigns a native capability bit a common meaning. It detects the new Common API when present and otherwise selects the highest version of the native legacy entry point, returning its table opaquely, without promising its call signatures are compatible.

These are real compile-time/runtime discovery primitives, but they do not yet make the existing provider operations universal. Per-format canonical-engine adapters and independent media qualification remain required before advertising universal create/write/format/repair/defrag. BOOT/kernel loading of a Linux userspace .so is not implied.

See the [full module audit](https://github.com/Infiltrator-Projects/Filesystems/blob/main/docs/FILESYSTEM_SO_ABI_AUDIT_2026-10-30.md).

## First adopter: FAT12 (9 November 2026)

`Filesystem-FAT12` now exports `infiltrator_filesystem_api_v1` with the stable Common V1 prefix, capability validation and only two admitted operations: `probe` and `format`. The format adapter delegates to its already-existing V4 FatFs formatter; it is **not** a second FAT12 formatting engine. Only a validated existing FAT12 partition, an empty label and independently verified exclusive host access are admitted. The caller must not offer blank-disk formatting through this Common V1 adapter, because `IfsFsHostIoV1` currently has no logical-sector-size field. Common V1 mount/read/write, check/recover and native kernel execution have not been implemented in this FAT12 pilot. The FAT12 repository currently carries an exact, provenance-recorded snapshot of this freestanding header; plan promotion to the pinned Common dependency before broad adoption.

An ABI evolution for blank target formatting must carry explicit logical sector size and physical-device/topology constraints without breaking callers of V1, and must have independently verified destructive image tests. The kernel will provide a format-neutral loader/VFS adapter rather than duplicate FAT12 logic.
