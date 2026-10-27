#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Require exclusive provider source ownership by the independent repositories."""
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
assert not (ROOT / "resources/temporal").exists(), "Provider source, translations and model tests belong only in their independent repositories"
makefile = (ROOT / "Makefile").read_text()
for target in ("temporal-resources", "temporal-resource-package", "temporal-resource-check"):
    assert not re.search(r"^" + re.escape(target) + r"\s*:", makefile, re.MULTILINE), target
assert "resources/temporal" not in makefile
assert "tools/check-temporal-providers.sh" in makefile
assert "resources/temporal/src/" not in (ROOT / "CMakeLists.txt").read_text()
ci = (ROOT / ".github/workflows/ci.yml").read_text()
release = (ROOT / ".github/workflows/release.yml").read_text()
for text in (ci, release):
    assert "common-temporal-resources" not in text
    assert "temporal-resource-package" not in text
    assert "infiltrator-calendar-resources_" not in text
assert "temporal-provider-check" in ci
entries = json.loads((ROOT / "tools/temporal-providers.json").read_text())
assert len(entries) == 54 and len({x["package"] for x in entries}) == 54
assert sum(x["kind"] == "calendar" for x in entries) == 31
assert sum(x["kind"] == "clock" for x in entries) == 23
print("No provider source archive or production build/package targets in Common; 31 calendar and 23 clock repositories own their implementations")
