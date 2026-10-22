# Shared system temporal resources

Common owns the source, build, tests, Debian package and releases of these
31 calendar and 23 clock implementations. Every module is independently
compiled, directly named using its canonical human-readable filename, and
identified by stable metadata. No aliases or post-build renames are needed.
Calendar, Files and System Settings load these installed capabilities rather
than building or embedding private implementations.
Neutral coordinate and formatting helpers remain shared Common APIs.
One private fixed-day helper owns signed modulo and UTC-offset arithmetic,
compiled into each using clock without coupling the installed resource files.
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

The Gregorian, Coptic, Persian, and Hebrew native adapters share one private
ICU text-and-buffer helper compiled into each separate module. Their chronology,
era/year semantics and navigation remain distinct, with no shared runtime
resource dependency.

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

## Numeric contracts and deployment size

ICU-backed calendars validate Julian day numbers before converting to the
finite millisecond date representation, including checked reverse conversion.
French Republican and Revised Julian arithmetic use their own, wider
binary64/integer-safe estimation domain instead of inheriting ICU's limits.
The module names, historical rules and public ABIs are unchanged.

CI retains symbolized builds for debugging. Only deployed resource copies are
stripped of DWARF sections; exported functions and runtime ABI stay intact.
The reproducibility and cross-distribution tests qualify the staged bytes.

## Checked civil-date representation

Calendar resources using the 32-bit Gregorian or Julian civil-year inverse
validate JDN representability before narrowing the year. ISO weeks preserve
headroom for adjacent week-year rollover. Formatting and period navigation
share the same admissibility boundary and fail explicitly without substituting
a plausible but incorrect date.

The Egyptian Nabonassar and traditional Armenian wandering-year resources
reject non-reversible signed-JDN extremes rather than presenting saturated
epoch arithmetic as genuine calendar dates. Hebrew field extraction and text
formatting use the identical finite ICU UTC-noon domain.

Polar seasonal clocks with unavailable solar events schedule a one-minute
retry without searching 36 hours of unchanged N/A output.

The Badíʿ (Bahá'í) resource now distinguishes a failed year-start computation
from its historically meaningful 1844 epoch. Invalid years do not silently
become 21 March 1844, and a legitimate intercalary month numbered zero is
not confused with a failed field conversion.

Modern Badíʿ Naw-Rúz compares the equinox instant with the computed sunset
of its Tehran civil date in UTC. Civil midnight and apparent-solar midnight
are distinct boundaries; comparing wrapped solar fields can otherwise shift
the civil result by a day and produce an impossible year length. Failed sunset
calculation remains an explicit failure. The 2060 rollover and modern-year
length invariants are covered at the engine and executable resource boundary.
