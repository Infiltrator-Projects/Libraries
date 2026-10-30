#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
cd "$(dirname "$0")/../.."
package_file=$(realpath "${1:?Package required}")
profile=${2:?Profile required}
name=$(jq -r .name SOURCE-MANIFEST.json)
repository=$(jq -r .repository SOURCE-MANIFEST.json)
expected="infiltrator-${repository,,}"
version=$(<VERSION)
[[ $profile == portable ]] || version+='+native1'
[[ $(dpkg-deb -f "$package_file" Package) == "$expected" ]] || exit 1
[[ $(dpkg-deb -f "$package_file" Version) == "$version" ]] || exit 1
# Expected published calendar source revision: verify actual DEB control
source_revision=$(bash common/calendar-tools/source-revision.sh)
[[ $(dpkg-deb -f "$package_file" X-Infiltrator-Source-Revision) == "$source_revision" ]] || { echo "Missing or mismatched calendar source revision in native package" >&2; exit 1; }
[[ $(dpkg-deb -f "$package_file" X-Infiltrator-Native-Profile) == "$profile" ]] || exit 1
[[ $(dpkg-deb -f "$package_file" X-Infiltrator-LTO) == enabled ]] || exit 1
if [[ $profile == aggressive ]]; then
  [[ $(dpkg-deb -f "$package_file" X-Infiltrator-PGO) == trained-two-pass ]] || exit 1
else
  [[ $(dpkg-deb -f "$package_file" X-Infiltrator-PGO) == disabled ]] || exit 1
fi
work=$(mktemp -d);trap 'rm -rf "$work"' EXIT
dpkg-deb --extract "$package_file" "$work"
mapfile -d '' -t files < <(find "$work" -type f -print0)
((${#files[@]}==2)) || { echo "Expected exactly two installed files; found ${#files[@]}" >&2;exit 1; }
lib="$work/System/Resources/Calendars/$name.so"
pdf="$work/System/Resources/Documentation/$repository/$name.pdf"
[[ -f "$lib" && -f "$pdf" && ! -L "$lib" && ! -L "$pdf" ]] || exit 1
[[ $(head -c 5 "$pdf") == '%PDF-' ]] || exit 1
[[ $(stat -c '%a' "$lib") == 644 && $(stat -c '%a' "$pdf") == 644 ]] || exit 1
[[ $(sha256sum "$lib" | cut -d' ' -f1) == "$(dpkg-deb -f "$package_file" X-Infiltrator-Resource-SHA256)" ]] || exit 1
! readelf --dyn-syms --wide "$lib" | grep -q __gcov || exit 1
bash common/calendar-tools/check.sh "$lib"
echo "$expected: verified exactly one native calendar .so, one illustrated PDF, no installed build dependencies"
