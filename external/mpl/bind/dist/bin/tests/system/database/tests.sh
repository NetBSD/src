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
n=0

rm -f dig.out.*

DIGOPTS="+tcp +noadd +nosea +nostat +nocmd +dnssec -p ${PORT}"
RNDCCMD="$RNDC -s 10.53.0.1 -p ${CONTROLPORT} -c ../common/rndc.conf"

# Check the example. domain

echo_i "checking pre reload zone ($n)"
ret=0
$DIG $DIGOPTS soa database. @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep "hostmaster\.isc\.org" dig.out.ns1.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

copy_setports ns1/named2.conf.in ns1/named.conf
$RNDCCMD reload 2>&1 >/dev/null

echo_i "checking post reload zone ($n)"
ret=1
try=0
while test $try -lt 6
do
	sleep 1
	ret=0
	$DIG $DIGOPTS soa database. @10.53.0.1 > dig.out.ns1.test$n || ret=1
	grep "marka\.isc\.org" dig.out.ns1.test$n > /dev/null || ret=1
	try=`expr $try + 1`
	test $ret -eq 0 && break
done
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
