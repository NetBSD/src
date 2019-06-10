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
# Run all the virtual time tests.
#

SYSTEMTESTTOP=.
. $SYSTEMTESTTOP/conf.sh

$PERL testsock.pl || {
	echo "I:Network interface aliases not set up.  Skipping tests." >&2;
	echo "R:UNTESTED" >&2;
	echo "E:virtual-time:`date`" >&2;
	exit 0;
}

status=0

for d in $SUBDIRS
do
	sh run.sh $d || status=1
done

exit $status
