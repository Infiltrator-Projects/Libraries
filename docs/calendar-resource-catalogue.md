# System calendar catalogue

Common exposes the normal host Gregorian calendar through platform date and
locale services. Installed, validated `.so` implementations add capabilities
under `/System/Resources/Calendars`; no custom chronology is used when its
module is missing. The default clock likewise follows normal system time.

The thirty-one optional calendars and twenty-three clocks are built and released
by their independent repositories. Each package owns one executable module and
its passive resources. Common keeps metadata, discovery, loading and host/base
APIs; its original implementation source is preserved without a production
provider build. InfiltratorOS compiles their published source installers locally.

Use `infiltratr_temporal_installed_calendar_*` to enumerate normal and installed
capabilities. Both metadata and provider ABI validate before a native module is
selectable. `infiltratr_temporal_calendar_module_open_system()` explicitly opens
the host Gregorian adapter without consulting resource folders.
