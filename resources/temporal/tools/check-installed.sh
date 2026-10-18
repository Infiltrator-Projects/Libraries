#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Compile only the public-ABI probes; never rebuild the installed providers.
set -eu
cd "$(dirname "$0")/.."
make -C ../.. all
probe_directory=$(mktemp -d)
trap 'rm -rf "$probe_directory"' EXIT HUP INT TERM
for kind in calendars clocks; do
    case "$kind" in
        calendars) definition=TEST_CALENDAR_RESOURCE_DIRECTORY; directory=Calendars ;;
        clocks) definition=TEST_CLOCK_RESOURCE_DIRECTORY; directory=Clocks ;;
    esac
    # pkg-config emits compiler arguments, intentionally split by the shell.
    # shellcheck disable=SC2046
    "${CC:-cc}" -std=c11 -O2 -I../../include $(pkg-config --cflags glib-2.0) \
        "-D$definition=\"/System/Resources/$directory\"" "tests/test-$kind.c" \
        ../../build/libinfiltratr-common.a $(pkg-config --libs glib-2.0) -lm -ldl \
        -o "$probe_directory/test-$kind"
    "$probe_directory/test-$kind"
done
