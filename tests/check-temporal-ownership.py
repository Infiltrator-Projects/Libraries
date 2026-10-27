#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Protect preserved sources and the independent production-build boundary."""
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
resource = ROOT / "resources/temporal"
manifest = json.loads((resource / "ARCHIVED-SOURCE.json").read_text())
assert {str(p.relative_to(resource)) for folder in ("src", "i18n") for p in (resource / folder).rglob("*") if p.is_file()} == set(manifest["sha256"])
for name, digest in manifest["sha256"].items():
    assert hashlib.sha256((resource / name).read_bytes()).hexdigest() == digest, name
assert not (resource / "Makefile").exists()
assert not (resource / "tools/build-package.sh").exists()
makefile = (ROOT / "Makefile").read_text()
for target in ("temporal-resources", "temporal-resource-package", "temporal-resource-check"):
    assert not re.search(r"^" + re.escape(target) + r"\s*:", makefile, re.MULTILINE), target
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
print(f"Preserved {len(manifest['sha256'])} original source/catalogue files; production provider build/package targets retired")
