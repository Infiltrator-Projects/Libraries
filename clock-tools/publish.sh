#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Publish only byte-qualified native artifacts for the current main commit.
set -euo pipefail
cd "$(dirname "$0")/../.."
: "${GITHUB_SHA:?GITHUB_SHA required}" "${GITHUB_REPOSITORY:?GITHUB_REPOSITORY required}" "${GH_TOKEN:?GH_TOKEN required}"
root=release
contract="$root/contract.json"
[[ -f $contract ]] || { echo 'Missing immutable qualification contract' >&2;exit 1; }
[[ $(jq -r .commit "$contract") == "$GITHUB_SHA" ]] || exit 1
version=$(jq -r .version "$contract")
name=$(jq -r .name "$contract")
mapfile -t assets < <(jq -r '.assets | keys[]' "$contract")
slug=${GITHUB_REPOSITORY##*/}
slug=${slug,,}
expected_deb="infiltrator-${slug}_${version}_amd64.deb"
expected_run="${slug}-${version}-native-installer.run"
(("${#assets[@]}"==2)) || { echo 'Release requires exactly two uploaded assets' >&2;exit 1; }
[[ $(printf '%s\n' "${assets[@]}") == "$(printf '%s\n' "$expected_deb" "$expected_run" | sort)" ]] || exit 1
bash -n "$root/$expected_run"
(( $(wc -c < "$root/$expected_run") < 16384 )) || exit 1
for filename in "${assets[@]}";do
  [[ $filename != */* && -f "$root/$filename" ]] || exit 1
  actual=$(sha256sum "$root/$filename" | cut -d' ' -f1)
  [[ $actual == "$(jq -r --arg f "$filename" '.assets[$f]' "$contract")" ]] || exit 1
done
head=$(gh api "repos/$GITHUB_REPOSITORY/git/ref/heads/main" --jq .object.sha)
if [[ $head != "$GITHUB_SHA" ]]; then echo 'Superseded main commit; release skipped'; exit 0;fi
tag="v$version"
if gh api "repos/$GITHUB_REPOSITORY/git/ref/tags/$tag" > /dev/null 2>&1;then
  [[ $(gh api "repos/$GITHUB_REPOSITORY/git/ref/tags/$tag" --jq .object.sha) == "$GITHUB_SHA" ]] || { echo 'Published tag belongs to a different commit; bump VERSION' >&2;exit 1; }
fi
paths=();for file in "${assets[@]}";do paths+=("$root/$file");done
if gh release view "$tag" --json isDraft > /dev/null 2>&1; then
  [[ $(gh release view "$tag" --json isDraft --jq .isDraft) == true ]] || { echo 'Refusing to overwrite immutable published release' >&2;exit 1; }
  gh release upload "$tag" "${paths[@]}" --clobber
else
  gh release create "$tag" "${paths[@]}" --target "$GITHUB_SHA" --draft --title "$name $version" --notes 'Native compiled clock, illustrated PDF, and Python-free build/installation. Exact payload and provider ABI checked.'
fi
check_remote() {
  local directory=$(mktemp -d)
  gh release download "$tag" --dir "$directory"
  mapfile -t remote < <(find "$directory" -mindepth 1 -maxdepth 1 -type f -printf '%f\n' | sort)
  [[ $(printf '%s\n' "${remote[@]}") == "$(printf '%s\n' "${assets[@]}")" ]] || { echo 'Remote release asset mismatch' >&2;exit 1; }
  for file in "${assets[@]}";do
    [[ $(sha256sum "$directory/$file" | cut -d' ' -f1) == "$(jq -r --arg f "$file" '.assets[$f]' "$contract")" ]] || exit 1
  done
  rm -rf "$directory"
}
check_remote
gh release edit "$tag" --draft=false --latest
check_remote
echo "Published and byte verified $tag"
