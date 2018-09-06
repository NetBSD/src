#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

#
# Clean up after a specified system test.
#

SYSTEMTESTTOP=.
. $SYSTEMTESTTOP/conf.sh

# See if the "-r" flag is present.  This will usually be set when all the tests
# are run (e.g. from "runall.sh") and tells the script not to delete the
# test.output file created by run.sh.  This is because the script running all
# the tests will call "testsummary.sh", which will concatenate all test output
# files into a single systests.output.

runall=0

while getopts "r" flag; do
    case $flag in
	r) runall=1 ;;
    esac
done
shift `expr $OPTIND - 1`

if [ $# -eq 0 ]; then
    echo "usage: $0 [-r] test-directory" >&2
    exit 1
fi

systest=$1
shift

if [ $runall -eq 0 ]; then
    rm -f $systest/test.output
fi

if [ -f $systest/clean.sh ]; then
    ( cd $systest && $SHELL clean.sh "$@" )
else
    echo "Test directory $systest does not exist" >&2
    exit 1
fi
