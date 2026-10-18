# Shared system temporal resources

Common owns the source, build, tests, Debian package and releases of these
30 calendar and 23 clock implementations. The modules are separate build
products; none of their custom algorithms are compiled into Common's consumer
libraries or into Calendar, Files or System Settings.

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
