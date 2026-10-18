#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Keep provider modules independently executable and their exports minimal."""
from pathlib import Path
import re
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
build = Path(sys.argv[1])
for directory, count, entry in [
    ("calendar-resources", 30, "infiltratr_temporal_calendar_provider_v1"),
    ("clock-resources", 23, "infiltratr_temporal_clock_provider_v1"),
]:
    modules = sorted((build / directory).glob("*.so"))
    assert len(modules) == count, (directory, len(modules))
    for module in modules:
        dependencies = subprocess.check_output(["readelf", "-d", str(module)], text=True)
        assert "libcalendar" not in dependencies, module
        symbols = subprocess.check_output(["nm", "-D", "--defined-only", "--format=posix", str(module)], text=True)
        exported = {line.split()[0].split("@")[0] for line in symbols.splitlines()
                    if line.startswith("infiltratr_")}
        assert exported == {entry, "infiltratr_temporal_resource_metadata_v1"}, (module, exported)
for path in (root / "src").rglob("*.[ch]"):
    text = path.read_text()
    assert not re.search(r'#\s*include\s*[<"](?:.*src/core/|calendar-internal\.h|calendar-core\.h)', text), path
print("53 independently loadable modules; no Calendar application build or ABI dependency")
