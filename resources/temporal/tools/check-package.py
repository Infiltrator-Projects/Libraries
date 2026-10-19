#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Verify the exact installable resource package independently of applications."""
from pathlib import Path
import subprocess
import sys
import tarfile
import io

deb = Path(sys.argv[1])
def field(name):
    return subprocess.check_output(["dpkg-deb", "-f", str(deb), name], text=True).strip()
assert field("Package") == "infiltrator-calendar-resources"
assert field("Version") == (Path(__file__).resolve().parents[3] / "VERSION").read_text().strip()
assert field("Homepage") == "https://github.com/Infiltrator-Projects/Libraries"
assert "calendar (" not in field("Depends") and "system-settings" not in field("Depends")
payload = subprocess.check_output(["dpkg-deb", "--fsys-tarfile", str(deb)])
with tarfile.open(fileobj=io.BytesIO(payload)) as archive:
    files = {m.name.removeprefix("./"): m for m in archive.getmembers() if m.isfile()}
    for kind, expected in [("Calendars", 30), ("Clocks", 23)]:
        modules = [p for p in files if p.startswith("System/Resources/" + kind + "/") and p.endswith(".so")]
        assert len(modules) == expected, (kind, len(modules))
        assert all(" " in Path(p).name for p in modules), (kind, "old short-name .so")
        if "--installed" in sys.argv:
            for name in modules:
                assert Path("/" + name).read_bytes() == archive.extractfile(files[name]).read(), name
    assert not any("cinnamon/applets" in p or "libcalendar" in p for p in files)
    assert "usr/share/doc/infiltrator-calendar-resources/ORIGIN" in files
print("Common resource package verified: 30 calendars, 23 clocks, independent ownership")
