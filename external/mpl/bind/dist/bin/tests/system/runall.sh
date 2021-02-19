#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
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
#               (This is ignored when running on Windows as the tests are always
#               run sequentially on that platform.)

SYSTEMTESTTOP=.
. $SYSTEMTESTTOP/conf.sh

usage="Usage: ./runall.sh [-c] [-n] [numprocesses]"

# Preserve values of environment variables which are already set.

SYSTEMTEST_FORCE_COLOR=${SYSTEMTEST_FORCE_COLOR:-0}
SYSTEMTEST_NO_CLEAN=${SYSTEMTEST_NO_CLEAN:-0}

# Handle command line switches if present.

while getopts "cn" flag; do
    case "$flag" in
        c) SYSTEMTEST_FORCE_COLOR=1 ;;
        n) SYSTEMTEST_NO_CLEAN=1 ;;
    esac
done
export NOCLEAN
shift `expr $OPTIND - 1`

# Obtain number of processes to use.

if [ $# -eq 0 ]; then
    numproc=1
elif [ $# -eq 1 ]; then
    test "$1" -eq "$1" > /dev/null 2>&1
    if [ $? -ne 0 ]; then
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
    if [ "$CYGWIN" = "" ]; then
        # Running on Unix, use "make" to run tests in parallel.
        make -j $numproc check
        status=$?
    else
        # Running on Windows: Cygwin "make" is available, but isn't being
        # used for the build. So we create a special makefile for the purpose
        # of parallel execution of system tests, and use that.
        $SHELL parallel.sh > parallel.mk
        make -f parallel.mk -j $numproc check
        $SHELL ./runsequential.sh
        $SHELL ./testsummary.sh || status=1
    fi
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
    {
        for testdir in $SUBDIRS; do
            $SHELL run.sh $testdir || status=1
        done
    } 2>&1 | tee "systests.output"
fi

exit $status
