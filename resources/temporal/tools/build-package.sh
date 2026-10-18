#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
cd "$(dirname "$0")/.."
build=${1:-build}
version=$(tr -d '\r\n' < ../../VERSION)
architecture=$(dpkg --print-architecture)
epoch=${SOURCE_DATE_EPOCH:-$(git -C ../.. show -s --format=%ct HEAD)}
stage="$build/package-root"
output="$PWD/$build/dist"
rm -rf "$stage"
mkdir -p "$stage/DEBIAN" "$stage/System/Resources/Calendars" \
    "$stage/System/Resources/Clocks" "$stage/usr/share/doc/infiltrator-calendar-resources" "$output"
install -m 0644 "$build/calendar-resources/"*.so "$stage/System/Resources/Calendars/"
install -m 0644 "$build/clock-resources/"*.so "$stage/System/Resources/Clocks/"
while IFS= read -r language; do
    case "$language" in ''|'#'*) continue ;; esac
    destination="$stage/usr/share/locale/$language/LC_MESSAGES"
    mkdir -p "$destination"
    msgfmt --check -o "$destination/infiltrator-temporal-resources.mo" "i18n/$language.po"
done < i18n/LINGUAS
install -m 0644 ../../LICENSE README.md ORIGIN "$stage/usr/share/doc/infiltrator-calendar-resources/"
cat > "$stage/DEBIAN/control" <<EOF
Package: infiltrator-calendar-resources
Version: $version
Architecture: $architecture
Section: libs
Priority: optional
Maintainer: Infiltrator Projects <noreply@infiltrator-projects.invalid>
Homepage: https://github.com/Infiltrator-Projects/Libraries
Depends: libc6 (>= 2.36), libglib2.0-0t64 (>= 2.74) | libglib2.0-0 (>= 2.74), libicu72 | libicu73 | libicu74 | libicu75 | libicu76 | libicu77 | libicu78 | libicu79 | libicu80 | libicu81
Description: Shared system calendar and clock implementations from Infiltrator Common
 Installs independently loadable modules in /System/Resources/Calendars and
 /System/Resources/Clocks. Common owns their source, build, ABI and release.
 Calendar, Files, System Settings and the desktop discover these optional
 capabilities. No application is required to build or use this package.
EOF
(cd "$stage" && find System usr -type f -print0 | LC_ALL=C sort -z | xargs -0 md5sum > DEBIAN/md5sums)
find "$stage" -exec touch --date="@$epoch" {} +
dpkg-deb --root-owner-group -Zxz --threads-max=1 --build "$stage" \
    "$output/infiltrator-calendar-resources_${version}_${architecture}.deb"
