#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
cd "$(dirname "$0")/../.."
((EUID==0)) || { echo 'Needs root for isolated dpkg ownership check' >&2;exit 1; }
package_file=$(realpath "${1:?Package required}")
repository=$(jq -r '.repository' SOURCE-MANIFEST.json)
name=$(jq -r '.name' SOURCE-MANIFEST.json)
package="infiltrator-${repository,,}"
resource="System/Resources/Clocks/$name.so"
work=$(mktemp -d);trap 'rm -rf "$work"' EXIT
mkdir -p "$work/legacy/DEBIAN" "$work/legacy/$(dirname "$resource")"
cat > "$work/legacy/DEBIAN/control" <<'CONTROL'
Package: infiltrator-calendar-resources
Version: 1.19.78
Architecture: amd64
Maintainer: Infiltrator Projects <noreply@infiltrator-projects.invalid>
Description: isolated legacy ownership fixture
CONTROL
printf 'legacy module' > "$work/legacy/$resource"
printf 'unrelated module' > "$work/legacy/System/Resources/Clocks/Other Provider.so"
dpkg-deb --root-owner-group --build "$work/legacy" "$work/legacy.deb"
dpkg=(dpkg --root "$work/system")
"${dpkg[@]}" --unpack "$work/legacy.deb"
"${dpkg[@]}" --unpack "$package_file"
owner=$(dpkg-query --admindir "$work/system/var/lib/dpkg" --search "/$resource")
[[ $owner == "$package: /$resource" ]] || { echo "Ownership transfer failed: $owner" >&2; exit 1; }
[[ $(head -c 4 "$work/system/$resource" | od -An -tx1 | tr -d ' \n') == 7f454c46 ]] || exit 1
[[ -f "$work/system/System/Resources/Clocks/Other Provider.so" ]] || exit 1
"${dpkg[@]}" --remove "$package"
[[ ! -e "$work/system/$resource" && -e "$work/system/System/Resources/Clocks/Other Provider.so" ]] || exit 1
"${dpkg[@]}" --unpack "$package_file"
"${dpkg[@]}" --remove infiltrator-calendar-resources
[[ -e "$work/system/$resource" && ! -e "$work/system/System/Resources/Clocks/Other Provider.so" ]] || exit 1
echo "$package: legacy ownership transfer and independent removal passed"
