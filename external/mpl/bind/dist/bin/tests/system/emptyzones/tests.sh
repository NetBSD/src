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

DIGOPTS="-p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

status=0
n=0

n=`expr $n + 1`
echo_i "check that switching to automatic empty zones works ($n)"
ret=0
$RNDCCMD 10.53.0.1 reload > /dev/null || ret=1
sleep 5

copy_setports ns1/named2.conf.in ns1/named.conf
$RNDCCMD 10.53.0.1 reload > /dev/null || ret=1
sleep 5

$DIG $DIGOPTS +vc version.bind txt ch @10.53.0.1 > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that allow-transfer { none; } works ($n)"
ret=0
$DIG $DIGOPTS axfr 10.in-addr.arpa @10.53.0.1 +all > dig.out.test$n || ret=1
grep "status: REFUSED" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
