#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Three-page illustrated calendar guide, generated with Ghostscript, no Python.
set -euo pipefail
cd "$(dirname "$0")/../.."
output=${1:?PDF destination required}
name=$(jq -r '.name' SOURCE-MANIFEST.json)
id=$(jq -r '.id' SOURCE-MANIFEST.json)
summary=$(awk 'NR>3 && NF && $0 !~ /^#/ && $0 !~ /^\*\*/ {print; exit}' README.md)
scope=$(sed -n 's/^\*\*Historical scope\/accuracy:\*\* //p' README.md | head -1)
[[ -n $summary && -n $scope ]] || { echo 'Calendar documentation source is missing' >&2;exit 1; }
escape_ps(){ printf %s "$1" | iconv -f UTF-8 -t ASCII//TRANSLIT -c | sed -e 's/\\/\\\\/g' -e 's/(/\\(/g' -e 's/)/\\)/g'; }
drawline(){ printf '(%s) %s %s %s txt\n' "$(escape_ps "$1")" "$2" "$3" "$4"; }
paragraph(){ local text=$1 y=$2 size=$3; while IFS= read -r line;do [[ -z $line ]] && continue;drawline "$line" 55 "$y" "$size";((y-=22));done < <(printf '%s\n' "$text" | fold -s -w 78); }
scratch=$(mktemp -d);trap 'rm -rf "$scratch"' EXIT
ps="$scratch/guide.ps"
cat > "$ps" <<'PS'
%!PS-Adobe-3.0
%%Title: Infiltrator Illustrated Calendar Guide
%%Creator: Infiltrator Projects native calendar tools
%%Pages: 3
%%BoundingBox: 0 0 595 842
%%EndComments
/txt { /size exch def /y exch def /x exch def /str exch def /Helvetica findfont size scalefont setfont x y moveto str show } bind def
/page {
  0.987 0.980 0.966 setrgbcolor 0 0 595 842 rectfill
  0.13 0.20 0.30 setrgbcolor 0 770 595 72 rectfill
  1 1 1 setrgbcolor (INFILTRATOR  /  CALENDARS) 52 793 13 txt
  0.73 0.41 0.21 setrgbcolor 52 770 130 5 rectfill
  0.65 0.65 0.65 setrgbcolor 52 55 490 0.8 rectfill
  0.38 0.44 0.50 setrgbcolor (ILLUSTRATED REFERENCE GUIDE) 52 34 9 txt
} bind def
/grid {
  0.14 0.22 0.32 setrgbcolor
  0 1 6 { /i exch def
    0 1 4 { /j exch def
      /xx 103 i 56 mul add def /yy 270 j 52 mul add def
      0.92 0.91 0.88 setrgbcolor xx yy 48 43 rectfill
      0.14 0.22 0.32 setrgbcolor 1 setlinewidth xx yy 48 43 rectstroke
    } for
  } for
  0.73 0.41 0.21 setrgbcolor 215 374 48 43 rectfill
  1 1 1 setrgbcolor (X) 233 391 15 txt
} bind def
/timeline {
  0.13 0.20 0.30 setrgbcolor 10 setlinewidth 90 407 moveto 510 407 lineto stroke
  0 1 6 {
    /i exch def /x 95 i 68 mul add def
    0.73 0.41 0.21 setrgbcolor x 407 9 0 360 arc fill
    0.13 0.20 0.30 setrgbcolor
  } for
} bind def
PS
{
  echo '%%Page: 1 1'
  echo page
  echo '0.13 0.20 0.30 setrgbcolor'
  drawline 'WHAT IS THIS CALENDAR?' 52 715 23
  echo '0.73 0.41 0.21 setrgbcolor'
  drawline "$name" 52 669 17
  echo '0.13 0.20 0.30 setrgbcolor'
  paragraph "$summary" 632 11
  echo '0.94 0.92 0.88 setrgbcolor 45 481 505 101 rectfill'
  echo '0.13 0.20 0.30 setrgbcolor'
  drawline 'HISTORICAL AND MATHEMATICAL CONTEXT' 60 546 12
  paragraph "$scope" 518 10
  echo grid
  echo '0.40 0.44 0.48 setrgbcolor'
  drawline 'Schematic dates: this diagram is an illustration, not a live calendar.' 55 196 10
  drawline "Calendar model ID: $id" 55 166 10
  drawline '1 / 3' 501 34 9
  echo showpage
  echo '%%Page: 2 2'
  echo page
  echo '0.13 0.20 0.30 setrgbcolor'
  drawline 'HOW IT WORKS' 52 718 23
  drawline 'Different calendars interpret the same day in different ways.' 55 663 11
  echo timeline
  echo '0.73 0.41 0.21 setrgbcolor'
  drawline 'ONE CONTINUOUS SEQUENCE OF DAYS' 130 455 13
  echo '0.13 0.20 0.30 setrgbcolor'
  drawline 'A calendar is a presentation of dates, not a different physical timeline.' 55 344 11
  paragraph "$summary" 302 11
  drawline 'USING IT IN INFILTRATOROS' 55 210 13
  paragraph 'Select the calendar to display a date in its own year, month, day and period scheme. The native calendar engine calculates dates and navigation without changing underlying time.' 183 10
  drawline '2 / 3' 501 34 9
  echo showpage
  echo '%%Page: 3 3'
  echo page
  echo '0.13 0.20 0.30 setrgbcolor'
  drawline 'ACCURACY, LIMITATIONS AND INSTALLATION' 52 712 21
  echo '0.73 0.41 0.21 setrgbcolor'
  drawline 'WHAT THIS MODEL CLAIMS' 55 646 13
  echo '0.13 0.20 0.30 setrgbcolor'
  paragraph "$scope" 615 11
  echo '0.73 0.41 0.21 setrgbcolor'
  drawline 'WHAT THE ENGINE CALCULATES' 55 493 13
  echo '0.13 0.20 0.30 setrgbcolor'
  paragraph "$summary" 465 11
  echo '0.94 0.92 0.88 setrgbcolor 46 263 504 115 rectfill'
  echo '0.13 0.20 0.30 setrgbcolor'
  drawline 'THE INSTALLED FILES' 61 341 13
  paragraph 'Only two files are installed: the compiled native calendar shared library (.so) and this illustrated PDF. The source, engineering tests, build tools and translations remain with the source repository.' 314 10
  drawline 'Infiltrator Projects / Calendar Resources' 55 176 11
  drawline "ID: $id" 55 146 10
  drawline '3 / 3' 501 34 9
  echo showpage
  echo '%%EOF'
} >> "$ps"
mkdir -p "$(dirname "$output")"
ps2pdf -dCompatibilityLevel=1.4 "$ps" "$output"
[[ $(head -c 5 "$output") == '%PDF-' ]] || { echo 'PDF header verification failed' >&2;exit 1; }
