#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Native PostScript/PDF generator: ordinary guide, no Python or runtime dependencies.
set -euo pipefail
cd "$(dirname "$0")/../.."
output=${1:?Output PDF is required}
command -v ps2pdf >/dev/null || { echo 'Missing ghostscript/ps2pdf build prerequisite' >&2; exit 1; }
id=$(jq -r '.id' SOURCE-MANIFEST.json)
name=$(jq -r '.name' SOURCE-MANIFEST.json)
summary=$(awk 'NR>3 && NF && $0 !~ /^#/ && $0 !~ /^\*\*/ {print; exit}' README.md)
scope=$(sed -n 's/^\*\*Historical scope\/accuracy:\*\* //p' README.md | head -1)
[[ -n $summary && -n $scope ]] || { echo 'Cannot generate a genuine user guide without model/accuracy explanation' >&2; exit 1; }
ps_escape() { printf '%s' "$1" | iconv -f UTF-8 -t ASCII//TRANSLIT -c | sed -e 's/\\/\\\\/g' -e 's/(/\\(/g' -e 's/)/\\)/g'; }
line() { printf '(%s) %s %s %s txt\n' "$(ps_escape "$1")" "$2" "$3" "$4"; }
paragraph() {
    local line_text=$1 y=$2 size=$3
    while IFS= read -r fragment; do
        [[ -z $fragment ]] && continue
        line "$fragment" 58 "$y" "$size"
        ((y-=22))
    done < <(printf '%s\n' "$line_text" | fold -s -w 76)
}
work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
ps="$work/guide.ps"
cat > "$ps" <<'PS'
%!PS-Adobe-3.0
%%Title: Infiltrator Clock Illustrated Guide
%%Creator: Infiltrator Projects Native Documentation Builder
%%Pages: 3
%%BoundingBox: 0 0 595 842
%%EndComments
/Times-Roman findfont 12 scalefont setfont
/txt { /size exch def /y exch def /x exch def /str exch def /Helvetica findfont size scalefont setfont x y moveto str show } bind def
/header {
  0.985 0.98 0.97 setrgbcolor 0 0 595 842 rectfill
  0.13 0.20 0.30 setrgbcolor 0 770 595 72 rectfill
  1 1 1 setrgbcolor (INFILTRATOR    /    CLOCK RESOURCES) 56 794 13 txt
  0.71 0.39 0.22 setrgbcolor 56 770 110 5 rectfill
  0.75 0.74 0.71 setrgbcolor 56 54 484 0.7 rectfill
  0.37 0.42 0.48 setrgbcolor (ILLUSTRATED USER GUIDE) 56 33 9 txt
} bind def
/ring {
  0.71 0.39 0.22 setrgbcolor 7 setlinewidth 297 360 97 0 360 arc stroke
  0.13 0.20 0.30 setrgbcolor 2 setlinewidth
  0 1 11 { /i exch def /a 90 i 30 mul sub def
    297 a cos 81 mul add 360 a sin 81 mul add moveto
    297 a cos 91 mul add 360 a sin 91 mul add lineto stroke
  } for
  5 setlinewidth 297 360 moveto 334 413 lineto stroke
  3 setlinewidth 297 360 moveto 234 338 lineto stroke
  0.71 0.39 0.22 setrgbcolor 297 360 7 0 360 arc fill
} bind def
/band {
  0.13 0.20 0.30 setrgbcolor 90 340 415 92 rectfill
  0.71 0.39 0.22 setrgbcolor 90 340 104 92 rectfill
  0.95 0.92 0.87 setrgbcolor 1 1 1 setrgbcolor
} bind def
PS
{
    echo '%%Page: 1 1'
    echo 'header'
    echo '0.13 0.20 0.30 setrgbcolor'
    line 'WHAT IS THIS CLOCK?' 56 718 22
    echo '0.71 0.39 0.22 setrgbcolor'
    line "$name" 56 665 18
    echo '0.13 0.20 0.30 setrgbcolor'
    paragraph "$summary" 622 11
    echo '0.94 0.91 0.86 setrgbcolor 47 453 501 104 rectfill'
    echo '0.13 0.20 0.30 setrgbcolor'
    line 'THE IDEA' 62 527 14
    paragraph "$scope" 501 10
    echo 'ring'
    echo '0.37 0.42 0.48 setrgbcolor'
    line 'Diagram: a reference time face, not a live reading.' 56 207 11
    echo '0.71 0.39 0.22 setrgbcolor'
    line "MODEL ID: $id" 56 158 11
    echo '0.37 0.42 0.48 setrgbcolor'
    line '1 / 3' 498 33 9
    echo 'showpage'
    echo '%%Page: 2 2'
    echo 'header'
    echo '0.13 0.20 0.30 setrgbcolor'
    line 'HOW TO READ AND USE IT' 56 718 22
    line "$name" 56 661 17
    echo '0.37 0.42 0.48 setrgbcolor'
    paragraph "$summary" 620 11
    echo 'band'
    echo '1 1 1 setrgbcolor'
    line 'TIME REFERENCE' 111 389 16
    line 'MODEL-DEFINED DIVISIONS' 218 354 10
    echo '0.13 0.20 0.30 setrgbcolor'
    line 'IN INFILTRATOROS' 56 299 13
    paragraph 'Select this clock from the available clock resources. The clock module renders its own notation and chooses the next visible update interval.' 266 11
    paragraph 'Seconds and vertical presentation are offered where the clock supports them. The module produces the reading; the diagram on these pages is explanatory.' 194 10
    echo '0.37 0.42 0.48 setrgbcolor'
    line '2 / 3' 498 33 9
    echo 'showpage'
    echo '%%Page: 3 3'
    echo 'header'
    echo '0.13 0.20 0.30 setrgbcolor'
    line 'HISTORY, ACCURACY AND SCOPE' 56 718 22
    echo '0.71 0.39 0.22 setrgbcolor'
    line 'HISTORICAL OR MATHEMATICAL BOUNDARY' 56 653 13
    echo '0.13 0.20 0.30 setrgbcolor'
    paragraph "$scope" 620 11
    echo '0.71 0.39 0.22 setrgbcolor'
    line 'IMPLEMENTED MODEL' 56 520 13
    echo '0.13 0.20 0.30 setrgbcolor'
    paragraph "$summary" 487 11
    echo '0.94 0.91 0.86 setrgbcolor 47 268 501 133 rectfill'
    echo '0.13 0.20 0.30 setrgbcolor'
    line 'WHAT IS INSTALLED?' 62 363 13
    paragraph 'The native compiled clock resource (.so) and this explanatory PDF are the only installed clock files. Source, build tools, tests and technical notes stay in the repository.' 332 10
    echo '0.37 0.42 0.48 setrgbcolor'
    line 'Infiltrator Projects / Individual Clock Resource' 56 204 11
    line "Identifier: $id" 56 177 10
    line '3 / 3' 498 33 9
    echo 'showpage'
    echo '%%EOF'
} >> "$ps"
mkdir -p "$(dirname "$output")"
ps2pdf -dCompatibilityLevel=1.4 "$ps" "$output"
[[ $(head -c 5 "$output") == '%PDF-' ]] || { echo 'PDF failed validation' >&2; exit 1; }
