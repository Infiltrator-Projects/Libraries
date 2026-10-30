#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Execute original provider fixtures through the stable ABI in native C++17.
set -euo pipefail
cd "$(dirname "$0")/../.."
library=$(realpath "${1:?Shared object required}")
audit=${2:-}
id=$(jq -r .id SOURCE-MANIFEST.json)
name=$(jq -r .name SOURCE-MANIFEST.json)
mkdir -p build
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -Icommon/include \
  -o build/provider-check common/calendar-tools/provider_check.cpp -ldl
cases=(tests/reference-cases.json tests/accuracy-cases.json)
if [[ $audit == --authorities ]]; then cases=(tests/authority-cases.json); fi
count=0
for file in "${cases[@]}"; do
  [[ -f $file ]] || continue
  while IFS= read -r record; do
    mapfile -d '' -t fields < <(printf %s "$record" | base64 --decode | \
      jq -j '([.op] + .args + .expected)[] | (if . == null then "__NULL__" else tostring end), "\u0000"')
    LANGUAGE=en_US LC_ALL=C.UTF-8 build/provider-check "$library" "$id" "$name" "${fields[@]}"
    ((count+=1))
  done < <(jq -r '.[] | @base64' "$file")
done
printf '%s: %s native C++ fixture checks passed\n' "$id" "$count"
