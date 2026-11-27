#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Render one installed temporal manual from its provider-owned documentation.
set -euo pipefail
kind=${1:?Resource kind required}
output=${2:?PDF destination required}
root=${3:?Provider root required}
case "$kind" in clock|calendar) ;; *) exit 2;; esac
for cmd in jq iconv fold ps2pdf; do command -v "$cmd" >/dev/null || { echo "Missing documentation prerequisite: $cmd" >&2; exit 1; }; done
cd "$root"
id=$(jq -er '.id | strings | select(length>0)' SOURCE-MANIFEST.json)
name=$(jq -er '.name | strings | select(length>0)' SOURCE-MANIFEST.json)
work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
source=docs/GUIDE.json
if [[ ! -f $source ]]; then
    # Older clocks have not yet migrated their prose to GUIDE.json. Retain the
    # existing verified model and limits once; do not fabricate a clock face.
    summary=$(awk 'NR>3 && NF && $0 !~ /^#/ && $0 !~ /^\*\*/ {print; exit}' README.md)
    scope=$(sed -n 's/^\*\*Historical scope\/accuracy:\*\* //p' README.md | head -1)
    [[ -n $summary && -n $scope ]] || { echo 'Missing provider model or scope' >&2; exit 1; }
    source="$work/legacy.json"
    jq -n --arg title "$name" --arg intro "$summary" --arg limits "$scope" '{title:$title,intro:$intro,limits:$limits}' > "$source"
fi
jq -e '(.intro | type == "string" and length > 0) and (.limits | type == "string" and length > 0)' "$source" >/dev/null
# Preserve provider-specific data, not the old presentation-only labels. A
# section appears once in the PDF; exact duplicate paragraphs are suppressed.
jq -r '
  def item($h;$v): if ($v|type)=="string" and ($v|length)>0 then {h:$h,v:$v} else empty end;
  def lines($h;$v): if ($v|type)=="array" then $v[] | item($h;.) else empty end;
  [item("Overview";.intro), item("Model at a glance";.summary),
   item("Origin and epoch";.origin), item("Origin and epoch";.epoch),
   item("Structure";.structure), item("Year and leap rule";.year_rule),
   item("Structure";.caption_detail),
   (.steps[]? | item(.heading;.text)),
   (if (.sections|type)=="array" then
       .sections[] | item("Period or cycle"; (.name + "  |  " + (.days|tostring)))
    else empty end),
   (if (.month_names|type)=="array" then item("Month sequence";(.month_names|join(", "))) else empty end),
   item("Additional days";.extra_days), item("Calendar rules";.pattern_note),
   item("Additional details";.after_note), item("Worked example";.example),
   item("History";.history), item("Implemented model";.model), item("Limits";.limits),
   (if (.source|type)=="string" then .source|split("\n")[]|item("Sources";.) else empty end),
   lines("Sources";.sources)]
  | reduce .[] as $x ([]; if any(.[]; .v == $x.v) then . else . + [$x] end)
  | .[] | [.h, (.v|gsub("[\t\r\n]+";" "))] | @tsv' "$source" > "$work/sections"
ps="$work/guide.ps"
cat > "$ps" <<'PS'
%!PS-Adobe-3.0
%%Title: InfiltratorOS resource manual
%%Pages: (atend)
%%BoundingBox: 0 0 595 842
%%EndComments
<< /PageSize [595 842] >> setpagedevice
/txt { /size exch def /font exch def /y exch def /x exch def /str exch def
 font findfont size scalefont setfont x y moveto
 /w str stringwidth pop def w 485 gt { gsave x y translate 485 w div 1 scale 0 0 moveto str show grestore } { str show } ifelse } bind def
PS
escape(){ printf '%s' "$1" | iconv -f UTF-8 -t ASCII//TRANSLIT -c | sed -e 's/\\/\\\\/g' -e 's/(/\\(/g' -e 's/)/\\)/g'; }
line(){ printf '(%s) 55 %s /%s %s txt\n' "$(escape "$1")" "$2" "$3" "$4" >> "$ps"; }
p=0; y=0; last=''
endpage(){
    printf '0.70 0.72 0.74 setrgbcolor 55 53 485 0.7 rectfill\n0.27 0.35 0.42 setrgbcolor\n' >> "$ps"
    line "Resource: $id  |  Page $p" 33 Helvetica 9
    printf 'showpage\n' >> "$ps"
}
startpage(){
    if ((p>0)); then endpage; fi
    p=$((p+1)); y=730
    printf '%%%%Page: %s %s\n0.13 0.20 0.30 setrgbcolor 0 774 595 68 rectfill\n1 1 1 setrgbcolor\n' "$p" "$p" >> "$ps"
    line "INFILTRATOROS / ${kind^^} RESOURCES" 812 Helvetica-Bold 10
    line "$name" 788 Helvetica-Bold 19
    printf '0.73 0.41 0.21 setrgbcolor 55 774 108 4 rectfill\n' >> "$ps"
    last=''
}
heading(){
    if ((y<108)); then startpage; fi
    if [[ $last != "$1" ]]; then
        printf '0.13 0.20 0.30 setrgbcolor\n' >> "$ps"
        line "$1" "$y" Helvetica-Bold 14
        y=$((y-23)); last=$1
    fi
}
paragraph(){
    local heading_text=$1 value=$2 fragment
    heading "$heading_text"
    while IFS= read -r fragment; do
        if ((y<78)); then startpage; heading "$heading_text"; fi
        printf '0.12 0.17 0.22 setrgbcolor\n' >> "$ps"
        line "$fragment" "$y" Helvetica 11
        y=$((y-15))
    done < <(printf '%s\n' "$value" | fold -s -w 76)
    y=$((y-8))
}
startpage
while IFS=$'\t' read -r section value; do [[ -n $value ]] && paragraph "$section" "$value"; done < "$work/sections"
if [[ $kind == calendar ]]; then
    paragraph 'Using this resource' 'Select this calendar in a compatible InfiltratorOS application. It changes the displayed calendar and date navigation, not the underlying instant or stored event order.'
else
    paragraph 'Using this resource' 'Select this clock in a compatible InfiltratorOS application. The provider controls its notation and visible update interval. Seconds and alternate presentation are available only where supported.'
fi
paragraph 'Installed component' 'The package owns this manual and its native shared-object resource. Removing the package removes its own files; other installed resources are unaffected.'
endpage
printf '%%%%Trailer\n%%%%Pages: %s\n%%%%EOF\n' "$p" >> "$ps"
mkdir -p "$(dirname "$output")"
ps2pdf -dCompatibilityLevel=1.4 "$ps" "$output"
[[ $(head -c 5 "$output") == '%PDF-' ]] || { echo 'PDF header verification failed' >&2; exit 1; }
