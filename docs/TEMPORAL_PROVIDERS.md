# Filesystem-discovered temporal resources

## Status

The independent `Calendar-*` and `Clock-*` repositories own provider source,
builds, tests, localized catalogues, packaging and releases. Common preserves
its original source as a reference and owns the public ABI, validation,
discovery, loading and policy bridge. Normal host
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
module is ignored without removing unrelated valid modules. If multiple files
claim the same stable ID, selection uses the first fully validated executable
provider in sorted filesystem order; an invalid first file cannot shadow it.
Concurrent snapshot initialization sleeps on a condition, while re-entrant
queries see only the normal host/base selectors.

The catalogue is snapshotted on first use in each process. Installing or removing
resources affects newly started consumers without invalidating live pointers.
Every one of the twenty-three explicit clocks uses its native implementation for
both formatting and next-visible-tick scheduling. All thirty-one supplied resource calendars
use native providers for fields, formatting and navigation. Missing, mismatched
or malformed resources cannot invoke a compiled fallback.

The legacy absolute clock formatting symbols remain ABI stubs that return false
without changing caller output. Elapsed-duration projection is a separate API;
it does not render an absolute clock face or replace a missing resource.

## Canonical solar geometry and offline clock identities

The private `src/temporal_solar_compact.h` owns the compact solar terms,
geographic clamping and solar hour-angle mathematics. Elapsed seasonal
durations use this header. Independent clock repositories own their copied
solar helpers and corrections, with no runtime dependency between installed
provider `.so` files.
Their distinct event timing, origin conventions, historical units and
next-tick rules remain provider-specific.

The portable clock-mode catalogue is still required to parse persisted
preferences and project elapsed durations without requiring system-installed
modules. It is **not** a second executable clock implementation. Resource
tests verify that every optional persisted mode ID resolves to its own
matching executable provider (23 modules); the OS-following `standard`
selector is deliberately not an optional module.

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

## Compiled resource filenames

Each provider’s `SOURCE-MANIFEST.json` and one-provider Makefile determine
its canonical compiler output pathname and embedded metadata name.
For example, the stable clock ID `decimal` is compiled directly as
`French Revolutionary Decimal Clock.so`. Discoveries validate executable
ABIs and resolve by stable metadata ID, not by filename. Tests reject any
filename that differs from the provider's embedded canonical name. APT package
upgrades remove the package-owned old short-name files; no aliases are shipped.

## Chronology-domain consistency

Field extraction is authoritative for whether a provider supports an input
JDN. Text formatting, period start and navigation may never use a broader
chronology simply because ICU or another renderer knows one. In particular,
Umm al-Qura is bounded to AH 1300–1600 across all operations. Numerical
extremes must be rejected with bounded work and without mutating caller-owned
output on failure. This contract is exercised through all resource ABIs.

## Numeric conversion boundaries

ICU-backed date providers validate the JDN against ICU's finite signed-
millisecond horizon before conversion, formatting, period navigation or
reverse numeric conversion. Invalid dates are rejected explicitly.
Independent French Republican and Revised Julian chronologies use a wider
binary64/year-boundary-safe domain rather than ICU's unrelated limit.

## Persian and Maya arithmetic boundaries

Persian year estimation is bounded by the same UTC-noon day horizon used for
ICU presentation. Its year estimate is checked against actual year starts and
both month and day are validated before lookup or publication. Failure applies
consistently to fields, formatting and period navigation, without rewriting
caller output. The Maya Long Count checks signed epoch subtraction and reserves
a tun of intermediate headroom at both signed-JDN endpoints; it does not
substitute a saturated but plausible historical date.

## Reversible ancient dates and unavailable polar events

Egyptian Nabonassar and traditional Armenian date conversion now rejects
non-reversible signed-JDN extremes: intermediate arithmetic must be checked,
not saturated and presented as historic. Hebrew fields and ICU formatting
share the same finite UTC-noon domain.

Seasonal clocks return a one-minute retry for an undefined polar solar event
without probing a synthetic 36-hour timeline. For valid clock faces, next-tick
scheduling continues to require an actual visible change.

## Civil-year and historical-year refusal

The proleptic Gregorian, Julian, ISO-week and derived historical resource
adapters reject JDNs whose conversion would narrow a civil year outside
GLib's `gint` representation. These limits follow actual Gregorian/Julian
year endpoints rather than arbitrary epoch windows. A failed Bahá'í year-start
calculation likewise propagates failure through fields, formatting and
navigation; the genuine 1844 epoch must never serve as an error fallback.
