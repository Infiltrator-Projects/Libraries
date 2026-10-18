# Calendar resource catalogue

All calendars are executable resources discovered under `/System/Resources/Calendars`.
Common owns no compiled calendar implementation or calendar catalogue. Its old
portable catalogue functions return an empty view for ABI compatibility.

Use `infiltratr_temporal_installed_calendar_*` for installed executable calendars.
Metadata and provider ABI must both validate before the resource is selectable.
Calendar Resources supplies thirty independent calendar IDs; neither Calendar nor
System Settings owns their implementations or controls their lifetime.
