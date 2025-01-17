#!/usr/bin/env bash
#
# SPDX-License-Identifier: GPL-2.0-only
#
# SPDX-FileCopyrightText: 2022 EfficiOS Inc.
#

if [ "x${URCU_TESTS_SRCDIR:-}" != "x" ]; then
	UTILSSH="$URCU_TESTS_SRCDIR/utils/utils.sh"
else
	UTILSSH="$(dirname "$0")/../utils/utils.sh"
fi

# Enable TAP
SH_TAP=1

# shellcheck source=../utils/utils.sh
source "$UTILSSH"


# Create a temporary file for tests output
TMPFILE=$(mktemp)

# Set trap to delete the temporary file on exit and call tap.sh '_exit'
trap 'rm -f "$TMPFILE"; _exit' EXIT


NUM_TESTS=1

plan_tests	${NUM_TESTS}

#for a in test_urcu_gc test_urcu_gc_mb test_urcu_qsbr_gc; do
for a in test_urcu_gc; do
	okx "${URCU_TESTS_TIME_BIN}" "${URCU_TESTS_BUILDDIR}/benchmark/${a}" "$@" 2>"${TMPFILE}"
	diag "time: $(cat "${TMPFILE}")"
done
