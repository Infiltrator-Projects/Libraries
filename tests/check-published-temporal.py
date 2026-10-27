#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Pass independently installed modules to the Common-only C ABI probe."""
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
entries = json.loads((ROOT / "tools/temporal-providers.json").read_text())
calendar_root = Path(os.environ.get("INFILTRATR_CALENDAR_RESOURCE_DIRECTORY", "/System/Resources/Calendars"))
clock_root = Path(os.environ.get("INFILTRATR_CLOCK_RESOURCE_DIRECTORY", "/System/Resources/Clocks"))
installed = calendar_root == Path("/System/Resources/Calendars") and clock_root == Path("/System/Resources/Clocks")
arguments = [sys.argv[1], "--installed" if installed else "--direct"]
assert len(entries) == 54
for entry in entries:
    directory = calendar_root if entry["kind"] == "calendar" else clock_root
    path = directory / (entry["name"] + ".so")
    assert path.is_file(), path
    arguments += [entry["kind"], entry["id"], str(path), entry["name"]]
subprocess.run(arguments, check=True, env=dict(os.environ, LANGUAGE="en_US", LC_ALL="C.UTF-8"))
