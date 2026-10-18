#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
json="$root/design/infiltrator-design-v1.json"
css="$root/design/infiltrator-web-v1.css"

check_string() {
  local json_path=$1 css_name=$2 suffix=${3-}
  local value
  value=$(jq -er "$json_path" "$json")
  grep -Fq -- "$css_name$value$suffix" "$css"
}

check_string '.typography.ui_family' '--infiltratr-font-ui: "' '", sans-serif;'
check_string '.typography.brand_family' '--infiltratr-font-brand: "' '", sans-serif;'
check_string '.typography.ui_regular_weight' '--infiltratr-font-ui-regular-weight: ' ';'
check_string '.typography.ui_bold_weight' '--infiltratr-font-ui-bold-weight: ' ';'
check_string '.typography.brand_weight' '--infiltratr-font-brand-weight: ' ';'

legacy_keys=(background panel card surface input border text title muted subtle
  button_background button_foreground selection_background selection_foreground
  neutral_accent success warning fault info)
theme_keys=("${legacy_keys[@]}" operation card_hover surface_hover operation_hover equals_hover titlebar connection connection_border heading summary kicker detail_label note status_border accent_foreground accent_hover selected_summary warning_muted warning_border success_border)

for key in "${legacy_keys[@]}"; do
  test "$(jq -er ".palette.$key" "$json")" = "$(jq -er ".theme.palettes.night.$key" "$json")"
done

for mode in night day; do
  for key in "${theme_keys[@]}"; do
    css_key=${key//_/-}
    value=$(jq -er ".theme.palettes.$mode.$key" "$json")
    grep -Fq -- "--infiltratr-$css_key: $value;" "$css"
  done
done

grep -Fq ':root[data-infiltratr-theme="day"]' "$css"
grep -Fq ':root[data-infiltratr-theme="night"]' "$css"
grep -Fq ':root[data-infiltratr-theme="system"]' "$css"
grep -Fq '@media (prefers-color-scheme: light)' "$css"

for key in small_radius control_radius card_radius panel_radius compact_spacing \
  control_spacing section_spacing content_padding screen_padding; do
  css_key=${key//_/-}
  check_string ".metrics.$key" "--infiltratr-$css_key: " 'px;'
done

for mode in day night; do
  for key in "${theme_keys[@]}"; do
    value=$(jq -er ".theme.palettes.$mode.$key" "$json")
    hex=${value#\#}
    field=${key}_rgb
    grep -Fq -- ".$field = 0x${hex}U" "$root/src/design.c"
  done
done
grep -Fq 'INFILTRATR_THEME_SYSTEM' "$root/include/infiltratr/design.h"

for key in small_radius control_radius card_radius panel_radius compact_spacing \
  control_spacing section_spacing content_padding screen_padding; do
  value=$(jq -er ".metrics.$key" "$json")
  grep -Fq -- ".$key = ${value}U" "$root/src/design.c"
done

ui_family=$(jq -er '.typography.ui_family' "$json")
brand_family=$(jq -er '.typography.brand_family' "$json")
grep -Fq -- ".ui_family = \"$ui_family\"" "$root/src/design.c"
grep -Fq -- ".brand_family = \"$brand_family\"" "$root/src/design.c"

assets="$root/cmake/InfiltratrTypographyAssets.cmake"
source_commit=$(jq -er '.typography.assets.source_commit' "$json")
source_repository=$(jq -er '.typography.assets.source_repository' "$json")
source_path=$(jq -er '.typography.assets.font_base_path' "$json")
grep -Fq -- "\"$source_commit\"" "$assets"
grep -Fq -- "\"$source_repository\"" "$assets"
grep -Fq -- "\"$source_path\"" "$assets"
for role in brand_regular ui_bold ui_regular; do
  filename=$(jq -er ".typography.font_files.$role" "$json")
  sha=$(jq -er ".typography.assets.file_sha256.$role" "$json")
  grep -Fq -- "\"$filename\"" "$assets"
  grep -Fq -- "\"$sha\"" "$assets"
done

grep -Fq 'INFILTRATR_MB_CORPO_NATIVE_FONT_DIR "/System/Resources/Fonts"' "$assets"
grep -Fq '/etc/fonts/conf.d/90-infiltrator-native-fonts.conf' "$assets"
grep -Fq 'InfiltratrEnsureNativeFonts.sh' "$assets"

font_bootstrap="$root/cmake/InfiltratrEnsureNativeFonts.sh"
test -f "$font_bootstrap"
sh -n "$font_bootstrap"
grep -Fq '/System/Resources/Fonts' "$font_bootstrap"
grep -Fq 'native_path /etc/fonts/conf.d' "$font_bootstrap"
grep -Fq '90-infiltrator-native-fonts.conf' "$font_bootstrap"
grep -Fq "$source_commit" "$font_bootstrap"
grep -Fq "$source_repository" "$font_bootstrap"
grep -Fq "$source_path" "$font_bootstrap"
for role in brand_regular ui_bold ui_regular; do
  filename=$(jq -er ".typography.font_files.$role" "$json")
  sha=$(jq -er ".typography.assets.file_sha256.$role" "$json")
  grep -Fq "$filename" "$font_bootstrap"
  grep -Fq "$sha" "$font_bootstrap"
done
if grep -Eq '(^|[^[:alnum:]_])(rm|rmdir)[[:space:]].*/System/Resources/Fonts' "$font_bootstrap"; then
  echo "native font bootstrap must never remove the shared font directory" >&2
  exit 1
fi

echo "PASS: JSON, C, CMake and web design adapters match the canonical design contract"
