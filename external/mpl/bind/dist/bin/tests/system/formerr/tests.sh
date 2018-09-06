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

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

status=0

echo_i "test name too long"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} nametoolong > nametoolong.out
ans=`grep got: nametoolong.out`
if [ "${ans}" != "got: 000080010000000000000000" ];
then
	echo_i "failed"; status=`expr $status + 1`;
fi

echo_i "two questions"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} twoquestions > twoquestions.out
ans=`grep got: twoquestions.out`
if [ "${ans}" != "got: 000080010000000000000000" ];
then
	echo_i "failed"; status=`expr $status + 1`;
fi

# this would be NOERROR if it included a COOKIE option,
# but is a FORMERR without one.
echo_i "empty question section (and no COOKIE option)"
$PERL formerr.pl -a 10.53.0.1 -p ${PORT} noquestions > noquestions.out
ans=`grep got: noquestions.out`
if [ "${ans}" != "got: 000080010000000000000000" ];
then
	echo_i "failed"; status=`expr $status + 1`;
fi

echo_i "exit status: $status"

[ $status -eq 0 ] || exit 1
