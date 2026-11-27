#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Generate the provider's single installed PDF manual.
set -euo pipefail
root=$(cd "$(dirname "$0")/../.." && pwd)
exec bash "$root/common/documentation-tools/temporal-guide.sh" calendar "${1:?PDF destination required}" "$root"
