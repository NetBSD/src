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

# Check the example.com. domain

echo_i "checking DNAME at apex works ($n)"
ret=0
$DIG $DIGOPTS +norec foo.example.com. \
	@10.53.0.1 a > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "example.com..*DNAME.*example.net." dig.out.ns1.test$n > /dev/null || ret=1
grep "foo.example.com..*CNAME.*foo.example.net." dig.out.ns1.test$n > /dev/null || ret=1
grep "flags:[^;]* aa[ ;]" dig.out.ns1.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking DLZ IXFR=2010062899 (less than serial) ($n)"
ret=0
$DIG $DIGOPTS ixfr=2010062899 example.com @10.53.0.1 +all > dig.out.ns1.test$n 
grep "example.com..*IN.IXFR" dig.out.ns1.test$n > /dev/null || ret=1
grep "example.com..*10.IN.DNAME.example.net." dig.out.ns1.test$n > /dev/null || ret=1
grep "example.com..*10.IN.NS.example.com." dig.out.ns1.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking DLZ IXFR=2010062900 (equal serial) ($n)"
ret=0
$DIG $DIGOPTS ixfr=2010062900 example.com @10.53.0.1 +all > dig.out.ns1.test$n
grep "example.com..*IN.IXFR" dig.out.ns1.test$n > /dev/null || ret=1
grep "example.com..*10.IN.DNAME.example.net." dig.out.ns1.test$n > /dev/null && ret=1
grep "example.com..*10.IN.NS.example.com." dig.out.ns1.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking DLZ IXFR=2010062901 (greater than serial) ($n)"
ret=0
$DIG $DIGOPTS ixfr=2010062901 example.com @10.53.0.1 +all > dig.out.ns1.test$n
grep "example.com..*IN.IXFR" dig.out.ns1.test$n > /dev/null || ret=1
grep "example.com..*10.IN.DNAME.example.net." dig.out.ns1.test$n > /dev/null && ret=1
grep "example.com..*10.IN.NS.example.com." dig.out.ns1.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking DLZ with a malformed SOA record"
ret=0
$DIG $DIGOPTS broken.com type600 @10.53.0.1 > dig.out.ns1.test$n
grep status: dig.out.ns1.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
