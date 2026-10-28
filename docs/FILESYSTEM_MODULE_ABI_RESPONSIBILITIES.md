# Common / Libraries: shared interface for single-file filesystem modules

**Target direction:** [Filesystems' authoritative single-module architecture](https://github.com/Infiltrator-Projects/Filesystems/blob/main/docs/UNIFIED_FILESYSTEM_MODULE_ARCHITECTURE.md). One independently maintained filesystem implementation is distributed as one trusted `<format>.so` under `/System/Filesystems/`, to be consumed by BOOT, the InfiltratorOS kernel and applications. Common owns **shared, format-neutral contracts**, not filesystem drivers or product-specific policy.

## Common-owned contracts (when implemented and adopted)

- Stable format ID, versioned/size-tagged module ABI, capability enumeration, immutable metadata and ABI/profile compatibility checks; one module may advertise different validated operations for each consumer.
- Generic host service tables: bounds-checked block I/O, memory allocation, synchronisation, cancellation, logging/diagnostics, errors, lifecycle and lifetime/ownership rules; the same format logic receives host services supplied by each execution environment.
- Module metadata discovery without executing untrusted code, identity and architecture checks, version negotiation, digest/signature/trust primitives and an explicit contract for safe loading from each platform.
- Cross-platform definitions that a BOOT-owned UEFI loader, kernel-owned bridge and userspace loader can implement **without linking ordinary userspace Common/glibc inside kernel or firmware**. Freestanding subset(s) must be separated and demonstrably compilable for those environments.

## Exclusions and dependencies

Each independent Filesystem-* repository owns its format-specific parser, reader/writer, allocator, formatter, defragmentation and recovery implementation, plus any needed internal representations. **Do not copy or move that format code into Common** merely because BOOT or the kernel needs access to it. BOOT owns EFI mechanics and early-boot firmware policy; the kernel owns privileged VFS mapping and root bootstrap; applications own their own UI and safety policy. Common should not become a second privileged filesystem driver, package manager or filesystem catalogue.

The `.so` suffix is the user-distributed product contract, not proof that Linux's normal dynamic loader, kernel and UEFI all accept the same executable ABI today. The required restricted binary profile or internal representation is still to be designed and tested with real BOOT/kernel consumers. Until that is qualified, existing `.ko` and `.efi` flows remain transitional compatibility mechanisms, not the final per-format distribution model.

## Promotion and validation

Define new interface proposals and compatibility matrices with filesystem, BOOT and kernel owners; prove one actual FAT12/FAT16 module can be loaded by a real application, BOOT and an OS bridge; test wrong-format rejection, malformed metadata, permissions, signatures, ABI mismatches, layout bounds, I/O failure and security boundaries. Pin adopted Common revisions; do not change callers silently or declare universal execution complete based on header-only checks.
