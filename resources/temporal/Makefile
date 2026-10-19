# SPDX-License-Identifier: GPL-3.0-or-later
# Common owns these independent system modules; consumers never build them.
.DEFAULT_GOAL := all
CC ?= cc
AR ?= ar
PKG_CONFIG ?= pkg-config
BUILD_DIR ?= build
COMMON_ROOT := ../..
VERSION := $(strip $(file <../../VERSION))
INFILTRATR_COMMON_ARCHIVE := $(BUILD_DIR)/common/libinfiltratr-common.a
INFILTRATR_COMMON_HEADERS := $(wildcard ../../include/infiltratr/*.h)
CPPFLAGS += -Isrc/support -Isrc/calendars -Isrc/calendars/engine -I../../include \
    -DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_74 -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_74 \
    -DGETTEXT_PACKAGE=\"infiltrator-temporal-resources\" -DU_DISABLE_RENAMING=1
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -fPIC -Wall -Wextra -Wpedantic -Werror -Wformat=2 \
    -Wshadow -Wstrict-prototypes -Wmissing-prototypes -fstack-protector-strong -fno-common \
    -ffile-prefix-map=$(CURDIR)=. -fdebug-prefix-map=$(CURDIR)=.
CORE_CFLAGS := $(shell $(PKG_CONFIG) --cflags glib-2.0 icu-i18n)
CORE_LIBS := $(shell $(PKG_CONFIG) --libs glib-2.0)
ICU_BRIDGE_LIBS := -ldl -pthread
MATH_LIBS := -lm
DYNLIB_LIBS := -ldl
REPRO_SEED_PREFIX := common-temporal-resources
HEADERS := $(wildcard src/support/*.h)
RESOURCE_ENGINE_SOURCES := \
	src/calendars/engine/time-astronomy.c \
	src/calendars/engine/calendar-arithmetic-indian.c \
	src/calendars/engine/calendar-perpetual.c \
	src/calendars/engine/icu-calendar.c \
	src/calendars/engine/icu-compat-bridge.c \
	src/calendars/engine/calendar-arithmetic.c \
	src/calendars/engine/calendar-ancient.c \
	src/calendars/engine/calendar-arithmetic-japanese.c \
	src/calendars/engine/calendar-custom.c \
	src/calendars/engine/calendar-arithmetic-coptic.c \
	src/calendars/engine/calendar-historical.c \
	src/calendars/engine/calendar-arithmetic-islamic.c \
	src/calendars/engine/calendar-helpers.c \
	src/calendars/engine/calendar-swedish.c \
	src/calendars/engine/calendar-bahai.c \
	src/calendars/engine/calendar-reform.c \
	src/calendars/engine/julian.c
RESOURCE_ENGINE_OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(RESOURCE_ENGINE_SOURCES))
RESOURCE_ENGINE_ARCHIVE := $(BUILD_DIR)/libcalendar-resource-engine.a
RESOURCE_ENGINE_HEADERS := $(wildcard src/calendars/engine/*.h)
CALENDAR_RESOURCE_BUILD_DIR := $(BUILD_DIR)/calendar-resources
CALENDAR_RESOURCE_MAP := src/calendars/calendar-resource.map
EXTRACTED_CALENDAR_RESOURCE_NAMES := iso-week julian revised-julian islamic-umalqura islamic-civil islamic-tbla islamic bahai buddhist ethiopian ethiopic-amete-alem chinese dangi indian japanese minguo roman byzantine egyptian-nabonassar armenian-traditional mayan french-republican swedish-historical international-fixed world positivist
CALENDAR_RESOURCE_NAMES := hebrew gregorian persian coptic $(EXTRACTED_CALENDAR_RESOURCE_NAMES)
HEBREW_RESOURCE_SOURCE := src/calendars/hebrew.c
GREGORIAN_RESOURCE_SOURCE := src/calendars/gregorian.c
PERSIAN_RESOURCE_SOURCE := src/calendars/persian.c
PERSIAN_RESOURCE_ARITHMETIC := src/calendars/arithmetic-persian.c
COPTIC_RESOURCE_SOURCE := src/calendars/coptic.c
CALENDAR_RESOURCE_SOURCES := $(HEBREW_RESOURCE_SOURCE) \
	$(GREGORIAN_RESOURCE_SOURCE) \
	$(PERSIAN_RESOURCE_SOURCE) \
	$(PERSIAN_RESOURCE_ARITHMETIC) \
	$(COPTIC_RESOURCE_SOURCE)
empty :=
space := $(empty) $(empty)
escape_resource_path = $(subst $(space),\$(space),$(1))
calendar_name_gregorian := Gregorian Calendar
calendar_name_julian := Julian Calendar
calendar_name_revised-julian := Revised Julian Calendar
calendar_name_iso-week := ISO 8601 Week Calendar
calendar_name_hebrew := Hebrew Calendar
calendar_name_islamic := Islamic Computational Calendar
calendar_name_islamic-civil := Islamic Civil Calendar
calendar_name_islamic-tbla := Islamic Tabular Calendar Astronomical Epoch
calendar_name_islamic-umalqura := Islamic Umm al-Qura Calendar
calendar_name_persian := Persian Solar Hijri Calendar
calendar_name_chinese := Traditional Chinese Lunisolar Calendar
calendar_name_dangi := Korean Dangi Calendar
calendar_name_indian := Indian National Saka Calendar
calendar_name_coptic := Coptic Calendar
calendar_name_ethiopian := Ethiopian Amete Mihret Calendar
calendar_name_ethiopic-amete-alem := Ethiopian Amete Alem Calendar
calendar_name_buddhist := Buddhist Era Calendar
calendar_name_japanese := Japanese Imperial Era Calendar
calendar_name_minguo := Republic of China Minguo Calendar
calendar_name_french-republican := French Republican Calendar
calendar_name_roman := Ancient Roman Calendar Notation
calendar_name_mayan := Maya Long Count Calendar
calendar_name_bahai := Bahai Badi Calendar
calendar_name_international-fixed := International Fixed Calendar
calendar_name_world := World Calendar
calendar_name_positivist := Positivist Calendar
calendar_name_byzantine := Byzantine Anno Mundi Calendar
calendar_name_egyptian-nabonassar := Egyptian Nabonassar Era Calendar
calendar_name_armenian-traditional := Traditional Armenian Calendar
calendar_name_swedish-historical := Swedish Historical Calendar
calendar_variant_iso-week := ISO_WEEK
calendar_variant_julian := JULIAN
calendar_variant_revised-julian := REVISED_JULIAN
calendar_variant_islamic-umalqura := ISLAMIC_UMM_AL_QURA
calendar_variant_islamic-civil := ISLAMIC_CIVIL
calendar_variant_islamic-tbla := ISLAMIC_TBLA
calendar_variant_islamic := ISLAMIC
calendar_variant_bahai := BAHAI
calendar_variant_buddhist := BUDDHIST
calendar_variant_ethiopian := ETHIOPIAN
calendar_variant_ethiopic-amete-alem := ETHIOPIC_AMETE_ALEM
calendar_variant_chinese := CHINESE
calendar_variant_dangi := DANGI
calendar_variant_indian := INDIAN
calendar_variant_japanese := JAPANESE
calendar_variant_minguo := MINGUO
calendar_variant_roman := ROMAN
calendar_variant_byzantine := BYZANTINE
calendar_variant_egyptian-nabonassar := EGYPTIAN_NABONASSAR
calendar_variant_armenian-traditional := ARMENIAN_TRADITIONAL
calendar_variant_mayan := MAYAN
calendar_variant_french-republican := FRENCH_REPUBLICAN
calendar_variant_swedish-historical := SWEDISH_HISTORICAL
calendar_variant_international-fixed := INTERNATIONAL_FIXED
calendar_variant_world := WORLD
calendar_variant_positivist := POSITIVIST
calendar_path = $(CALENDAR_RESOURCE_BUILD_DIR)/$(calendar_name_$(1)).so
calendar_target = $(call escape_resource_path,$(call calendar_path,$(1)))
CALENDAR_RESOURCES := $(foreach id,$(CALENDAR_RESOURCE_NAMES),$(call calendar_target,$(id)))
CLOCK_RESOURCE_BUILD_DIR := $(BUILD_DIR)/clock-resources
CLOCK_RESOURCE_SOURCE := src/clocks/fixed-clock.c
JULIAN_CLOCK_RESOURCE_SOURCE := src/clocks/julian-clock.c
CONVENTIONAL_CLOCK_RESOURCE_SOURCE := src/clocks/conventional-clock.c
CHINESE_KE_CLOCK_SOURCE := src/clocks/chinese-ke-clock.c
MEAN_SOLAR_CLOCK_SOURCE := src/clocks/mean-solar-clock.c
SIDEREAL_CLOCK_SOURCE := src/clocks/sidereal-clock.c
CHINESE_TIME_CLOCK_SOURCE := src/clocks/chinese-time-clock.c
SOLAR_CLOCK_SOURCE := src/clocks/solar-clock.c
SOLAR_ORIGIN_CLOCK_SOURCE := src/clocks/solar-origin-clock.c
SEASONAL_CLOCK_SOURCE := src/clocks/seasonal-clock.c
SEASONAL_CLOCK_RESOURCE_NAMES := roman-temporal japanese-temporal japanese-temporal-early nuremberg-hours nuremberg-solar
CLOCK_RESOURCE_MAP := src/clocks/clock-resource.map
FIXED_CLOCK_RESOURCE_NAMES := decimal internet unix hexadecimal binary
JULIAN_CLOCK_RESOURCE_NAMES := julian modified-julian
CONVENTIONAL_CLOCK_RESOURCE_NAMES := standard-12 standard-24
NEXT_CLOCK_RESOURCE_NAMES := chinese-ke mean-solar
ORIGIN_CLOCK_RESOURCE_NAMES := italian-hours babylonian-hours babylonian-ancient indian-ghati
CLOCK_RESOURCE_NAMES := $(FIXED_CLOCK_RESOURCE_NAMES) $(JULIAN_CLOCK_RESOURCE_NAMES) $(CONVENTIONAL_CLOCK_RESOURCE_NAMES) $(NEXT_CLOCK_RESOURCE_NAMES) $(ORIGIN_CLOCK_RESOURCE_NAMES) sidereal chinese-time solar $(SEASONAL_CLOCK_RESOURCE_NAMES)
clock_name_standard-12 := Standard 12-Hour Clock
clock_name_standard-24 := Standard 24-Hour Clock
clock_name_decimal := French Revolutionary Decimal Clock
clock_name_internet := Swatch Internet Time Clock
clock_name_unix := Unix Epoch Clock
clock_name_hexadecimal := Hexadecimal Day Clock
clock_name_binary := Binary Digital Clock
clock_name_sidereal := Local Sidereal Clock
clock_name_solar := Apparent Solar Clock
clock_name_julian := Astronomical Julian Date Clock
clock_name_mean-solar := Local Mean Solar Clock
clock_name_modified-julian := Modified Julian Date Clock
clock_name_chinese-time := Traditional Chinese Double-Hour Clock
clock_name_chinese-ke := Traditional Chinese Hundred-Ke Clock
clock_name_roman-temporal := Ancient Roman Temporal Clock
clock_name_japanese-temporal := Late Edo Japanese Temporal Clock
clock_name_japanese-temporal-early := Early Edo Japanese Temporal Clock
clock_name_italian-hours := Historical Italian Hours Clock
clock_name_babylonian-hours := Renaissance Babylonian Hours Clock
clock_name_babylonian-ancient := Ancient Babylonian Beru US Clock
clock_name_indian-ghati := Traditional Indian Ghati Clock
clock_name_nuremberg-hours := Historical Nuremberg Great Clock
clock_name_nuremberg-solar := Nuremberg Style Solar Clock
clock_path = $(CLOCK_RESOURCE_BUILD_DIR)/$(clock_name_$(1)).so
clock_target = $(call escape_resource_path,$(call clock_path,$(1)))
CLOCK_RESOURCES := $(foreach id,$(CLOCK_RESOURCE_NAMES),$(call clock_target,$(id)))
$(RESOURCE_ENGINE_ARCHIVE): $(RESOURCE_ENGINE_OBJECTS)
	@rm -f "$@"
	$(AR) rcsD "$@" $(RESOURCE_ENGINE_OBJECTS)


# Each compiler invocation produces its canonical named .so directly; no
# post-build rename, alias or application-specific display substitution.
calendar_native_sources_hebrew := src/calendars/hebrew.c src/calendars/engine/icu-compat-bridge.c
calendar_native_sources_gregorian := src/calendars/gregorian.c src/support/julian-day.c src/calendars/engine/icu-calendar.c src/calendars/engine/icu-compat-bridge.c
calendar_native_sources_persian := src/calendars/persian.c src/calendars/arithmetic-persian.c src/support/julian-day.c src/calendars/engine/icu-calendar.c src/calendars/engine/icu-compat-bridge.c
calendar_native_sources_coptic := src/calendars/coptic.c src/calendars/engine/calendar-arithmetic-coptic.c src/support/julian-day.c src/calendars/engine/icu-calendar.c src/calendars/engine/icu-compat-bridge.c

define build_native_calendar
$(call calendar_target,$(1)): $(calendar_native_sources_$(1)) $(CALENDAR_RESOURCE_MAP) $(INFILTRATR_COMMON_HEADERS) $(INFILTRATR_COMMON_ARCHIVE)
	@mkdir -p "$(CALENDAR_RESOURCE_BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden $(CORE_CFLAGS) \
		-frandom-seed=$(REPRO_SEED_PREFIX)-$(1)-resource \
		-DINFILTRATR_TEMPORAL_DISPLAY_NAME='"$(calendar_name_$(1))"' \
		-shared $(calendar_native_sources_$(1)) $(INFILTRATR_COMMON_ARCHIVE) \
		-Wl,--version-script=$(CALENDAR_RESOURCE_MAP) -o "$$@" \
		$(LDFLAGS) $(CORE_LIBS) $(ICU_BRIDGE_LIBS) $(MATH_LIBS)
endef
$(foreach id,hebrew gregorian persian coptic,$(eval $(call build_native_calendar,$(id))))

define build_extracted_calendar
$(call calendar_target,$(1)): src/calendars/native-calendar.c src/calendars/catalogue.h $(RESOURCE_ENGINE_ARCHIVE) src/support/julian-day.c $(INFILTRATR_COMMON_ARCHIVE) $(CALENDAR_RESOURCE_MAP)
	@mkdir -p "$(CALENDAR_RESOURCE_BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden -DRESOURCE_$(calendar_variant_$(1)) $(CORE_CFLAGS) \
		-DINFILTRATR_TEMPORAL_DISPLAY_NAME='"$(calendar_name_$(1))"' \
		-shared src/calendars/native-calendar.c src/support/julian-day.c \
		$(RESOURCE_ENGINE_ARCHIVE) $(INFILTRATR_COMMON_ARCHIVE) \
		-Wl,--version-script=$(CALENDAR_RESOURCE_MAP) -Wl,-z,defs \
		-o "$$@" $(LDFLAGS) $(CORE_LIBS) $(ICU_BRIDGE_LIBS) $(MATH_LIBS)
endef
$(foreach id,$(EXTRACTED_CALENDAR_RESOURCE_NAMES),$(eval $(call build_extracted_calendar,$(id))))

clock_kind_decimal := 1
clock_kind_internet := 2
clock_kind_unix := 3
clock_kind_hexadecimal := 4
clock_kind_binary := 5
clock_seconds_decimal := true
clock_seconds_internet := true
clock_seconds_unix := false
clock_seconds_hexadecimal := false
clock_seconds_binary := true

define build_fixed_clock
$(call clock_target,$(1)): $(CLOCK_RESOURCE_SOURCE) $(CLOCK_RESOURCE_MAP) $(INFILTRATR_COMMON_HEADERS)
	@mkdir -p "$(CLOCK_RESOURCE_BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden \
		-frandom-seed=$(REPRO_SEED_PREFIX)-$(1)-clock-resource \
		-DCLOCK_RESOURCE_KIND=$(clock_kind_$(1)) -DCLOCK_RESOURCE_ID='"$(1)"' \
		-DINFILTRATR_TEMPORAL_DISPLAY_NAME='"$(clock_name_$(1))"' \
		-DCLOCK_RESOURCE_SECONDS=$(clock_seconds_$(1)) \
		-shared $(CLOCK_RESOURCE_SOURCE) \
		-Wl,--version-script=$(CLOCK_RESOURCE_MAP) -o "$$@" $(LDFLAGS)
endef
$(foreach id,$(FIXED_CLOCK_RESOURCE_NAMES),$(eval $(call build_fixed_clock,$(id))))

define build_julian_clock
$(call clock_target,$(1)): $(JULIAN_CLOCK_RESOURCE_SOURCE) $(CLOCK_RESOURCE_MAP) $(INFILTRATR_COMMON_HEADERS)
	@mkdir -p "$(CLOCK_RESOURCE_BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden \
		-frandom-seed=$(REPRO_SEED_PREFIX)-$(1)-clock-resource \
		-DJULIAN_CLOCK_KIND=$(if $(filter julian,$(1)),1,2) \
		-DINFILTRATR_TEMPORAL_DISPLAY_NAME='"$(clock_name_$(1))"' \
		-shared $(JULIAN_CLOCK_RESOURCE_SOURCE) \
		-Wl,--version-script=$(CLOCK_RESOURCE_MAP) -o "$$@" $(LDFLAGS)
endef
$(foreach id,$(JULIAN_CLOCK_RESOURCE_NAMES),$(eval $(call build_julian_clock,$(id))))

define build_conventional_clock
$(call clock_target,$(1)): $(CONVENTIONAL_CLOCK_RESOURCE_SOURCE) $(CLOCK_RESOURCE_MAP) $(INFILTRATR_COMMON_HEADERS)
	@mkdir -p "$(CLOCK_RESOURCE_BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden \
		-frandom-seed=$(REPRO_SEED_PREFIX)-$(1)-clock-resource \
		-DCONVENTIONAL_CLOCK_KIND=$(if $(filter standard-12,$(1)),12,24) \
		-DINFILTRATR_TEMPORAL_DISPLAY_NAME='"$(clock_name_$(1))"' \
		-shared $(CONVENTIONAL_CLOCK_RESOURCE_SOURCE) \
		-Wl,--version-script=$(CLOCK_RESOURCE_MAP) -o "$$@" $(LDFLAGS)
endef
$(foreach id,$(CONVENTIONAL_CLOCK_RESOURCE_NAMES),$(eval $(call build_conventional_clock,$(id))))

define build_solar_origin_clock
$(call clock_target,$(1)): $(SOLAR_ORIGIN_CLOCK_SOURCE) $(CLOCK_RESOURCE_MAP) $(INFILTRATR_COMMON_HEADERS)
	@mkdir -p "$(CLOCK_RESOURCE_BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden \
		-frandom-seed=$(REPRO_SEED_PREFIX)-$(1)-solar-origin-clock \
		$(if $(filter babylonian-hours,$(1)),-DCLOCK_SUNRISE=1,) \
		$(if $(filter babylonian-ancient,$(1)),-DCLOCK_ANCIENT_BABYLONIAN=1,) \
		$(if $(filter indian-ghati,$(1)),-DCLOCK_GHATI=1,) \
		-DINFILTRATR_TEMPORAL_DISPLAY_NAME='"$(clock_name_$(1))"' \
		-shared $(SOLAR_ORIGIN_CLOCK_SOURCE) \
		-Wl,--version-script=$(CLOCK_RESOURCE_MAP) -o "$$@" $(LDFLAGS) $(MATH_LIBS)
endef
$(foreach id,$(ORIGIN_CLOCK_RESOURCE_NAMES),$(eval $(call build_solar_origin_clock,$(id))))

clock_seasonal_variant_roman-temporal := CLOCK_ROMAN
clock_seasonal_variant_japanese-temporal := CLOCK_JAPANESE_LATE
clock_seasonal_variant_japanese-temporal-early := CLOCK_JAPANESE_EARLY
clock_seasonal_variant_nuremberg-hours := CLOCK_NUREMBERG_HISTORICAL
clock_seasonal_variant_nuremberg-solar := CLOCK_NUREMBERG_LOCAL

define build_seasonal_clock
$(call clock_target,$(1)): $(SEASONAL_CLOCK_SOURCE) $(CLOCK_RESOURCE_MAP) $(INFILTRATR_COMMON_HEADERS) $(INFILTRATR_COMMON_ARCHIVE)
	@mkdir -p "$(CLOCK_RESOURCE_BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden -D$(clock_seasonal_variant_$(1)) \
		-DINFILTRATR_TEMPORAL_DISPLAY_NAME='"$(clock_name_$(1))"' \
		-shared $(SEASONAL_CLOCK_SOURCE) $(INFILTRATR_COMMON_ARCHIVE) \
		-Wl,--version-script=$(CLOCK_RESOURCE_MAP) -Wl,-z,defs \
		-o "$$@" $(LDFLAGS) $(MATH_LIBS) $(DYNLIB_LIBS)
endef
$(foreach id,$(SEASONAL_CLOCK_RESOURCE_NAMES),$(eval $(call build_seasonal_clock,$(id))))

clock_oneoff_source_chinese-ke := $(CHINESE_KE_CLOCK_SOURCE)
clock_oneoff_source_mean-solar := $(MEAN_SOLAR_CLOCK_SOURCE)
clock_oneoff_source_sidereal := $(SIDEREAL_CLOCK_SOURCE)
clock_oneoff_source_chinese-time := $(CHINESE_TIME_CLOCK_SOURCE)
clock_oneoff_source_solar := $(SOLAR_CLOCK_SOURCE)
clock_oneoff_ids := chinese-ke mean-solar sidereal chinese-time solar

define build_oneoff_clock
$(call clock_target,$(1)): $(clock_oneoff_source_$(1)) $(CLOCK_RESOURCE_MAP) $(INFILTRATR_COMMON_HEADERS)
	@mkdir -p "$(CLOCK_RESOURCE_BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden \
		-frandom-seed=$(REPRO_SEED_PREFIX)-$(1)-clock-resource \
		-DINFILTRATR_TEMPORAL_DISPLAY_NAME='"$(clock_name_$(1))"' \
		-shared $(clock_oneoff_source_$(1)) \
		-Wl,--version-script=$(CLOCK_RESOURCE_MAP) -o "$$@" $(LDFLAGS) \
		$(if $(filter sidereal solar mean-solar,$(1)),$(MATH_LIBS),)
endef
$(foreach id,$(clock_oneoff_ids),$(eval $(call build_oneoff_clock,$(id))))


.PHONY: all check package install clean common-build
all: $(CALENDAR_RESOURCES) $(CLOCK_RESOURCES)
common-build:
	$(MAKE) -C "$(COMMON_ROOT)" CC="$(CC)" AR="$(AR)" \
	    BUILD_DIR="resources/temporal/$(BUILD_DIR)/common" \
	    CPPFLAGS="-Iinclude" CFLAGS="-O2 -g -fPIC -ffile-prefix-map=$(abspath $(COMMON_ROOT))=. -fdebug-prefix-map=$(abspath $(COMMON_ROOT))=." all
$(INFILTRATR_COMMON_ARCHIVE): common-build
	@:
$(CALENDAR_RESOURCES): $(RESOURCE_ENGINE_HEADERS) $(HEADERS)
$(BUILD_DIR)/%.o: src/%.c $(RESOURCE_ENGINE_HEADERS) $(HEADERS)
	@mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE_CFLAGS) -MMD -MP -c "$<" -o "$@"
$(BUILD_DIR)/test-clocks: tests/test-clocks.c tests/resource-test-path.h $(CLOCK_RESOURCES) $(INFILTRATR_COMMON_ARCHIVE)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE_CFLAGS) \
	    -DTEST_CLOCK_RESOURCE_DIRECTORY=\"$(abspath $(CLOCK_RESOURCE_BUILD_DIR))\" \
	    "$<" $(INFILTRATR_COMMON_ARCHIVE) $(CORE_LIBS) -ldl -lm -o "$@"
$(BUILD_DIR)/test-calendars: tests/test-calendars.c tests/resource-test-path.h $(CALENDAR_RESOURCES) $(INFILTRATR_COMMON_ARCHIVE)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE_CFLAGS) \
	    -DTEST_CALENDAR_RESOURCE_DIRECTORY=\"$(abspath $(CALENDAR_RESOURCE_BUILD_DIR))\" \
	    "$<" $(INFILTRATR_COMMON_ARCHIVE) $(CORE_LIBS) -ldl -lm -o "$@"
$(BUILD_DIR)/test-engines: tests/test-engines.c $(RESOURCE_ENGINE_ARCHIVE) $(INFILTRATR_COMMON_ARCHIVE)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE_CFLAGS) tests/test-engines.c src/support/julian-day.c \
	    $(RESOURCE_ENGINE_ARCHIVE) $(INFILTRATR_COMMON_ARCHIVE) $(CORE_LIBS) $(ICU_BRIDGE_LIBS) -lm -o "$@"
check: all $(BUILD_DIR)/test-clocks $(BUILD_DIR)/test-calendars $(BUILD_DIR)/test-engines
	./$(BUILD_DIR)/test-clocks
	./$(BUILD_DIR)/test-calendars
	./$(BUILD_DIR)/test-engines
	python3 tools/check-boundary.py "$(BUILD_DIR)"
package: check
	tools/build-package.sh "$(BUILD_DIR)"
clean:
	rm -rf "$(BUILD_DIR)"
-include $(RESOURCE_ENGINE_OBJECTS:.o=.d)
