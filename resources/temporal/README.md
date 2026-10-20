# Shared system temporal resources

Common owns the source, build, tests, Debian package and releases of these
31 calendar and 23 clock implementations. The modules are separately compiled build products. The compiler emits the
complete human-readable canonical filenames directly, and the same name is
embedded in their metadata. Stable provider IDs are independent of filenames;
there are no aliases, post-build renames or display-only disguises. The selectable provider implementations are loaded from their system
files; Calendar, Files and System Settings do not compile or replace them.
Neutral coordinate and formatting helpers remain shared Common APIs.
The compact NOAA/GML solar mathematics is maintained in a single private
Common header, compiled separately into the relevant `.so` providers
without linking their lifetimes together. Distinct historic rules remain
implemented and qualified in each owning clock.

From the Common repository, run `make temporal-resource-check` or
`make temporal-resource-package`. The Debian package retains its existing
`infiltrator-calendar-resources` identity for ordinary APT upgrades. It installs
modules under `/System/Resources/Calendars` and `/System/Resources/Clocks`.
Its version and release belong to Common, independently of every application.

Normal system date/time capabilities work without this optional module pack.
Applications discover additional capabilities through Common. Missing custom
implementations remain unavailable rather than using an embedded custom copy.

`ORIGIN` records the preserved source import. The build has no dependency on
the Calendar repository, its private headers, its application library or its
release pipeline.

Neutral civil/JDN coordinate arithmetic also lives in Common. Provider and
application ABI adapters delegate to it rather than retaining arithmetic copies.

Release modules are built on Debian 12 (glibc 2.36 / GLib 2.74 / ICU 72).
Common CI installs and executes those exact package bytes on Debian 12, Debian 13
and Ubuntu 24.04. Newer build-host headers must not raise the shipped GLib ABI.
`tools/check-installed.sh` tests every installed provider through the public ABI
without rebuilding any implementation.

## Compiled resource isolation

The full calendar engine archive is retained for direct unit qualification.
Extracted custom calendars compile a single selected descriptor and collect
unreferenced functions/data at the resource link step; this prevents a Julian
calendar binary, for example, from carrying Maya and French Republican rules.
The boundary checker inspects internal symbols in representative `.so` files.

Each provider's fields operation defines its supported date domain. Formatting
and navigation must reject the same unsupported inputs, including dates beyond
the bounded Umm al-Qura table. Extreme saturated Julian-day conversions must
terminate with an explicit unsupported result rather than looping indefinitely.
