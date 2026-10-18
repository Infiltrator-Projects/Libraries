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
HEBREW_RESOURCE := $(CALENDAR_RESOURCE_BUILD_DIR)/hebrew.so
GREGORIAN_RESOURCE := $(CALENDAR_RESOURCE_BUILD_DIR)/gregorian.so
PERSIAN_RESOURCE := $(CALENDAR_RESOURCE_BUILD_DIR)/persian.so
COPTIC_RESOURCE := $(CALENDAR_RESOURCE_BUILD_DIR)/coptic.so
CALENDAR_RESOURCES := $(HEBREW_RESOURCE) $(GREGORIAN_RESOURCE) \
	$(PERSIAN_RESOURCE) $(COPTIC_RESOURCE) \
	$(addprefix $(CALENDAR_RESOURCE_BUILD_DIR)/,$(addsuffix .so,$(EXTRACTED_CALENDAR_RESOURCE_NAMES)))
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
CLOCK_RESOURCES := $(addprefix $(CLOCK_RESOURCE_BUILD_DIR)/,$(addsuffix .so,$(CLOCK_RESOURCE_NAMES)))
$(RESOURCE_ENGINE_ARCHIVE): $(RESOURCE_ENGINE_OBJECTS)
	@rm -f "$@"
	$(AR) rcsD "$@" $(RESOURCE_ENGINE_OBJECTS)

$(addprefix $(CALENDAR_RESOURCE_BUILD_DIR)/,$(addsuffix .so,$(EXTRACTED_CALENDAR_RESOURCE_NAMES))): $(CALENDAR_RESOURCE_BUILD_DIR)/%.so: src/calendars/native-calendar.c src/calendars/catalogue.h $(RESOURCE_ENGINE_ARCHIVE) src/support/julian-day.c $(INFILTRATR_COMMON_ARCHIVE) $(CALENDAR_RESOURCE_MAP)
	@mkdir -p "$(CALENDAR_RESOURCE_BUILD_DIR)"
	@case "$*" in \
	  iso-week) variant=ISO_WEEK ;; \
	  julian) variant=JULIAN ;; \
	  revised-julian) variant=REVISED_JULIAN ;; \
	  islamic-umalqura) variant=ISLAMIC_UMM_AL_QURA ;; \
	  islamic-civil) variant=ISLAMIC_CIVIL ;; \
	  islamic-tbla) variant=ISLAMIC_TBLA ;; \
	  islamic) variant=ISLAMIC ;; \
	  bahai) variant=BAHAI ;; \
	  buddhist) variant=BUDDHIST ;; \
	  ethiopian) variant=ETHIOPIAN ;; \
	  ethiopic-amete-alem) variant=ETHIOPIC_AMETE_ALEM ;; \
	  chinese) variant=CHINESE ;; \
	  dangi) variant=DANGI ;; \
	  indian) variant=INDIAN ;; \
	  japanese) variant=JAPANESE ;; \
	  minguo) variant=MINGUO ;; \
	  roman) variant=ROMAN ;; \
	  byzantine) variant=BYZANTINE ;; \
	  egyptian-nabonassar) variant=EGYPTIAN_NABONASSAR ;; \
	  armenian-traditional) variant=ARMENIAN_TRADITIONAL ;; \
	  mayan) variant=MAYAN ;; \
	  french-republican) variant=FRENCH_REPUBLICAN ;; \
	  swedish-historical) variant=SWEDISH_HISTORICAL ;; \
	  international-fixed) variant=INTERNATIONAL_FIXED ;; \
	  world) variant=WORLD ;; \
	  positivist) variant=POSITIVIST ;; \
	  *) exit 1 ;; esac; \
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden -DRESOURCE_$$variant $(CORE_CFLAGS) \
		-shared src/calendars/native-calendar.c src/support/julian-day.c \
		$(RESOURCE_ENGINE_ARCHIVE) $(INFILTRATR_COMMON_ARCHIVE) \
		-Wl,--version-script=$(CALENDAR_RESOURCE_MAP) -Wl,-z,defs \
		-o $@ $(LDFLAGS) $(CORE_LIBS) $(ICU_BRIDGE_LIBS) $(MATH_LIBS)

$(HEBREW_RESOURCE): $(HEBREW_RESOURCE_SOURCE) $(CALENDAR_RESOURCE_MAP) \
		src/calendars/engine/icu-compat-bridge.c $(INFILTRATR_COMMON_HEADERS) \
		$(INFILTRATR_COMMON_ARCHIVE)
	@mkdir -p "$(CALENDAR_RESOURCE_BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden \
		-frandom-seed=$(REPRO_SEED_PREFIX)-hebrew-resource $(CORE_CFLAGS) \
		-shared $(HEBREW_RESOURCE_SOURCE) src/calendars/engine/icu-compat-bridge.c \
		$(INFILTRATR_COMMON_ARCHIVE) \
		-Wl,--version-script=$(CALENDAR_RESOURCE_MAP) \
		-o $@ $(LDFLAGS) $(CORE_LIBS) $(ICU_BRIDGE_LIBS) $(MATH_LIBS)

$(GREGORIAN_RESOURCE): $(GREGORIAN_RESOURCE_SOURCE) $(CALENDAR_RESOURCE_MAP) \
		src/support/julian-day.c src/calendars/engine/icu-calendar.c \
		src/calendars/engine/icu-compat-bridge.c $(INFILTRATR_COMMON_HEADERS) \
		$(INFILTRATR_COMMON_ARCHIVE)
	@mkdir -p "$(CALENDAR_RESOURCE_BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden \
		-frandom-seed=$(REPRO_SEED_PREFIX)-gregorian-resource $(CORE_CFLAGS) \
		-shared $(GREGORIAN_RESOURCE_SOURCE) src/support/julian-day.c \
		src/calendars/engine/icu-calendar.c src/calendars/engine/icu-compat-bridge.c \
		$(INFILTRATR_COMMON_ARCHIVE) \
		-Wl,--version-script=$(CALENDAR_RESOURCE_MAP) \
		-o $@ $(LDFLAGS) $(CORE_LIBS) $(ICU_BRIDGE_LIBS) $(MATH_LIBS)

$(PERSIAN_RESOURCE): $(PERSIAN_RESOURCE_SOURCE) $(CALENDAR_RESOURCE_MAP) \
		$(PERSIAN_RESOURCE_ARITHMETIC) src/calendars/engine/icu-calendar.c \
		src/calendars/engine/icu-compat-bridge.c $(INFILTRATR_COMMON_HEADERS) \
		$(INFILTRATR_COMMON_ARCHIVE)
	@mkdir -p "$(CALENDAR_RESOURCE_BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden \
		-frandom-seed=$(REPRO_SEED_PREFIX)-persian-resource $(CORE_CFLAGS) \
		-shared $(PERSIAN_RESOURCE_SOURCE) $(PERSIAN_RESOURCE_ARITHMETIC) src/support/julian-day.c \
		src/calendars/engine/icu-calendar.c src/calendars/engine/icu-compat-bridge.c \
		$(INFILTRATR_COMMON_ARCHIVE) \
		-Wl,--version-script=$(CALENDAR_RESOURCE_MAP) \
		-o $@ $(LDFLAGS) $(CORE_LIBS) $(ICU_BRIDGE_LIBS) $(MATH_LIBS)

$(COPTIC_RESOURCE): $(COPTIC_RESOURCE_SOURCE) $(CALENDAR_RESOURCE_MAP) \
		src/calendars/engine/calendar-arithmetic-coptic.c src/calendars/engine/icu-calendar.c \
		src/calendars/engine/icu-compat-bridge.c $(INFILTRATR_COMMON_HEADERS) \
		$(INFILTRATR_COMMON_ARCHIVE)
	@mkdir -p "$(CALENDAR_RESOURCE_BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden \
		-frandom-seed=$(REPRO_SEED_PREFIX)-coptic-resource $(CORE_CFLAGS) \
		-shared $(COPTIC_RESOURCE_SOURCE) src/calendars/engine/calendar-arithmetic-coptic.c src/support/julian-day.c \
		src/calendars/engine/icu-calendar.c src/calendars/engine/icu-compat-bridge.c \
		$(INFILTRATR_COMMON_ARCHIVE) \
		-Wl,--version-script=$(CALENDAR_RESOURCE_MAP) \
		-o $@ $(LDFLAGS) $(CORE_LIBS) $(ICU_BRIDGE_LIBS) $(MATH_LIBS)

$(addprefix $(CLOCK_RESOURCE_BUILD_DIR)/,$(addsuffix .so,$(FIXED_CLOCK_RESOURCE_NAMES))): $(CLOCK_RESOURCE_BUILD_DIR)/%.so: $(CLOCK_RESOURCE_SOURCE) $(CLOCK_RESOURCE_MAP) $(INFILTRATR_COMMON_HEADERS)
	@mkdir -p "$(CLOCK_RESOURCE_BUILD_DIR)"
	@case "$*" in decimal) kind=1; seconds=true ;; internet) kind=2; seconds=true ;; \
		unix) kind=3; seconds=false ;; hexadecimal) kind=4; seconds=false ;; \
		binary) kind=5; seconds=true ;; *) exit 1 ;; esac; \
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden \
		-frandom-seed=$(REPRO_SEED_PREFIX)-$*-clock-resource \
		-DCLOCK_RESOURCE_KIND=$$kind -DCLOCK_RESOURCE_ID=\"$*\" \
		-DCLOCK_RESOURCE_NAME=\"$*\" -DCLOCK_RESOURCE_SECONDS=$$seconds \
		-shared $(CLOCK_RESOURCE_SOURCE) \
		-Wl,--version-script=$(CLOCK_RESOURCE_MAP) -o $@ $(LDFLAGS)

$(addprefix $(CLOCK_RESOURCE_BUILD_DIR)/,$(addsuffix .so,$(JULIAN_CLOCK_RESOURCE_NAMES))): $(CLOCK_RESOURCE_BUILD_DIR)/%.so: $(JULIAN_CLOCK_RESOURCE_SOURCE) $(CLOCK_RESOURCE_MAP) $(INFILTRATR_COMMON_HEADERS)
	@mkdir -p "$(CLOCK_RESOURCE_BUILD_DIR)"
	@case "$*" in julian) kind=1 ;; modified-julian) kind=2 ;; *) exit 1 ;; esac; \
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden \
		-frandom-seed=$(REPRO_SEED_PREFIX)-$*-clock-resource \
		-DJULIAN_CLOCK_KIND=$$kind \
		-shared $(JULIAN_CLOCK_RESOURCE_SOURCE) \
		-Wl,--version-script=$(CLOCK_RESOURCE_MAP) -o $@ $(LDFLAGS)

$(addprefix $(CLOCK_RESOURCE_BUILD_DIR)/,$(addsuffix .so,$(CONVENTIONAL_CLOCK_RESOURCE_NAMES))): $(CLOCK_RESOURCE_BUILD_DIR)/%.so: $(CONVENTIONAL_CLOCK_RESOURCE_SOURCE) $(CLOCK_RESOURCE_MAP) $(INFILTRATR_COMMON_HEADERS)
	@mkdir -p "$(CLOCK_RESOURCE_BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden \
		-frandom-seed=$(REPRO_SEED_PREFIX)-$*-clock-resource \
		-DCONVENTIONAL_CLOCK_KIND=$(if $(filter standard-12,$*),12,24) \
		-shared $(CONVENTIONAL_CLOCK_RESOURCE_SOURCE) \
		-Wl,--version-script=$(CLOCK_RESOURCE_MAP) -o $@ $(LDFLAGS)

$(CLOCK_RESOURCE_BUILD_DIR)/chinese-ke.so: $(CHINESE_KE_CLOCK_SOURCE) $(CLOCK_RESOURCE_MAP) $(INFILTRATR_COMMON_HEADERS)
	@mkdir -p "$(CLOCK_RESOURCE_BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden \
		-frandom-seed=$(REPRO_SEED_PREFIX)-chinese-ke-clock-resource \
		-shared $(CHINESE_KE_CLOCK_SOURCE) \
		-Wl,--version-script=$(CLOCK_RESOURCE_MAP) -o $@ $(LDFLAGS)

$(addprefix $(CLOCK_RESOURCE_BUILD_DIR)/,$(addsuffix .so,$(ORIGIN_CLOCK_RESOURCE_NAMES))): $(CLOCK_RESOURCE_BUILD_DIR)/%.so: $(SOLAR_ORIGIN_CLOCK_SOURCE) $(CLOCK_RESOURCE_MAP) $(INFILTRATR_COMMON_HEADERS)
	@mkdir -p "$(CLOCK_RESOURCE_BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden \
		-frandom-seed=$(REPRO_SEED_PREFIX)-$*-solar-origin-clock \
		$(if $(filter babylonian-hours,$*),-DCLOCK_SUNRISE=1,) \
		$(if $(filter babylonian-ancient,$*),-DCLOCK_ANCIENT_BABYLONIAN=1,) \
		$(if $(filter indian-ghati,$*),-DCLOCK_GHATI=1,) \
		-shared $(SOLAR_ORIGIN_CLOCK_SOURCE) \
		-Wl,--version-script=$(CLOCK_RESOURCE_MAP) -o $@ $(LDFLAGS) $(MATH_LIBS)

$(addprefix $(CLOCK_RESOURCE_BUILD_DIR)/,$(addsuffix .so,$(SEASONAL_CLOCK_RESOURCE_NAMES))): $(CLOCK_RESOURCE_BUILD_DIR)/%.so: $(SEASONAL_CLOCK_SOURCE) $(CLOCK_RESOURCE_MAP) $(INFILTRATR_COMMON_HEADERS) $(INFILTRATR_COMMON_ARCHIVE)
	@mkdir -p "$(CLOCK_RESOURCE_BUILD_DIR)"
	@case "$*" in \
	  roman-temporal) kind=CLOCK_ROMAN ;; \
	  japanese-temporal) kind=CLOCK_JAPANESE_LATE ;; \
	  japanese-temporal-early) kind=CLOCK_JAPANESE_EARLY ;; \
	  nuremberg-solar) kind=CLOCK_NUREMBERG_LOCAL ;; \
	  nuremberg-hours) kind=CLOCK_NUREMBERG_HISTORICAL ;; \
	  *) exit 1 ;; esac; \
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden -D$$kind \
		-shared $(SEASONAL_CLOCK_SOURCE) $(INFILTRATR_COMMON_ARCHIVE) \
		-Wl,--version-script=$(CLOCK_RESOURCE_MAP) -Wl,-z,defs \
		-o $@ $(LDFLAGS) $(MATH_LIBS) $(DYNLIB_LIBS)

$(CLOCK_RESOURCE_BUILD_DIR)/solar.so: $(SOLAR_CLOCK_SOURCE) $(CLOCK_RESOURCE_MAP) $(INFILTRATR_COMMON_HEADERS)
	@mkdir -p "$(CLOCK_RESOURCE_BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden \
		-frandom-seed=$(REPRO_SEED_PREFIX)-solar-clock-resource \
		-shared $(SOLAR_CLOCK_SOURCE) \
		-Wl,--version-script=$(CLOCK_RESOURCE_MAP) -o $@ $(LDFLAGS) $(MATH_LIBS)

$(CLOCK_RESOURCE_BUILD_DIR)/chinese-time.so: $(CHINESE_TIME_CLOCK_SOURCE) $(CLOCK_RESOURCE_MAP) $(INFILTRATR_COMMON_HEADERS)
	@mkdir -p "$(CLOCK_RESOURCE_BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden \
		-frandom-seed=$(REPRO_SEED_PREFIX)-chinese-time-clock-resource \
		-shared $(CHINESE_TIME_CLOCK_SOURCE) \
		-Wl,--version-script=$(CLOCK_RESOURCE_MAP) -o $@ $(LDFLAGS)

$(CLOCK_RESOURCE_BUILD_DIR)/sidereal.so: $(SIDEREAL_CLOCK_SOURCE) $(CLOCK_RESOURCE_MAP) $(INFILTRATR_COMMON_HEADERS)
	@mkdir -p "$(CLOCK_RESOURCE_BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden \
		-frandom-seed=$(REPRO_SEED_PREFIX)-sidereal-clock-resource \
		-shared $(SIDEREAL_CLOCK_SOURCE) \
		-Wl,--version-script=$(CLOCK_RESOURCE_MAP) -o $@ $(LDFLAGS) $(MATH_LIBS)

$(CLOCK_RESOURCE_BUILD_DIR)/mean-solar.so: $(MEAN_SOLAR_CLOCK_SOURCE) $(CLOCK_RESOURCE_MAP) $(INFILTRATR_COMMON_HEADERS)
	@mkdir -p "$(CLOCK_RESOURCE_BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden \
		-frandom-seed=$(REPRO_SEED_PREFIX)-mean-solar-clock-resource \
		-shared $(MEAN_SOLAR_CLOCK_SOURCE) \
		-Wl,--version-script=$(CLOCK_RESOURCE_MAP) -o $@ $(LDFLAGS) $(MATH_LIBS)


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
$(BUILD_DIR)/test-clocks: tests/test-clocks.c $(CLOCK_RESOURCES) $(INFILTRATR_COMMON_ARCHIVE)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE_CFLAGS) \
	    -DTEST_CLOCK_RESOURCE_DIRECTORY=\"$(abspath $(CLOCK_RESOURCE_BUILD_DIR))\" \
	    "$<" $(INFILTRATR_COMMON_ARCHIVE) $(CORE_LIBS) -ldl -lm -o "$@"
$(BUILD_DIR)/test-calendars: tests/test-calendars.c $(CALENDAR_RESOURCES) $(INFILTRATR_COMMON_ARCHIVE)
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
