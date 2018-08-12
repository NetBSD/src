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

# Run system tests that must be run sequentially
#
# Note: Use "make check" (or runall.sh) to run all the system tests.  This
# script will just run those tests that require that each of their nameservers
# is the only one running on an IP address.
#

SYSTEMTESTTOP=.
. $SYSTEMTESTTOP/conf.sh

for d in $SEQUENTIALDIRS
do
    $SHELL run.sh "${@}" $d 2>&1 | tee $d/test.output
done
