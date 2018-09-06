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

DIGOPTS="+tcp +noadd +nosea +nostat +nocmd +dnssec -p ${PORT}"

# Check the good. domain

echo_i "checking that validation with enabled digest types works"
ret=0
$DIG $DIGOPTS a.good. @10.53.0.3 a > dig.out.good || ret=1
grep "status: NOERROR" dig.out.good > /dev/null || ret=1
grep "flags:[^;]* ad[ ;]" dig.out.good > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Check the bad. domain

echo_i "checking that validation with no supported digest types and must-be-secure results in SERVFAIL"
ret=0
$DIG $DIGOPTS a.bad. @10.53.0.3 a > dig.out.bad || ret=1
grep "SERVFAIL" dig.out.bad > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that validation with no supported digest algorithms results in insecure"
ret=0
$DIG $DIGOPTS bad. @10.53.0.4 ds > dig.out.ds || ret=1
grep "NOERROR" dig.out.ds > /dev/null || ret=1
grep "flags:[^;]* ad[ ;]" dig.out.ds > /dev/null || ret=1
$DIG $DIGOPTS a.bad. @10.53.0.4 a > dig.out.insecure || ret=1
grep "NOERROR" dig.out.insecure > /dev/null || ret=1
grep "flags:[^;]* ad[ ;]" dig.out.insecure > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
echo_i "exit status: $status"

[ $status -eq 0 ] || exit 1
