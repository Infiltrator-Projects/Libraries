# Preserved temporal implementation source

The original Common 1.19.78 provider sources and translations remain here as a
reference copy. `ARCHIVED-SOURCE.json` verifies their original hashes. Production
modules, tests, translations, Debian packages and native installers now belong
to the 31 `Calendar-*` and 23 `Clock-*` repositories.

Common no longer has a provider Makefile or a combined resource package builder.
Its releases contain the Common SDK source; they do not publish
`infiltrator-calendar-resources` or any production calendar/clock `.so` files.
Existing historical releases remain unchanged.

Common retains public metadata/discovery/loading APIs, the host Gregorian
adapter, neutral coordinate/duration operations and synthetic ABI test fixtures.
`make temporal-provider-check` compiles only API consumer probes and loads
already installed independent modules. Main CI fetches their published generic
Debian packages with `tools/temporal-packages.py` and exercises the public ABI on
Debian 12, Ubuntu 24.04 and Debian 13. No provider source is rebuilt by Common.

Canonical installed paths remain `/System/Resources/Calendars/` and
`/System/Resources/Clocks/`. Provider metadata IDs and executable ABI version 1
remain unchanged. Removing a consumer does not remove these provider packages.
Custom-calendar translations use per-provider gettext domains and are included
in their owning calendar packages.

The notes below describe the retained original implementation. Current model
and historical-accuracy contracts are maintained by each independent provider.

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
