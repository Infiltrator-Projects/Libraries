# Filesystem-discovered temporal resources

## Status

Common now supports two temporal capability sources in parallel:

1. the existing compiled calendar and clock catalogues; and
2. validated native resource modules installed in the InfiltratorOS resource namespace.

The existing compiled catalogues remain authoritative compatibility capability. Nothing in this design removes them.

## Native resource directories

The native InfiltratorOS locations are case-sensitive and explicit:

```text
/System/Resources/Calendars/
/System/Resources/Clocks/
```

Common does not translate these names through the earlier provisional lowercase `calendars/` and `clocks/` root convention. `infiltratr_temporal_runtime_scan_native()` addresses the native directories directly.

## Discovery and validation

`infiltratr/temporal_resource.h` remains the low-level directory scanner. It enumerates direct non-hidden file/directory candidates deterministically, ignores symbolic links/reparse points and unrelated filesystem objects, and treats a missing directory as an empty catalogue.

`infiltratr/temporal_runtime.h` adds the executable-resource boundary. ABI version 1 recognises direct native-module files only. A module is an installed temporal resource only when it exports `infiltratr_temporal_resource_metadata_v1` and returns valid versioned metadata containing a stable identifier, user-facing name and resource kind. Clock metadata also carries the generic seconds/location capability flags.

A directory entry, arbitrary file, malformed module or module of the wrong kind is ignored independently. Presence in the filesystem alone never makes an invalid capability selectable.

Directory bundles remain reserved for a later resource ABI. Freezing a bundle schema before a real resource needs one would add format policy without evidence.

## Merged catalogues

Common exposes process-stable merged metadata views for installed calendars and clocks. Existing compiled entries come first. Valid native resources are appended in deterministic filesystem order. A native resource may not replace a built-in **metadata** entry: the built-in label wins duplicate IDs.

The merged catalogue is snapshotted on first use in a process. Installing or removing resources therefore takes effect for newly started consumers rather than invalidating pointers or executable code underneath a running application.

Metadata discovery and behaviour ownership remain separate. Native Calendar Resources own chronology and must validate their provider ABI before executing a discovered calendar module. Clock metadata alone does not prove executable clock support; the clock runtime must validate and bind the implementation ABI.

The shared clock presentation handle is **resource-only** for all sixteen extracted clocks, even when the stable ID is listed in Common's compiled capability metadata: decimal, Internet, Unix, hexadecimal, binary, Julian, modified Julian, Chinese hundred-kè, mean solar, local sidereal, Chinese double-hours, apparent solar, Italian sunset hours, Renaissance Babylonian sunrise hours, standard 12-hour and standard 24-hour. An absent, invalid or mismatched native module must fail closed. The compiled capability catalogue is **metadata**, not an alternative implementation. The remaining seven alternative modes retain an interim portable formatter until extracted. The old legacy profile formatting symbol remains for ABI linkage but deliberately no longer performs clock-face calculations; clients must use the resource presentation ABI. The duration API still formats elapsed time intervals, which have different semantics from clock faces.

## Trust boundary

These directories are system resource namespaces. Native module code runs with the host process's authority, so this contract is for trusted packaged/system-installed resources. It is not a user-plugin security model. Any future untrusted or per-user native resource mechanism requires an explicit isolation and trust design rather than silently reusing in-process loading.

## Design rules

- The filesystem directory is the installed-capability catalogue for additional temporal resources.
- Existing built-in calendars and clocks continue to work during migration.
- Calendar and clock remain independently selectable capabilities.
- A stable ID is semantic identity; array position and filename are not identity.
- One bad optional resource must not invalidate unrelated temporal systems.
- A missing resource must never silently become a different calendar or clock.
- Canonical timestamps and durations are never rewritten merely because presentation resources change.
- Product-specific chronology/timekeeping semantics stay with the product/provider that owns them; Common owns product-neutral discovery and metadata validation.

## First proving case

The first intended historical pair remains:

- `egyptian-early-lunar` — Early Egyptian Lunar Calendar;
- `egyptian-stellar-solar` — Early Egyptian Stellar–Solar Time.

The runtime contract now provides the installation/discovery boundary for those resources. Their actual historical algorithms, evidence, tests and any richer bundle needs remain separate work and must not be fabricated merely to exercise the loader.
