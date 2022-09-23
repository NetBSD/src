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

# Creates the system tests output file from the various test.output.* files.  It
# then searches that file and prints the number of tests passed, failed, not
# run.  It also checks whether the IP addresses 10.53.0.[1-8] were set up and,
# if not, prints a warning.
#
# Usage:
#    testsummary.sh [-n]
#
# -n	Do NOT delete the individual test.output.* files after concatenating
#	them into systests.output.
#
# Status return:
# 0 - no tests failed
# 1 - one or more tests failed

SYSTEMTESTTOP=.
. $SYSTEMTESTTOP/conf.sh

keepfile=0

while getopts "n" flag; do
    case $flag in
	n) keepfile=1 ;;
    esac
done

if [ `ls test.output.* 2> /dev/null | wc -l` -eq 0 ]; then
    echowarn "I:No 'test.output.*' files were found."
    echowarn "I:Printing summary from pre-existing 'systests.output'."
else
    cat test.output.* > systests.output
    if [ $keepfile -eq 0 ]; then
        rm -f test.output.*
    fi
fi

status=0
echoinfo "I:System test result summary:"
echoinfo "`grep 'R:[a-z0-9_-][a-z0-9_-]*:[A-Z][A-Z]*' systests.output | cut -d':' -f3 | sort | uniq -c | sed -e 's/^/I:/'`"

FAILED_TESTS=`grep 'R:[a-z0-9_-][a-z0-9_-]*:FAIL' systests.output | cut -d':' -f2 | sort | sed -e 's/^/I:      /'`
if [ -n "${FAILED_TESTS}" ]; then
	echoinfo "I:The following system tests failed:"
	echoinfo "${FAILED_TESTS}"
	status=1
fi

CRASHED_TESTS=$(find . \( -name 'core' -or -name 'core.*' -or -name '*.core' \) ! -name '*.txt' | cut -d'/' -f2 | sort -u | sed -e 's/^/I:      /')
if [ -n "${CRASHED_TESTS}" ]; then
	echoinfo "I:Core dumps were found for the following system tests:"
	echoinfo "${CRASHED_TESTS}"
fi

ASSERTION_FAILED_TESTS=`find . -name named.run | xargs grep "assertion failure" | cut -d'/' -f2 | sort -u | sed -e 's/^/I:      /'`
if [ -n "${ASSERTION_FAILED_TESTS}" ]; then
	echoinfo "I:Assertion failures were detected for the following system tests:"
	echoinfo "${ASSERTION_FAILED_TESTS}"
fi

TSAN_REPORT_TESTS=`find . -name 'tsan.*' | cut -d'/' -f2 | sort -u | sed -e 's/^/I:      /'`
if [ -n "${TSAN_REPORT_TESTS}" ]; then
	echoinfo "I:ThreadSanitizer reported issues for the following system tests:"
	echoinfo "${TSAN_REPORT_TESTS}"
fi

RESULTS_FOUND=`grep -c 'R:[a-z0-9_-][a-z0-9_-]*:[A-Z][A-Z]*' systests.output`
TESTS_RUN=`echo "${SUBDIRS}" | wc -w`
if [ "${RESULTS_FOUND}" -ne "${TESTS_RUN}" ]; then
	echofail "I:Found ${RESULTS_FOUND} test results, but ${TESTS_RUN} tests were run"
	status=1
fi

exit $status
