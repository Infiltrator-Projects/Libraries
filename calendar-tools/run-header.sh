#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Exact-source native bootstrap. No source, libraries or binaries are embedded.
set -Eeuo pipefail
readonly repository='@REPOSITORY@'
readonly version='@VERSION@'
readonly source_commit='@SOURCE_COMMIT@'
readonly common_commit='@COMMON_COMMIT@'
readonly source_url="https://github.com/Infiltrator-Projects/$repository.git"
[[ "$repository" =~ ^Calendar-[A-Za-z0-9-]+$ && "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ &&
   "$source_commit" =~ ^[0-9a-f]{40}$ && "$common_commit" =~ ^[0-9a-f]{40}$ ]] || exit 1
(( $(wc -c < "$0") < 16384 )) || { echo 'Unexpected embedded installer payload' >&2; exit 1; }
if [[ "${1:-}" == --help || "${1:-}" == -h ]]; then
  echo 'InfiltratorOS native calendar installer: --profile aggressive|native|portable, --build-only --output DIR, --system-package-mode'
  exit 0
fi
# Fetch Git objects at immutable revisions, as in the InfiltratorFS RUN.
# Do not follow a moving branch or accept a substituted Common checkout.
command -v git >/dev/null || { echo 'git is required to fetch source' >&2; exit 1; }
work=$(mktemp -d "${TMPDIR:-/tmp}/infiltrator-provider.XXXXXXXX")
trap 'rm -rf -- "$work"' EXIT
trap 'exit 1' HUP INT TERM
git init --quiet "$work/source"
git -C "$work/source" remote add origin "$source_url"
git -C "$work/source" fetch --quiet --depth=1 origin "$source_commit"
git -C "$work/source" checkout --quiet --detach FETCH_HEAD
[[ $(git -C "$work/source" rev-parse HEAD) == "$source_commit" ]]
[[ $(git -C "$work/source" ls-tree HEAD -- common | awk '$1 == "160000" {print $3}') == "$common_commit" ]]
[[ $(git -C "$work/source" config -f .gitmodules --get submodule.common.url) == https://github.com/Infiltrator-Projects/Libraries.git ]]
git -C "$work/source" submodule update --init --depth 1 -- common
[[ $(git -C "$work/source/common" rev-parse HEAD) == "$common_commit" ]]
[[ $(cat "$work/source/VERSION") == "$version" ]]
bash "$work/source/common/calendar-tools/build.sh" install "$@"
