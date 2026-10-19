# System calendar catalogue

Common exposes the normal host Gregorian calendar through platform date and
locale services. Installed, validated `.so` implementations add capabilities
under `/System/Resources/Calendars`; no custom chronology is used when its
module is missing. The default clock likewise follows normal system time.

Common owns all thirty-one optional calendar implementations and twenty-three clock
implementations in `resources/temporal`. Their `.deb` and `.so` releases are
published by Common and distributed through APT. Calendar, Files and System
Settings own neither the source nor their build/release pipeline.

Use `infiltratr_temporal_installed_calendar_*` to enumerate normal and installed
capabilities. Both metadata and provider ABI validate before a native module is
selectable. `infiltratr_temporal_calendar_module_open_system()` explicitly opens
the host Gregorian adapter without consulting resource folders.
