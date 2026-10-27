#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Build only Common's public-ABI consumer probe, never provider implementations.
set -eu
ROOT=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
common_build=${COMMON_BUILD_DIR:-"$ROOT/build"}
if [ ! -x "$common_build/temporal-published-providers-smoke" ]; then
    make -C "$ROOT" "BUILD_DIR=$common_build" CC="${CC:-cc}" "$common_build/temporal-published-providers-smoke"
fi
python3 "$ROOT/tests/check-published-temporal.py" "$common_build/temporal-published-providers-smoke"
