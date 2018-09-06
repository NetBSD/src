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

n=`expr $n + 1`
echo_i "Check that domain names that are too big when applying a search list entry are handled cleanly ($n)"
ret=0
l=012345678901234567890123456789012345678901234567890123456789012
t=0123456789012345678901234567890123456789012345678901234567890
d=$l.$l.$l.$t
$NSLOOKUP -port=${PORT} -domain=$d -type=soa example 10.53.0.1 > nslookup.out${n} || ret=1
grep "origin = ns1.example" nslookup.out${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "Check A only lookup"
ret=0
$NSLOOKUP -port=${PORT} a-only.example.net 10.53.0.1 > nslookup.out${n} || ret=1
lines=`grep "Server:" nslookup.out${n} | wc -l`
test $lines = 1 || ret=1
lines=`grep a-only.example.net nslookup.out${n} | wc -l`
test $lines = 1 || ret=1
grep "1.2.3.4" nslookup.out${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "Check AAAA only lookup"
ret=0
$NSLOOKUP -port=${PORT} aaaa-only.example.net 10.53.0.1 > nslookup.out${n} || ret=1
lines=`grep "Server:" nslookup.out${n} | wc -l`
test $lines = 1 || ret=1
lines=`grep aaaa-only.example.net nslookup.out${n} | wc -l`
test $lines = 1 || ret=1
grep "2001::ffff" nslookup.out${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "Check dual A + AAAA lookup"
ret=0
$NSLOOKUP -port=${PORT} dual.example.net 10.53.0.1 > nslookup.out${n} || ret=1
lines=`grep "Server:" nslookup.out${n} | wc -l`
test $lines = 1 || ret=1
lines=`grep dual.example.net nslookup.out${n} | wc -l`
test $lines = 2 || ret=1
grep "1.2.3.4" nslookup.out${n} > /dev/null || ret=1
grep "2001::ffff" nslookup.out${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "Check CNAME to A only lookup"
ret=0
$NSLOOKUP -port=${PORT} cname-a-only.example.net 10.53.0.1 > nslookup.out${n} || ret=1
lines=`grep "Server:" nslookup.out${n} | wc -l`
test $lines = 1 || ret=1
lines=`grep "canonical name" nslookup.out${n} | wc -l`
test $lines = 1 || ret=1
lines=`grep a-only.example.net nslookup.out${n} | grep -v "canonical name" | wc -l`
test $lines = 1 || ret=1
grep "1.2.3.4" nslookup.out${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "Check CNAME to AAAA only lookup"
ret=0
$NSLOOKUP -port=${PORT} cname-aaaa-only.example.net 10.53.0.1 > nslookup.out${n} || ret=1
lines=`grep "Server:" nslookup.out${n} | wc -l`
test $lines = 1 || ret=1
lines=`grep "canonical name" nslookup.out${n} | wc -l`
test $lines = 1 || ret=1
lines=`grep aaaa-only.example.net nslookup.out${n} | grep -v "canonical name" |wc -l`
test $lines = 1 || ret=1
grep "2001::ffff" nslookup.out${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "Check CNAME to dual A + AAAA lookup"
ret=0
$NSLOOKUP -port=${PORT} cname-dual.example.net 10.53.0.1 > nslookup.out${n} || ret=1
lines=`grep "Server:" nslookup.out${n} | wc -l`
test $lines = 1 || ret=1
lines=`grep "canonical name" nslookup.out${n} | wc -l`
test $lines = 1 || ret=1
lines=`grep dual.example.net nslookup.out${n} | grep -v "canonical name" | wc -l`
test $lines = 2 || ret=1
grep "1.2.3.4" nslookup.out${n} > /dev/null || ret=1
grep "2001::ffff" nslookup.out${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
