# Architecture

## Purpose

Common is the canonical first-party shared library for portable mechanics, algorithms, design tokens, localisation, durable I/O and other behaviour that has proved genuinely reusable across the Infiltrator project family.

## Architectural model

The repository is intentionally layered:

- Portable core — dependency-light C interfaces for algorithms and data contracts that are meaningful across operating systems.
- Platform adapters — POSIX, dynamic-library and other host-facing code that translates platform mechanisms into stable Common contracts, including user/XDG path discovery, recursive directory creation and normalized absolute clock-deadline conversion where products would otherwise duplicate platform arithmetic or depend on toolkit helpers.
- Shared design/localisation — product-neutral visual tokens and localisation mechanics consumed by native and web-facing projects.
- Build and package surfaces — Make, CMake and installed-package metadata that let consumers use one reviewed implementation rather than private copies.
- Regression and consumer tests — contract tests that protect behaviour at the point where multiple projects now depend on it.

Common owns product-neutral behaviour only. Filesystem semantics remain in InfiltratorFS/Defragmenter, automotive semantics remain in Link and its product faces, and application-specific presentation remains with each application.

## Temporal resource discovery

Common owns generic discovery and validated executable loading. Each of the
31 calendars and 23 explicit clocks is built and released in its own repository,
with its own Debian package, native source installer and source ZIP. Common’s
original `resources/temporal` source is retained as a frozen reference, with no
provider build/package pipeline. The independent packages install modules under
`/System/Resources/Calendars/` and `/System/Resources/Clocks/`. Calendar, Files and
System Settings consume those modules through the public Common ABI.

Discovery is deterministic, validates executable provider ABI and ignores
metadata-only or malformed files. If several candidates claim the same ID,
the first *valid executable* in sorted directory order wins. Installed
catalogues are stable, per-process snapshots. Concurrent initialization
waits on a condition rather than busy-spinning, and recursive initialization
exposes only the base catalogue, not partially constructed optional entries.

A clock resource owns both rendering and its next-visible-tick calculation.
Common retains neutral compact solar calculations for elapsed-duration APIs.
Independent providers own their copied solar helpers, solar-event rules,
historical conventions and localized labels. The host Gregorian
calendar and OS-following clock selector remain available without the pack.

See [Temporal Providers](TEMPORAL_PROVIDERS.md) for the executable/trust
boundary and [Historical Fidelity](TEMPORAL_HISTORICAL_FIDELITY.md) for
accuracy claims and documented limitations.

## Ownership and dependency rule

The portable core is intentionally dependency-light. Consumers pin an exact reviewed Common revision; external libraries or platform APIs sit behind narrow adapters rather than defining Common semantics.

A function does not belong in Common merely because two files happen to look similar. Promotion requires a stable contract, clear ownership, tests that define edge behaviour, and evidence that centralising the implementation improves the project family rather than weakening the strongest caller.

## Source of truth

Public headers, implementation, tests and the exact tagged release together define Common behaviour. Documentation explains those contracts but does not override code or tests. USAGE.md defines consumer boundaries, while the design documents define presentation ownership.

## Change discipline

Shared code has a larger blast radius than product-local code. Changes therefore preserve ABI/API contracts where promised, add regression coverage for newly shared behaviour and avoid moving domain policy into the library. Consumers pin exact reviewed revisions so later Common development cannot silently change already-reviewed products.

## Related documentation

- USAGE.md
- design/README.md
- design/CONSUMERS.md
- docs/I18N.md
- docs/TEMPORAL_PROVIDERS.md
