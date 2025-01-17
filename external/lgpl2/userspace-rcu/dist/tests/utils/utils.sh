#!/usr/bin/env bash
#
# SPDX-License-Identifier: GPL-2.0-only
#
# SPDX-FileCopyrightText: 2022 EfficiOS Inc.
#

# This file is meant to be sourced at the start of shell script-based tests.


# Error out when encountering an undefined variable
set -u

# If "readlink -f" is available, get a resolved absolute path to the
# tests source dir, otherwise make do with a relative path.
scriptdir="$(dirname "${BASH_SOURCE[0]}")"
if readlink -f "." >/dev/null 2>&1; then
	testsdir=$(readlink -f "$scriptdir/..")
else
	testsdir="$scriptdir/.."
fi

# The OS on which we are running. See [1] for possible values of 'uname -s'.
# We do a bit of translation to ease our life down the road for comparison.
# Export it so that called executables can use it.
# [1] https://en.wikipedia.org/wiki/Uname#Examples
if [ "x${URCU_TESTS_OS_TYPE:-}" = "x" ]; then
	URCU_TESTS_OS_TYPE="$(uname -s)"
	case "$URCU_TESTS_OS_TYPE" in
	MINGW*)
		URCU_TESTS_OS_TYPE="mingw"
		;;
	Darwin)
		URCU_TESTS_OS_TYPE="darwin"
		;;
	Linux)
		URCU_TESTS_OS_TYPE="linux"
		;;
	CYGWIN*)
		URCU_TESTS_OS_TYPE="cygwin"
		;;
	*)
		URCU_TESTS_OS_TYPE="unsupported"
		;;
	esac
fi
export URCU_TESTS_OS_TYPE

# Allow overriding the source and build directories
if [ "x${URCU_TESTS_SRCDIR:-}" = "x" ]; then
	URCU_TESTS_SRCDIR="$testsdir"
fi
export URCU_TESTS_SRCDIR

if [ "x${URCU_TESTS_BUILDDIR:-}" = "x" ]; then
	URCU_TESTS_BUILDDIR="$testsdir"
fi
export URCU_TESTS_BUILDDIR

# Source the generated environment file if it's present.
if [ -f "${URCU_TESTS_BUILDDIR}/utils/env.sh" ]; then
	# shellcheck source=./env.sh
	. "${URCU_TESTS_BUILDDIR}/utils/env.sh"
fi


### External Tools ###

if [ "x${URCU_TESTS_NPROC_BIN:-}" = "x" ]; then
	URCU_TESTS_NPROC_BIN="nproc"
fi
export URCU_TESTS_NPROC_BIN

if [ "x${URCU_TESTS_GETCONF_BIN:-}" = "x" ]; then
	URCU_TESTS_GETCONF_BIN="getconf"
fi
export URCU_TESTS_GETCONF_BIN


# By default, it will not source tap.sh.  If you to tap output directly from
# the test script, define the 'SH_TAP' variable to '1' before sourcing this
# script.
if [ "x${SH_TAP:-}" = x1 ]; then
	# shellcheck source=./tap.sh
	. "${URCU_TESTS_SRCDIR}/utils/tap.sh"
fi


### Functions ###

# Print the number of online CPUs, fallback to '1' if no tools to count CPUs
# are found in the environment.
urcu_nproc() {
	if command -v "${URCU_TESTS_NPROC_BIN}" >/dev/null 2>&1; then
		"${URCU_TESTS_NPROC_BIN}"
	elif command -v "${URCU_TESTS_GETCONF_BIN}" >/dev/null 2>&1; then
		"${URCU_TESTS_GETCONF_BIN}" _NPROCESSORS_ONLN
	else
		# Fallback to '1'
		echo 1
	fi
}

# Shell implementation of the 'seq' command.
urcu_xseq() {
	local i=$1

	while [[ "$i" -le "$2" ]]; do
		echo "$i"
		i=$(( i + 1 ))
	done
}
