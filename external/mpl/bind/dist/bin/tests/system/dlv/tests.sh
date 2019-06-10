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

echo_i "checking that DNSKEY reference by DLV validates as secure ($n)"
ret=0
$DIG $DIGOPTS child1.utld dnskey @10.53.0.5 > dig.out.ns5.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns5.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that child DNSKEY reference by DLV validates as secure ($n)"
ret=0
$DIG $DIGOPTS grand.child1.utld dnskey @10.53.0.5 > dig.out.ns5.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns5.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that SOA reference by DLV in a DRUZ with DS validates as secure ($n)"
ret=0
$DIG $DIGOPTS child1.druz soa @10.53.0.5 > dig.out.ns5.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns5.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that child SOA reference by DLV in a DRUZ with DS validates as secure ($n)"
ret=0
$DIG $DIGOPTS grand.child1.druz soa @10.53.0.5 > dig.out.ns5.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns5.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
