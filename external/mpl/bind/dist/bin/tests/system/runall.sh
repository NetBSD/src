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

# Run all the system tests.
#
# Usage:
#    runall.sh [-c] [-n] [numprocesses]
#
#   -c          Force colored output.
#
#   -n          Noclean.  Keep all output files produced by all tests.  These
#               can later be removed by running "cleanall.sh".
#
#   numprocess  Number of concurrent processes to use when running the tests.
#               The default is one, which causes the tests to run sequentially.

. ./conf.sh

usage="Usage: ./runall.sh [-c] [-n] [numprocesses]"

# Preserve values of environment variables which are already set.

SYSTEMTEST_FORCE_COLOR=${SYSTEMTEST_FORCE_COLOR:-0}
SYSTEMTEST_NO_CLEAN=${SYSTEMTEST_NO_CLEAN:-0}

# Handle command line switches if present.

while getopts "cn-" flag; do
  case "$flag" in
    c) SYSTEMTEST_FORCE_COLOR=1 ;;
    n) SYSTEMTEST_NO_CLEAN=1 ;;
    -) break ;;
    *) exit 1 ;;
  esac
done
export NOCLEAN
shift $((OPTIND - 1))

# Obtain number of processes to use.

if [ $# -eq 0 ]; then
  numproc=1
elif [ $# -eq 1 ]; then
  if [ "$1" -ne "$1" ] 2>&1; then
    # Value passed is not numeric
    echo "$usage" >&2
    exit 1
  fi
  numproc=$1
else
  echo "$usage" >&2
  exit 1
fi

# Run the tests.

export SYSTEMTEST_FORCE_COLOR
export SYSTEMTEST_NO_CLEAN

status=0

if [ "$NOPARALLEL" = "" ]; then
  # use "make" to run tests in parallel.
  make -j "$numproc" check
  status=$?
else
  # the NOPARALLEL environment variable indicates that tests must be
  # run sequentially.
  $PERL testsock.pl || {
    cat <<-EOF
	I:NOTE: System tests were skipped because they require the
	I:      test IP addresses 10.53.0.* to be configured as alias
	I:      addresses on the loopback interface.  Please run
	I:      "bin/tests/system/ifconfig.sh up" as root to configure them.
	EOF
    exit 1
  }
  (
    status=0
    for testdir in $SUBDIRS; do
      $SHELL legacy.run.sh -r "$testdir" || status=1
    done
    echo "$status" >systests.status
  ) 2>&1 | tee "systests.output"
  read -r status <systests.status
  rm systests.status
fi

exit "$status"
