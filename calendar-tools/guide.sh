#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Three-page illustrated human-readable PDF from the provider-owned GUIDE.json.
# Uses standard PostScript and Ghostscript at build-time; no Python.
set -euo pipefail
cd "$(dirname "$0")/../.."
pdf=${1:?PDF path is required}
[[ -f docs/GUIDE.json ]] || { echo 'Missing calendar guide facts' >&2;exit 1; }
command -v ps2pdf >/dev/null || { echo 'Missing ghostscript' >&2;exit 1; }
read_value() { jq -er --arg key "$1" '.[$key] // empty' docs/GUIDE.json; }
title=$(read_value title)
intro=$(read_value intro)
summary=$(read_value summary)
origin=$(jq -r '.origin // .epoch // "The calendar has its own documented historical starting convention."' docs/GUIDE.json)
structure=$(jq -r '.structure // .pattern_note // .summary' docs/GUIDE.json)
history=$(read_value history)
example=$(jq -r '.example // .pattern_note // .summary' docs/GUIDE.json)
limits=$(read_value limits)
source=$(read_value source)
diagram=$(jq -r '.diagram_title // "THE CALENDAR YEAR"' docs/GUIDE.json)
escape_ps() {
  printf '%s' "$1" | iconv -f UTF-8 -t ASCII//TRANSLIT -c |
    sed -e 's/\\/\\\\/g' -e 's/(/\\(/g' -e 's/)/\\)/g'
}
line() { printf '(%s) %s %s %s text\n' "$(escape_ps "$1")" "$2" "$3" "$4"; }
paragraph() {
  local content=$1 start=$2 size=$3
  while IFS= read -r row; do
    [[ -z $row ]] && continue
    line "$row" 52 "$start" "$size"
    ((start-=22)) || :
  done < <(printf '%s\n' "$content" | fold -s -w 79)
}
temp=$(mktemp -d);trap 'rm -rf "$temp"' EXIT
ps="$temp/guide.ps"
cat > "$ps" <<'PS'
%!PS-Adobe-3.0
%%Pages: 3
%%BoundingBox: 0 0 595 842
%%Title: Infiltrator Calendar Illustrated Guide
%%Creator: Infiltrator Projects Native Calendar Toolchain
/text { /sz exch def /yy exch def /xx exch def /words exch def
 /Helvetica findfont sz scalefont setfont xx yy moveto words show } bind def
/base {
  0.987 0.982 0.965 setrgbcolor 0 0 595 842 rectfill
  0.13 0.20 0.29 setrgbcolor 0 768 595 74 rectfill
  1 1 1 setrgbcolor (INFILTRATOR / CALENDARS) 52 794 13 text
  0.72 0.36 0.22 setrgbcolor 52 768 138 6 rectfill
  0.74 0.73 0.69 setrgbcolor 52 53 490 0.6 rectfill
  0.34 0.37 0.45 setrgbcolor (ILLUSTRATED REFERENCE GUIDE) 52 32 9 text
} bind def
/wheel {
  0.72 0.36 0.22 setrgbcolor 8 setlinewidth 315 310 83 0 360 arc stroke
  0.13 0.20 0.29 setrgbcolor 1.4 setlinewidth
  0 1 11 { /i exch def /a i 30 mul def
    315 a cos 70 mul add 310 a sin 70 mul add moveto
    315 a cos 93 mul add 310 a sin 93 mul add lineto stroke
  } for
  315 310 5 0 360 arc fill
} bind def
/sections {
  0.13 0.20 0.29 setrgbcolor 52 388 120 49 rectfill
  0.72 0.36 0.22 setrgbcolor 175 388 120 49 rectfill
  0.13 0.20 0.29 setrgbcolor 298 388 120 49 rectfill
  0.72 0.36 0.22 setrgbcolor 421 388 120 49 rectfill
} bind def
/timeline {
  0.13 0.20 0.29 setrgbcolor 5 setlinewidth 80 390 moveto 514 390 lineto stroke
  0.72 0.36 0.22 setrgbcolor 7 setlinewidth 310 390 moveto 514 390 lineto stroke
  80 390 8 0 360 arc fill
  310 390 8 0 360 arc fill
  514 390 8 0 360 arc fill
  0.13 0.20 0.29 setrgbcolor
  (START) 66 361 11 text
  (TRANSITION) 270 361 11 text
  (REFORM) 493 361 11 text
} bind def
PS
{
  echo '%%Page: 1 1'
  echo base
  echo '0.13 0.20 0.29 setrgbcolor'
  line 'WHAT IS THIS CALENDAR?' 52 708 22
  echo '0.72 0.36 0.22 setrgbcolor'
  line "$title" 52 654 16
  echo '0.13 0.20 0.29 setrgbcolor'
  paragraph "$intro" 615 11
  echo '0.94 0.91 0.85 setrgbcolor 46 446 503 118 rectfill'
  echo '0.13 0.20 0.29 setrgbcolor'
  line 'THE ESSENTIAL IDEA' 58 532 13
  paragraph "$summary" 501 11
  echo wheel
  echo '0.72 0.36 0.22 setrgbcolor'
  line 'WHERE THE CALENDAR BEGINS' 52 172 11
  echo '0.13 0.20 0.29 setrgbcolor'
  paragraph "$origin" 148 10
  echo '0.34 0.37 0.45 setrgbcolor'
  line '1 / 3' 501 32 9
  echo showpage
  echo '%%Page: 2 2'
  echo base
  echo '0.13 0.20 0.29 setrgbcolor'
  line 'HOW THE CALENDAR WORKS' 52 708 22
  echo '0.72 0.36 0.22 setrgbcolor'
  line "$diagram" 52 651 12
  echo '0.13 0.20 0.29 setrgbcolor'
  paragraph "$structure" 607 11
  echo sections
  echo '0.13 0.20 0.29 setrgbcolor'
  mapfile -t sections_label < <(jq -r '.sections[:4][]?.name // empty' docs/GUIDE.json)
  for index in "${!sections_label[@]}"; do
    x=$((54 + index*123))
    line "${sections_label[$index]}" "$x" 364 8
  done
  line "$(jq -r '.diagram_caption // empty' docs/GUIDE.json)" 52 454 10
  echo '0.72 0.36 0.22 setrgbcolor'
  line 'A PRACTICAL EXAMPLE' 52 299 13
  echo '0.13 0.20 0.29 setrgbcolor'
  paragraph "$example" 269 11
  echo '0.72 0.36 0.22 setrgbcolor'
  line 'IN INFILTRATOROS' 52 159 11
  echo '0.13 0.20 0.29 setrgbcolor'
  paragraph 'Choose this calendar from available resources. The native module calculates dates and renders its own day, month, year, and complete date output.' 133 10
  echo '0.34 0.37 0.45 setrgbcolor'
  line '2 / 3' 501 32 9
  echo showpage
  echo '%%Page: 3 3'
  echo base
  echo '0.13 0.20 0.29 setrgbcolor'
  line 'HISTORY AND ACCURACY' 52 708 22
  echo '0.72 0.36 0.22 setrgbcolor'
  line 'THE HISTORICAL CONVENTION' 52 650 13
  echo '0.13 0.20 0.29 setrgbcolor'
  paragraph "$history" 616 11
  echo '0.72 0.36 0.22 setrgbcolor'
  line 'IMPORTANT LIMITATIONS' 52 477 13
  echo '0.13 0.20 0.29 setrgbcolor'
  paragraph "$limits" 443 11
  echo '0.94 0.91 0.85 setrgbcolor 46 248 503 117 rectfill'
  echo '0.13 0.20 0.29 setrgbcolor'
  line 'WHAT IS INSTALLED?' 58 335 13
  paragraph 'Exactly two files: the native compiled C++ calendar resource (.so), and this illustrated PDF. No Python, build scripts, source code or engineering documentation are installed.' 304 10
  echo '0.72 0.36 0.22 setrgbcolor'
  line 'REFERENCE' 52 204 11
  echo '0.13 0.20 0.29 setrgbcolor'
  paragraph "$source" 180 9
  echo '0.34 0.37 0.45 setrgbcolor'
  line '3 / 3' 501 32 9
  echo showpage
  echo '%%EOF'
} >> "$ps"
mkdir -p "$(dirname "$pdf")"
ps2pdf -dCompatibilityLevel=1.4 "$ps" "$pdf"
[[ $(head -c5 "$pdf") == '%PDF-' ]] || { echo 'PDF generation failed' >&2;exit 1; }
