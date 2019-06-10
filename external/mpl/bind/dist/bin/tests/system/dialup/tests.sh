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

rm -f dig.out.*

DIGOPTS="+norec +tcp +noadd +nosea +nostat +noquest +nocmd -p 5300"

# Check the example. domain

$DIG $DIGOPTS example. @10.53.0.1 soa > dig.out.ns1.test || ret=1
echo "I:checking that first zone transfer worked"
ret=0
try=0
while test $try -lt 120
do
	$DIG $DIGOPTS example. @10.53.0.2 soa > dig.out.ns2.test || ret=1
	if grep SERVFAIL dig.out.ns2.test > /dev/null
	then
		try=`expr $try + 1`
		sleep 1
	else
                digcomp dig.out.ns1.test dig.out.ns2.test || ret=1
		break;
	fi
done
echo "I:try $try"
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking that second zone transfer worked"
ret=0
try=0
while test $try -lt 120
do
	$DIG $DIGOPTS example. @10.53.0.3 soa > dig.out.ns3.test || ret=1
	if grep SERVFAIL dig.out.ns3.test > /dev/null
	then
		try=`expr $try + 1`
		sleep 1
	else
                digcomp dig.out.ns1.test dig.out.ns3.test || ret=1
		break;
	fi
done
echo "I:try $try"
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:exit status: $status"
[ $status -eq 0 ] || exit 1
