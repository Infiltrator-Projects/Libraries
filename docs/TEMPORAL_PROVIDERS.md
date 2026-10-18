# Filesystem-discovered temporal resources

## Status

Common owns the source, build, tests, packaging and releases of the independently
installed calendar and explicit clock resources in `resources/temporal`. It also
owns their ABI, validation, discovery, loading and policy bridge. Normal host
Gregorian date/locale services remain available without optional modules.
The compiled clock table describes persisted modes and duration projections; it
is never evidence that an executable clock is installed.

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

## Installed catalogues and execution

The installed calendar and clock catalogues admit only modules whose metadata and
executable provider ABIs both validate. Entries have stable IDs and deterministic
filesystem order. The OS-following `standard` clock remains a policy selector.
A metadata-only file is never an executable capability. One invalid optional
module is ignored without removing unrelated valid modules.

The catalogue is snapshotted on first use in each process. Installing or removing
resources affects newly started consumers without invalidating live pointers.
Every one of the twenty-three explicit clocks uses its native implementation for
both formatting and next-visible-tick scheduling. All thirty supplied resource calendars
use native providers for fields, formatting and navigation. Missing, mismatched
or malformed resources cannot invoke a compiled fallback.

The legacy absolute clock formatting symbols remain ABI stubs that return false
without changing caller output. Elapsed-duration projection is a separate API;
it does not render an absolute clock face or replace a missing resource.

## Trust boundary

These directories are system resource namespaces. Native module code runs with the host process's authority, so this contract is for trusted packaged/system-installed resources. It is not a user-plugin security model. Any future untrusted or per-user native resource mechanism requires an explicit isolation and trust design rather than silently reusing in-process loading.

## Design rules

- The filesystem directory is the installed-capability catalogue for additional temporal resources.
- No consumer owns a duplicate implementation or a hidden fallback.
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
