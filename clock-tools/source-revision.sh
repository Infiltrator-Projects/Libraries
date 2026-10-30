#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Native .run archives include an immutable source-commit file; live checkout
# builds fall back to their own Git HEAD. Bash's $(<file || command) is NOT
# a reliable way to read a file in compound command substitutions.
set -euo pipefail
if [[ -e SOURCE-REVISION || -L SOURCE-REVISION ]]; then
  [[ -f SOURCE-REVISION && ! -L SOURCE-REVISION ]] || {
    echo 'Invalid native clock source-revision file' >&2; exit 1;
  }
  revision=$(cat -- SOURCE-REVISION)
else
  revision=$(git rev-parse --verify HEAD)
fi
[[ "$revision" =~ ^[0-9a-f]{40}$ ]] || {
  echo 'Native clock source commit must contain exactly 40 lowercase hex digits' >&2; exit 1;
}
printf '%s\n' "$revision"
