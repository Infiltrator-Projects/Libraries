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
[[ -f $source ]] || { echo 'Missing canonical docs/GUIDE.json' >&2; exit 1; }
jq -e '(.title | type == "string" and length > 0) and (.intro | type == "string" and length > 0) and (.limits | type == "string" and length > 0) and (.example | type == "string" and length > 0) and (.history | type == "string" and length > 0) and (((.sources // []) | length > 0) or ((.source // "") | length > 0))' "$source" >/dev/null
name=$(jq -er '.title' "$source")
# Preserve provider-specific data, not the old presentation-only labels. A
# section appears once in the PDF; exact duplicate paragraphs are suppressed.
jq -r '
  def item($h;$v): if ($v|type)=="string" and ($v|length)>0 then {h:$h,v:$v} else empty end;
  def lines($h;$v): if ($v|type)=="array" then $v[] | item($h;.) else empty end;
  [item("Overview";.intro), item("Model at a glance";.summary),
   item("Origin and epoch";.origin), item("Origin and epoch";.epoch),
   item("Structure";.structure), item("Year and leap rule";.year_rule),
   item("Structure";.caption_detail),
   item("Cycle note";.diagram_caption),
   (.steps[]? | item(.heading;.text)),
   (if (.sections|type)=="array" and (.sections|length)>0 then
       item("@grid";"sections")
    elif (.month_names|type)=="array" and (.month_names|length)>0 then
       item("@grid";"month_names")
    else empty end),
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
/celltxt { /size exch def /font exch def /y exch def /x exch def /str exch def
 font findfont size scalefont setfont x y moveto
 /w str stringwidth pop def w 223 gt { gsave x y translate 223 w div 1 scale 0 0 moveto str show grestore } { str show } ifelse } bind def
PS
escape(){ printf '%s' "$1" | iconv -f UTF-8 -t ASCII//TRANSLIT | sed -e 's/\\/\\\\/g' -e 's/(/\\(/g' -e 's/)/\\)/g'; }
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
# Render a real provider-defined sequence, not an invented calendar face.
# The boxes are equal-sized because the values may be dates, lengths or
# cycle labels rather than comparable quantities. Preserve their exact order.
period_grid(){
    local mode=$1 title name value cell col x row_y n=0
    if [[ $mode == sections ]]; then
        title='Period or cycle'
        jq -er '.sections[] | [(.name|strings), (.days|tostring)] | @tsv' "$source" > "$work/grid"
    else
        title='Month sequence'
        jq -er '.month_names | to_entries[] | [(.value|strings), ((.key+1)|tostring)] | @tsv' "$source" > "$work/grid"
    fi
    if ((y<175)); then startpage; fi
    heading "$title"
    row_y=$y
    while IFS=$'\t' read -r name value; do
        col=$((n%2)); x=$((55+col*248))
        if ((col==0)); then
            if ((row_y<106)); then startpage; heading "$title"; row_y=$y; fi
        fi
        printf '0.94 0.95 0.96 setrgbcolor %s %s 237 35 rectfill\n' "$x" "$((row_y-32))" >> "$ps"
        printf '0.13 0.20 0.30 setrgbcolor (%s) %s %s /Helvetica-Bold 10 celltxt\n' "$(escape "$name")" "$((x+7))" "$((row_y-10))" >> "$ps"
        printf '0.27 0.35 0.42 setrgbcolor (%s) %s %s /Helvetica 9 celltxt\n' "$(escape "$value")" "$((x+7))" "$((row_y-24))" >> "$ps"
        n=$((n+1))
        if ((n%2==0)); then row_y=$((row_y-41)); fi
    done < "$work/grid"
    if ((n%2)); then row_y=$((row_y-41)); fi
    y=$((row_y-10)); last=''
}
startpage
while IFS=$'\t' read -r section value; do
    [[ -n $value ]] || continue
    if [[ $section == @grid ]]; then period_grid "$value"; else paragraph "$section" "$value"; fi
done < "$work/sections"
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
cp "$work/guide.pdf" "$output"
