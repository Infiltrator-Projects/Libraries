#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "${1:-}" = '--help' ] || [ "${1:-}" = '-h' ]; then
  echo 'InfiltratorOS native clock source installer: --profile aggressive|native|portable, --build-only --output DIR, --system-package-mode'
  exit 0
fi
for cmd in sh tar awk tail sha256sum; do command -v "$cmd" >/dev/null 2>&1 || { echo "Missing bootstrap utility: $cmd" >&2; exit 1; }; done
work=$(mktemp -d "${TMPDIR:-/tmp}/infiltrator-provider.XXXXXXXX")
trap 'rm -rf -- "$work"' EXIT HUP INT TERM
line=$(LC_ALL=C awk '/^__INFILTRATOR_PROVIDER_SOURCE__$/ { print NR + 1; exit }' "$0")
test -n "$line"
tail -n +"$line" "$0" > "$work/source.tar.gz"
printf '%s  %s\n' '@PAYLOAD_SHA256@' "$work/source.tar.gz" | sha256sum -c - >/dev/null
tar -xzf "$work/source.tar.gz" -C "$work" --no-same-owner --no-same-permissions
bash "$work/source/common/clock-tools/build.sh" install "$@"
exit 0
__INFILTRATOR_PROVIDER_SOURCE__
