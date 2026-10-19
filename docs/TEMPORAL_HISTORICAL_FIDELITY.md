# Historical fidelity of calendar and clock resources

Status: evidence-based audit in progress. This document is **not** a blanket
certification that every calendar and clock reproduces every historical period.
It records what each installed implementation *actually claims* and what
historical evidence is still required before claiming historical authenticity.

## Rule of evidence

The resource ABI and packaging identify **which implementation is installed**.
They do not certify historicity. A calendar may be:

- **Civil or religious practice:** the dates and transitions actually used in
  a specified place and period.
- **Mathematical convention:** a perfectly consistent rule that may be modern,
  retrospective, proposed, or computational rather than observed.
- **Astronomical reconstruction:** the result of an explicitly specified
  ephemeris/observation rule with a defined precision and reference location.

These are distinct contracts. Do not rewrite a stable ID to change between them.
Do not silently substitute a Gregorian-derived year label for a pre-reform
lunisolar calendar, extrapolate a bounded official table, or present an
approximation as an observational record.

## Established reference anchors now tested

`resources/temporal/tests/test-calendars.c` compares installed provider
output against **independent historical date anchors**, not merely against a
provider's own inverse calculation. Checked anchors include the original
French Republican epoch and year XII, 1 Tishrei 5784, the Maya GMT Long Count
completion, the Swedish 30 February 1712, Indian National Saka New Year,
modern Persian Nowruz, and the 1940-1941 Thai Buddhist civil-year transition.

These are sparse anchors, not exhaustive verification. `test-clocks.c`
currently focuses chiefly on formatting/timing contracts, epoch samples and
internal consistency; independent historical clock observations remain to be
developed.

## Calendar catalogue: 31 separate capabilities

The rows below describe **the installed algorithm family**, not a claim that
its result is official in every era, country, or observance.

| Stable ID | Implemented meaning | Fidelity boundary |
| --- | --- | --- |
| `gregorian` | Gregorian civil arithmetic | Before local adoption is proleptic; country-specific changeover is not modeled |
| `hebrew` | Fixed calculated Hebrew calendar | Fixed rabbinic computation, not every ancient observational practice |
| `persian` | Modern Solar Hijri arithmetic with ICU-compatible leap corrections | Iranian civil month lengths differed before 1304 AP (1925 CE); ancient date projections are not historic civil records |
| `coptic` | Alexandrian/Coptic arithmetic | Era and civil arithmetic, not a general ancient Egyptian calendar |
| `iso-week` | ISO week/year arithmetic | Modern standard; earlier projections are proleptic |
| `julian` | Julian arithmetic | Not the irregular pre-Augustan administration of Julian leap days |
| `revised-julian` | Milanković revised Julian arithmetic | Proposed/adopted modern reform; local ecclesiastical use is distinct |
| `islamic-umalqura` | Bounded Umm al-Qura month table, 1300-1600 AH | A retrospective table outside its original official adoption era; no extrapolation outside table |
| `islamic-civil` | Islamic civil/tabular arithmetic | Not guaranteed to match local crescent sightings |
| `islamic-tbla` | Islamic tabular arithmetic with astronomical epoch | Distinct from civil epoch and observed crescent |
| `islamic` | ICU computational astronomical Islamic calendar | Not guaranteed to reproduce historical local observations |
| `bahai` | Badíʿ dates, fixed old convention and modern Tehran-equinox calculation | Explicit astronomical approximation and documented future-year range |
| `buddhist` | Gregorian-derived year +543 | Modern numerical era projection, not historical Thai April-start civil years or other Buddhist traditions |
| `thai-buddhist-solar` | Thai Buddhist solar civil calendar from 1 April 1912 | Year 2483 B.E. has nine months; rejects earlier unsupported eras and impossible dates |
| `ethiopian` | Ethiopian Amete Mihret arithmetic | Era-specific Ethiopian model |
| `ethiopic-amete-alem` | Ethiopian Amete Alem arithmetic | Separate 5500-year era, not an alias |
| `chinese` | ICU traditional Chinese computational lunisolar model | Retrospective output not certified against each historical imperial astronomy reform/location |
| `dangi` | ICU Korean Dangi computational lunisolar model | Retrospective output not certified against historic Korean local calculation |
| `indian` | Indian National Saka calendar | Adopted modern national calendar; not all historical Indian regional calendars |
| `japanese` | Meiji-to-Reiwa imperial era names over Gregorian dates | **Pre-1 January 1873 Japanese civil dates were lunisolar**, so projection for Meiji 1-5 is not historical civil notation |
| `minguo` | ROC era over Gregorian dates | Does not imply earlier retrospective official usage |
| `roman` | Roman Kalends/Nones/Ides over proleptic Julian dates | Not the original pre-Julian Roman calendar |
| `byzantine` | Byzantine Anno Mundi/Constantinople, Julian Sept-1 year | Era convention, not a literal astronomical creation date |
| `egyptian-nabonassar` | Egyptian 365-day wandering year with Nabonassar era | Ptolemaic-era reckoning; **not** the original pharaonic regnal calendar |
| `armenian-traditional` | Armenian wandering 365-day year from 552 Julian | Deliberately distinct from later Armenian reforms |
| `mayan` | Maya Long Count using GMT correlation 584283 | Alternative scholarly correlations exist; GMT is explicit, not universally certain |
| `french-republican` | Republican calendar epoch, Romme arithmetic continuation | Original year I-XIV anchors match contemporary concordances; extended arithmetic was not adopted historically |
| `swedish-historical` | Actual Swedish exceptional civil dates in 1700-1753 | Models 30 February 1712 and the 1753 Gregorian changeover |
| `international-fixed` | Cotsworth International Fixed proposal | A proposed regular calendar, not a universally adopted civil calendar |
| `world` | World Calendar proposal | Distinct proposal with its own intercalary days |
| `positivist` | Comte's Positivist proposal | Retrospective calendar proposal, not general civil practice |

## Clock catalogue: 23 separate capabilities

| Stable IDs | Implemented meaning | Fidelity boundary |
| --- | --- | --- |
| `standard-12`, `standard-24` | Fixed 12/24-hour conventional display | Uses conventional UTC offset, not a reconstruction of historical clock standards |
| `decimal` | 10h × 100m × 100s civil-day arithmetic | Mathematical French Revolutionary decimal-time projection; no historical Paris mean-time locality in this ID |
| `internet` | Swatch beats at UTC+01:00 | Fixed Biel reference offset; not caller-local time |
| `unix` | Unix seconds from epoch | Standard numerical elapsed-time count |
| `hexadecimal`, `binary` | Mathematical day/clock display | Modern representation, not a claimed historical practice |
| `julian`, `modified-julian` | Astronomical JD/MJD numerical clocks | Distinct astronomical day-number origins, not civil wall clocks |
| `chinese-ke` | 100-ke division of local day | Explicit Han-era-style 100-ke model; not every historical 96-ke or variable system |
| `mean-solar` | Longitude-derived local mean solar time | Neglects UT1-UTC difference and historical civil station corrections |
| `sidereal` | Local mean sidereal time from compact J2000 formula | Documented astronomical approximation, not a high-precision IAU ephemeris |
| `chinese-time` | Twelve Earthly Branch double-hours | Uses offset-based modern local time, not a pre-standardisation historic longitude observation |
| `solar` | Apparent local solar time | NOAA compact solar approximation, not observational ephemeris |
| `italian-hours` | Equal hours from sunset | Computed solar event from a compact modern approximation |
| `babylonian-hours` | Renaissance equal hours from sunrise | Separate from ancient Babylonian bēru divisions |
| `babylonian-ancient` | bēru and UŠ elapsed from sunset | Reconstructed coarse measurement, not a named Babylonian civil-era/location chronology |
| `indian-ghati` | ghaṭī/pala count from sunrise | Modern solar sunrise calculation, not a locality/date-specific historic instrument |
| `roman-temporal` | Twelve seasonal daylight hours and night watches | Reconstructed modern solar model, not direct records of historical daylight observations |
| `japanese-temporal-early`, `japanese-temporal` | Seasonal Edo toki/bell conventions | Different sunrise/twilight definitions; calculated modern astronomical model |
| `nuremberg-hours` | Historic seasonal Great Clock allocation | Replays documented 1488 Julian Wendetage in a modern annual projection |
| `nuremberg-solar` | Nuremberg-style location-dependent solar model | Modernized solar reconstruction, explicitly separate from fixed historical schedule |

## Confirmed mismatch and implemented correction

The Thai Buddhist Era is **not** simply Gregorian year +543 in all of its
documented Thai civil usage. Prior to 1941 the Thai year began 1 April.
The 1940-1941 reform yielded the unusually short year 2483 B.E., starting
1 April 1940 and ending 31 December 1940.

The existing `buddhist` provider remains untouched and continues to mean its
explicit Gregorian-derived +543 convention. The separate
`thai-buddhist-solar` provider implements the attested historic transition
from 1 April 1912 onward. It rejects years prior to its documented model,
and rejects impossible year/month combinations; no historical algorithm is
fabricated for the pre-1912 system.

## Further confirmed limitations (not yet fixed)

1. **Persian prior to 1925:** earlier official month lengths differed from
   the fixed modern sequence. Accurately supporting them requires a separate
   historical chronology and documentary evidence, rather than extending the
   modern formula silently.
2. **Japanese prior to 1873:** Meiji 1-5 requires the Japanese lunisolar civil
   rules, not the modern Gregorian projection.
3. **China/Korea and observed Islamic dates:** region, observation, calendar
   reform and astronomical locality can change the historical date. A modern
   ICU-derived calculation is not automatically proof of historic observance.
4. **Historical clock models:** solar-based and sidereal implementations need
   independently established astronomical/chronometric vectors and documented
   bounds on approximation error before exact historical claims.
5. **Calendar-region adoption:** Gregorian and some other systems do not have
   one global legal cutover date. Projected dates and actual jurisdictional
   date-label transitions must not be conflated.

## Primary references

- Japan, National Diet Library, *Calendar History*, Gregorian reform:
  https://www.ndl.go.jp/koyomi/e/history/calendar.html
- Thailand, Parliamentary Museum, history of the 2483 B.E. Calendar Act:
  https://parliamentmuseum.go.th/ar63-Calendar.html
- Iranian Persian calendar, FarsiWeb, historic month-length and year range:
  https://persian-computing.org/archives/Sharif-FarsiWeb-Inc/FAQ.html
- ICU PersianCalendar technical specification:
  https://unicode-org.github.io/icu-docs/apidoc/dev/icu4j/com/ibm/icu/util/PersianCalendar.html
- French Republican calendrical concordance, Archives of Lyon:
  https://www.fondsenligne.archives-lyon.fr/ac69v2/calendrier.php
- Hebrew 1 Tishrei 5784, Hebcal:
  https://www.hebcal.com/holidays/rosh-hashana-2023
- Maya GMT correlation and alternatives, FAMSI:
  https://www.famsi.org/research/vanstone/2012/faq.html

These source citations establish specific boundaries and control examples.
Remaining rows reflect repository source inspection rather than independently
proven exhaustive historical accuracy. No additional calendar may be called
"historically verified" until its entire claimed period and variants have
adequate evidence and regression vectors.
