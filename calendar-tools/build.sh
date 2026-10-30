#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Native shell/GCC packaging for Infiltrator calendar modules. No Python is used.
set -euo pipefail
cd "$(dirname "$0")/../.."
root=$PWD
command=${1:-}; shift || :
profile=aggressive
output=dist
build_only=0
system_package_mode=0
jobs=$(nproc 2>/dev/null || printf 2); ((jobs>8)) && jobs=8
while (($#)); do
  case $1 in
    --profile) profile=${2:?missing profile};shift 2;;
    --output) output=${2:?missing output};shift 2;;
    --jobs) jobs=${2:?missing jobs};shift 2;;
    --build-only) build_only=1;shift;;
    --system-package-mode) system_package_mode=1;shift;;
    --help|-h) cat <<'HELP'
Infiltrator native calendar installer
  --profile aggressive|native|portable   CPU optimisation profile (default: aggressive)
  --build-only --output DIRECTORY        Build and validate without installing
  --system-package-mode                  Permit OS-owned root installation
HELP
      exit 0;;
    *) echo "Unknown installer option: $1" >&2;exit 2;;
  esac
done
case $profile in aggressive|native|portable);;*) echo "Invalid profile $profile" >&2;exit 2;;esac
[[ $jobs =~ ^[0-9]+$ && $jobs -gt 0 ]] || exit 2
case $command in release|deb|install);;*) echo 'Usage: build.sh release|deb|install [options]' >&2;exit 2;;esac
if [[ $command == install && $EUID -eq 0 && $system_package_mode != 1 && $build_only != 1 ]]; then
  echo 'Run as ordinary user unless using --system-package-mode' >&2;exit 2
fi
if [[ $command == install && $build_only == 1 && $output == dist ]]; then
  echo 'Build-only must specify --output DIRECTORY' >&2;exit 2
fi
name=$(jq -r '.name' SOURCE-MANIFEST.json)
repository=$(jq -r '.repository' SOURCE-MANIFEST.json)
id=$(jq -r '.id' SOURCE-MANIFEST.json)
version=$(<VERSION)
[[ $version =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo 'Invalid version' >&2;exit 1; }
slug=${repository,,}
package="infiltrator-$slug"
resource="System/Resources/Calendars/$name.so"
guide="System/Resources/Documentation/$repository/$name.pdf"
if ! command -v jq >/dev/null 2>&1; then
  if [[ $command == install && $build_only == 0 ]]; then
    apt=(apt-get -o DPkg::Lock::Timeout=300); ((EUID!=0)) && apt=(sudo "${apt[@]}")
    "${apt[@]}" update; "${apt[@]}" install -y --no-install-recommends jq
  else echo "Missing jq build prerequisite" >&2; exit 1; fi
fi
source_revision=$(bash common/calendar-tools/source-revision.sh)
# Only these tools are build-time requirements. The installed .so/PDF need none of them.
prereqs=(gcc g++ msgfmt make pkg-config dpkg dpkg-deb readelf strip jq ps2pdf tar sha256sum awk sed)
if [[ $command == release ]]; then prereqs+=(zip gzip); fi
missing=()
for bin in "${prereqs[@]}"; do command -v "$bin" >/dev/null || missing+=("$bin"); done
for pc in $(jq -r '.build_dependencies[]' SOURCE-MANIFEST.json); do
  if command -v pkg-config >/dev/null && ! pkg-config --exists "$pc"; then missing+=("$pc"); fi
done
if ((${#missing[@]})); then
  if [[ $command == install && $build_only == 0 ]]; then
    echo "Installing missing native build prerequisites: ${missing[*]}"
    apt=(apt-get -o DPkg::Lock::Timeout=300)
    if ((EUID!=0)); then apt=(sudo "${apt[@]}"); fi
    "${apt[@]}" update
    packages=(build-essential binutils pkg-config jq ghostscript tar gzip zip gettext locales)
    if jq -e '.build_dependencies | index("glib-2.0") != null' SOURCE-MANIFEST.json >/dev/null; then packages+=(libglib2.0-dev); fi
    if jq -e '.build_dependencies | index("icu-i18n") != null' SOURCE-MANIFEST.json >/dev/null; then packages+=(libicu-dev); fi
    "${apt[@]}" install -y --no-install-recommends "${packages[@]}"
  else
    printf 'Missing build prerequisites: %s\n' "${missing[*]}" >&2;exit 1
  fi
fi
for bin in "${prereqs[@]}"; do command -v "$bin" >/dev/null || { echo "Still missing $bin" >&2;exit 1; };done
for pc in $(jq -r '.build_dependencies[]' SOURCE-MANIFEST.json); do pkg-config --exists "$pc" || { echo "Still missing $pc" >&2;exit 1; };done
[[ -f common/VERSION ]] || { echo 'Missing pinned Common sources' >&2;exit 1; }
mkdir -p build
build_package() {
  local tune=$1 targetdir=$2 flags profileflags=() native=0
  local ver=$version
  [[ $tune == portable ]] || { ver="${ver}+native1"; native=1; }
  mkdir -p "$targetdir"
  targetdir=$(realpath "$targetdir")
  local machine
  machine=$(uname -m)
  flags='-g0 -pipe -flto -DNDEBUG -fPIC -std=c11 -O2'
  if [[ $tune == portable ]]; then
    [[ $machine == x86_64 ]] || { echo 'Portable package requires amd64' >&2;return 1; }
    flags+=' -march=x86-64 -mtune=generic'
  else
    flags='-g0 -pipe -flto -DNDEBUG -fPIC -std=c11 -O3'
    if [[ $machine == x86_64 ]]; then flags+=' -march=native -mtune=native'; else flags+=' -mcpu=native'; fi
  fi
  local pgo=disabled
  rm -rf build/package build/pgo
  mkdir -p build/package
  if [[ $tune == aggressive ]]; then
    pgo=trained-two-pass
    mkdir -p build/pgo
    CFLAGS="$flags -fprofile-generate=$root/build/pgo -fprofile-update=atomic" \
      LDFLAGS="-flto -fprofile-generate=$root/build/pgo -Wl,-z,relro,-z,now" \
      make --no-print-directory -j"$jobs" BUILD_DIR=build/package check
    build/provider-check "$(realpath "build/package/$name.so")" "$id" "$name" train
    find build/pgo -name '*.gcda' -print -quit | grep -q . || { echo 'PGO produced no measurements' >&2;return 1; }
    rm -rf build/package
    mkdir -p build/package
    CFLAGS="$flags -fprofile-use=$root/build/pgo -fprofile-correction -fprofile-partial-training" \
      LDFLAGS="-flto -fprofile-use=$root/build/pgo -Wl,-z,relro,-z,now" \
      make --no-print-directory -j"$jobs" BUILD_DIR=build/package check
  else
    CFLAGS="$flags" LDFLAGS='-flto -Wl,-z,relro,-z,now' \
      make --no-print-directory -j"$jobs" BUILD_DIR=build/package check
  fi
  local library="build/package/$name.so"
  strip --strip-unneeded "$library"
  bash common/calendar-tools/check.sh "$library"
  if readelf -Ws "$library" | grep -q __gcov; then echo 'PGO instrumented binary leaked' >&2;return 1;fi
  local stage=$(mktemp -d)
  mkdir -p "$stage/$(dirname "$resource")" "$stage/$(dirname "$guide")" "$stage/DEBIAN"
  install -m 0644 "$library" "$stage/$resource"
  bash common/calendar-tools/guide.sh "$stage/$guide"
  chmod 0644 "$stage/$guide"
  local libc_min
  libc_min=$(readelf --version-info "$library" | grep -oE 'GLIBC_[0-9]+(\.[0-9]+)+' | sed 's/^GLIBC_//' | sort -Vu | tail -1)
  libc_min=${libc_min:-2.34}
  local deps="libc6 (>= $libc_min)"
  if jq -e '.build_dependencies | index("glib-2.0") != null' SOURCE-MANIFEST.json >/dev/null; then deps+=', libglib2.0-0 (>= 2.74) | libglib2.0-0t64 (>= 2.74)'; fi
  if jq -e '.build_dependencies | index("icu-i18n") != null' SOURCE-MANIFEST.json >/dev/null; then
    deps+=', libicu72 | libicu73 | libicu74 | libicu75 | libicu76 | libicu77 | libicu78 | libicu79 | libicu80 | libicu81'
  fi
  local checksum arch
  checksum=$(sha256sum "$stage/$resource" | cut -d' ' -f1)
  arch=$(dpkg --print-architecture)
  cat > "$stage/DEBIAN/control" <<CONTROL
Package: $package
Version: $ver
Architecture: $arch
Section: libs
Priority: optional
Maintainer: Infiltrator Projects <noreply@infiltrator-projects.invalid>
Depends: $deps
Replaces: infiltrator-calendar-resources
Homepage: https://github.com/Infiltrator-Projects/$repository
X-Infiltrator-Source-Revision: $source_revision
X-Infiltrator-Native-Profile: $tune
X-Infiltrator-LTO: enabled
X-Infiltrator-PGO: $pgo
X-Infiltrator-Cflags: $flags
X-Infiltrator-Resource-SHA256: $checksum
Description: $name temporal calendar resource
 Native standalone calendar provider with passive illustrated PDF documentation.
CONTROL
  local destination="$targetdir/${package}_${ver}_${arch}.deb"
  dpkg-deb --root-owner-group -Zxz --build "$stage" "$destination"
  bash common/calendar-tools/check-package.sh "$destination" "$tune"
  rm -rf "$stage"
  echo "$destination"
}
case $command in
  deb)
    build_package "$profile" "$output";;
  install)
    if ((build_only)); then build_package "$profile" "$output"; exit; fi
    tmp=$(mktemp -d);trap 'rm -rf "$tmp"' EXIT
    build_package "$profile" "$tmp"
    found=("$tmp"/*.deb)
    if ((EUID==0));then dpkg --install "${found[0]}";else sudo -- dpkg --install "${found[0]}";fi;;
  release)
    mkdir -p "$output"
    output=$(realpath "$output")
    [[ -z $(find "$output" -mindepth 1 -maxdepth 1 -print -quit) ]] || { echo 'Release directory must be empty' >&2;exit 1; }
    build_package portable "$output"
    temp=$(mktemp -d);trap 'rm -rf "$temp"' EXIT
    mkdir -p "$temp/source"
    tar -cf - --exclude='./.git' --exclude='*/.git' --exclude='./build' --exclude='./dist' --exclude='./.build' --exclude='*.pyc' . | tar -xf - -C "$temp/source"
    printf '%s\n' "$source_revision" > "$temp/source/SOURCE-REVISION"
    prefix="${slug}-${version}"
    (cd "$temp" && mv source "$prefix" && zip -qr "$output/${prefix}-source.zip" "$prefix" && mv "$prefix" source)
    (cd "$temp" && tar -czf "$temp/payload.tar.gz" source)
    hash=$(sha256sum "$temp/payload.tar.gz" | cut -d' ' -f1)
    sed "s/@PAYLOAD_SHA256@/$hash/g" common/calendar-tools/run-header.sh > "$output/${prefix}-native-installer.run"
    cat "$temp/payload.tar.gz" >> "$output/${prefix}-native-installer.run"
    chmod 0755 "$output/${prefix}-native-installer.run"
    [[ $(find "$output" -maxdepth 1 -type f | wc -l) == 3 ]] || { echo 'Expected exactly 3 release assets' >&2;exit 1; }
    printf 'Native, Python-free release generated: %s\n' "$prefix";;
esac
