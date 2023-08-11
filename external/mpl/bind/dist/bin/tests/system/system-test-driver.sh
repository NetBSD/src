#!/bin/sh

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# shellcheck disable=SC2181
# shellcheck disable=SC2034

usage() {
    echo "$0 --test-name=NAME --log-file=PATH.log --trs-file=PATH.trs --color-tests={yes|no} --expect-failure={yes|no} --enable-hard-errors={yes|no}"
}

#
# This requires GNU getopt
#
getopt --test >/dev/null
if [ "$?" -ne 4 ]; then
    echo "fatal: GNU getopt is required"
    exit 1
fi

OPTS=$(getopt --shell "sh" --name "$(basename "$0")" --options '' --longoptions test-name:,log-file:,trs-file:,color-tests:,expect-failure:,enable-hard-errors: -- "$@")

if [ "$?" != 0 ] ; then echo "Failed parsing options." >&2 ; exit 1 ; fi

eval set -- "$OPTS"

TEST_NAME=
LOG_FILE=
TRS_FILE=
COLOR_TESTS=yes
EXPECT_FAILURE=no
HARD_ERRORS=yes

while true; do
    case "$1" in
	--test-name ) TEST_NAME="$2"; shift; shift ;;
	--log-file ) LOG_FILE="$2"; shift; shift ;;
	--trs-file ) TRS_FILE="$2"; shift; shift ;;
	--color-tests ) COLOR_TESTS="$2"; shift; shift ;;
	--expect-failure ) EXPECT_FAILURE="$2"; shift; shift ;;
	--hard-errors ) HARD_ERRORS="$2"; shift; shift ;;
	-- ) shift; break ;;
	*) break ;;
    esac
done

if [ -z "$1" ]; then
    echo "fatal: test name required"
    usage
    exit 1
fi

TEST_PROGRAM="$1"
shift

if [ -z "$TEST_NAME" ]; then
    TEST_NAME="$(basename "$TEST_PROGRAM")"
fi
if [ -z "$LOG_FILE" ]; then
    LOG_FILE="$TEST_PROGRAM.log"
fi
if [ -z "$TRS_FILE" ]; then
    TRS_FILE="$TEST_PROGRAM.trs"
fi

echo "Running $TEST_PROGRAM"

random=$(awk 'BEGIN { srand(); print int(rand()*32768) }' /dev/null)
./run.sh -p "$((random%32000+32000))" "$@" "$TEST_PROGRAM"

exit $?
