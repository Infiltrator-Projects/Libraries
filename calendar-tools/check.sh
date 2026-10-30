#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
cd "$(dirname "$0")/../.."
lib=$(realpath "${1:?library required}")
authorities=${2:-}
id=$(jq -r '.id' SOURCE-MANIFEST.json)
name=$(jq -r '.name' SOURCE-MANIFEST.json)
testbin=build/provider-check
mkdir -p build
${CXX:-g++} -std=c++20 -Wall -Wextra -Wpedantic -Werror -Icommon/include -o "$testbin" common/calendar-tools/provider_check.cpp -ldl
files=(tests/reference-cases.json tests/accuracy-cases.json)
if [[ $authorities == --authorities ]]; then files=(tests/authority-cases.json); fi
count=0
for file in "${files[@]}"; do
    [[ -f $file ]] || continue
    while IFS= read -r encoded; do
        mapfile -d '' -t fields < <(printf '%s' "$encoded" | base64 -d | jq -j '([.op] + .args + .expected)[] | (if . == null then "__NULL__" else tostring end), "\u0000"')
        LANGUAGE=en_US LC_ALL=C.UTF-8 "$testbin" "$lib" "$id" "$name" "${fields[@]}"
        ((count+=1))
    done < <(jq -r '.[] | @base64' "$file")
done
printf '%s: %d native fixture checks passed\n' "$id" "$count"
