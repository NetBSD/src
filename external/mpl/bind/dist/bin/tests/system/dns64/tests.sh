#!/bin/sh

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

set -e

. ../conf.sh

status=0
n=0

rm -f dig.out.*

DIGOPTS="+tcp +noadd +nosea +nostat +nocmd -p ${PORT}"

for conf in conf/good*.conf; do
  echo_i "checking that $conf is accepted ($n)"
  ret=0
  $CHECKCONF "$conf" || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
done

for conf in conf/bad*.conf; do
  echo_i "checking that $conf is rejected ($n)"
  ret=0
  $CHECKCONF "$conf" >/dev/null && ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
done

# Check the example. domain

echo_i "checking non-excluded AAAA lookup works ($n)"
ret=0
$DIG $DIGOPTS aaaa-only.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA lookup works ($n)"
ret=0
$DIG $DIGOPTS excluded-only.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded AAAA and non-mapped A lookup works ($n)"
ret=0
$DIG $DIGOPTS excluded-bad-a.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA and mapped A lookup works ($n)"
ret=0
$DIG $DIGOPTS excluded-good-a.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:aaaa::1.2.3.4" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking default exclude acl ignores mapped A records (all mapped) ($n)"
ret=0
$DIG $DIGOPTS a-and-mapped.example. @10.53.0.2 -b 10.53.0.4 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:bbbb::1.2.3.5" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking default exclude acl ignores mapped A records (some mapped) ($n)"
ret=0
$DIG $DIGOPTS a-and-aaaa-and-mapped.example. @10.53.0.2 -b 10.53.0.4 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee::4" dig.out.ns2.test$n >/dev/null || ret=1
grep "::ffff:1.2.3.4" dig.out.ns2.test$n >/dev/null && ret=1
grep "::ffff:1.2.3.5" dig.out.ns2.test$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking default exclude acl works with AAAA only ($n)"
ret=0
$DIG $DIGOPTS aaaa-only.example. @10.53.0.2 -b 10.53.0.4 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking default exclude acl A only lookup works ($n)"
ret=0
$DIG $DIGOPTS a-only.example. @10.53.0.2 -b 10.53.0.4 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:bbbb::102:305" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking partially excluded only AAAA lookup works ($n)"
ret=0
$DIG $DIGOPTS partially-excluded-only.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::3" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking partially-excluded AAAA and non-mapped A lookup works ($n)"
ret=0
$DIG $DIGOPTS partially-excluded-bad-a.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking partially-excluded only AAAA and mapped A lookup works ($n)"
ret=0
$DIG $DIGOPTS partially-excluded-good-a.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking AAAA only lookup works ($n)"
ret=0
$DIG $DIGOPTS aaaa-only.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A only lookup works ($n)"
ret=0
$DIG $DIGOPTS a-only.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:aaaa::102:305" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A and AAAA lookup works ($n)"
ret=0
$DIG $DIGOPTS a-and-aaaa.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-mapped A lookup works ($n)"
ret=0
$DIG $DIGOPTS a-not-mapped.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking NODATA AAAA lookup works ($n)"
ret=0
$DIG $DIGOPTS mx-only.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-existent AAAA lookup works ($n)"
ret=0
$DIG $DIGOPTS non-existent.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-excluded AAAA via CNAME lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-aaaa-only.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA via CNAME lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-excluded-only.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded AAAA and non-mapped A via CNAME lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-excluded-bad-a.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA and mapped A via CNAME lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-excluded-good-a.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:aaaa::1.2.3.4" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking AAAA only via CNAME lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-aaaa-only.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A only via CNAME lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-a-only.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:aaaa::102:305" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A and AAAA via CNAME lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-a-and-aaaa.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-mapped A via CNAME lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-a-not-mapped.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "CNAME	a-not-mapped.example." dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking NODATA AAAA via CNAME lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-mx-only.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "CNAME	mx-only.example." dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-existent AAAA via CNAME lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-non-existent.example. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Check the example. domain recursive only

echo_i "checking non-excluded AAAA lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS aaaa-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS excluded-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded AAAA and non-mapped A lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS excluded-bad-a.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA and mapped A lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS excluded-good-a.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:bbbb::1.2.3.4" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking partially excluded only AAAA lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS partially-excluded-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::3" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking partially-excluded AAAA and non-mapped A lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS partially-excluded-bad-a.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking partially-excluded only AAAA and mapped A lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS partially-excluded-good-a.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking AAAA only lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS aaaa-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A only lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS a-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:bbbb::102:305" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A and AAAA lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS a-and-aaaa.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-mapped A lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS a-not-mapped.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking NODATA AAAA lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS mx-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-existent AAAA lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS non-existent.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-excluded AAAA via CNAME lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS cname-aaaa-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA via CNAME lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS cname-excluded-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded AAAA and non-mapped A via CNAME lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS cname-excluded-bad-a.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA and mapped A via CNAME lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS cname-excluded-good-a.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 2," dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:bbbb::102:304" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking AAAA only via CNAME lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS cname-aaaa-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A only via CNAME lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS cname-a-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:bbbb::102:305" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A and AAAA via CNAME lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS cname-a-and-aaaa.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-mapped A via CNAME lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS cname-a-not-mapped.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "CNAME	a-not-mapped.example." dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking NODATA AAAA via CNAME lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS cname-mx-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "CNAME	mx-only.example." dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-existent AAAA via CNAME lookup works, recursive only ($n)"
ret=0
$DIG $DIGOPTS cname-non-existent.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Check the example. domain recursive only w/o recursion

echo_i "checking non-excluded AAAA lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec aaaa-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec excluded-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee::3" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded AAAA and non-mapped A lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec excluded-bad-a.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA and mapped A lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec excluded-good-a.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking partially excluded only AAAA lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec partially-excluded-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 2," dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee:" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::3" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking partially-excluded AAAA and non-mapped A lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec partially-excluded-bad-a.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 2," dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee:" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking partially-excluded only AAAA and mapped A lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec partially-excluded-good-a.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 2," dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee:" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking AAAA only lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec aaaa-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A only lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec a-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A and AAAA lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec a-and-aaaa.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-mapped A lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec a-not-mapped.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking NODATA AAAA lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec mx-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-existent AAAA lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec non-existent.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-excluded AAAA via CNAME lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec cname-aaaa-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA via CNAME lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec cname-excluded-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 2," dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee::3" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded AAAA and non-mapped A via CNAME lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec cname-excluded-bad-a.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 2," dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA and mapped A via CNAME lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec cname-excluded-good-a.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 2," dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking AAAA only via CNAME lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec cname-aaaa-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A only via CNAME lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec cname-a-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "CNAME	a-only.example." dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A and AAAA via CNAME lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec cname-a-and-aaaa.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-mapped A via CNAME lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec cname-a-not-mapped.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "CNAME	a-not-mapped.example." dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking NODATA AAAA via CNAME lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec cname-mx-only.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "CNAME	mx-only.example." dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-existent AAAA via CNAME lookup works, recursive only +norec ($n)"
ret=0
$DIG $DIGOPTS +norec cname-non-existent.example. @10.53.0.1 -b 10.53.0.1 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Check the example. domain from non client

echo_i "checking non-excluded AAAA from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS aaaa-only.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS excluded-only.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee::3" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded AAAA and non-mapped A from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS excluded-bad-a.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA and mapped A from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS excluded-good-a.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking AAAA only from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS aaaa-only.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A only from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS a-only.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A and AAAA from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS a-and-aaaa.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-mapped A from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS a-not-mapped.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking NODATA AAAA from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS mx-only.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-existent AAAA from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS non-existent.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-excluded AAAA via CNAME from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-aaaa-only.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA via CNAME from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-excluded-only.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee::3" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded AAAA and non-mapped A via CNAME from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-excluded-bad-a.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA and mapped A via CNAME from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-excluded-good-a.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking AAAA only via CNAME from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-aaaa-only.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A only via CNAME from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-a-only.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A and AAAA via CNAME from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-a-and-aaaa.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-mapped A via CNAME from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-a-not-mapped.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "CNAME	a-not-mapped.example." dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking NODATA AAAA via CNAME from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-mx-only.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "CNAME	mx-only.example." dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-existent AAAA via CNAME from non-client lookup works ($n)"
ret=0
$DIG $DIGOPTS cname-non-existent.example. @10.53.0.2 -b 10.53.0.3 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Check the signed. domain

echo_i "checking non-excluded AAAA lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS aaaa-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS excluded-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded AAAA and non-mapped A lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS excluded-bad-a.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA and mapped A lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS excluded-good-a.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:aaaa::102:304" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking AAAA only lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS aaaa-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A only lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS a-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:aaaa::102:305" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A and AAAA lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS a-and-aaaa.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-mapped A lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS a-not-mapped.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking NODATA AAAA lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS mx-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-existent AAAA lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS non-existent.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-excluded AAAA via CNAME lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS cname-aaaa-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA via CNAME lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS cname-excluded-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded AAAA and non-mapped A via CNAME lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS cname-excluded-bad-a.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA and mapped A via CNAME lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS cname-excluded-good-a.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:aaaa::102:304" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking AAAA only via CNAME lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS cname-aaaa-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A only via CNAME lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS cname-a-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:aaaa::102:305" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A and AAAA via CNAME lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS cname-a-and-aaaa.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-mapped A via CNAME lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS cname-a-not-mapped.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "CNAME	a-not-mapped.signed." dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking NODATA AAAA via CNAME lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS cname-mx-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "CNAME	mx-only.signed." dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-existent AAAA via CNAME lookup is signed zone works ($n)"
ret=0
$DIG $DIGOPTS cname-non-existent.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Check the signed. domain
echo_i "checking non-excluded AAAA lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec aaaa-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec excluded-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee::3" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded AAAA and non-mapped A lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec excluded-bad-a.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA and mapped A lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec excluded-good-a.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking AAAA only lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec aaaa-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A only lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec a-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A and AAAA lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec a-and-aaaa.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-mapped A lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec a-not-mapped.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking NODATA AAAA lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec mx-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 0" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-existent AAAA lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec non-existent.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-excluded AAAA via CNAME lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec cname-aaaa-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA via CNAME lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec cname-excluded-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee::3" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded AAAA and non-mapped A via CNAME lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec cname-excluded-bad-a.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking excluded only AAAA and mapped A via CNAME lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec cname-excluded-good-a.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:eeee::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking AAAA only via CNAME lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec cname-aaaa-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::2" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A only via CNAME lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec cname-a-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 2," dig.out.ns2.test$n >/dev/null || ret=1
grep "2001:aaaa::102:305" dig.out.ns2.test$n >/dev/null && ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking A and AAAA via CNAME lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec cname-a-and-aaaa.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "2001::1" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-mapped A via CNAME lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec cname-a-not-mapped.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 2" dig.out.ns2.test$n >/dev/null || ret=1
grep "CNAME	a-not-mapped.signed." dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking NODATA AAAA via CNAME lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec cname-mx-only.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 2," dig.out.ns2.test$n >/dev/null || ret=1
grep "CNAME	mx-only.signed." dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking non-existent AAAA via CNAME lookup is signed zone works with +dnssec ($n)"
ret=0
$DIG $DIGOPTS +dnssec cname-non-existent.signed. @10.53.0.2 -b 10.53.0.2 aaaa >dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 2," dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking reverse mapping ($n)"
ret=0
$DIG $DIGOPTS -x 2001:aaaa::10.0.0.1 @10.53.0.2 >dig.out.ns2.test$n || ret=1
grep -i "CNAME.1.0.0.10.IN-ADDR.ARPA.$" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

list=$($DIG $DIGOPTS -b 10.53.0.6 @10.53.0.2 +short aaaa a-only.example | sort)
for a in $list; do
  ret=0
  echo_i "checking reverse mapping of $a ($n)"
  $DIG $DIGOPTS -x $a @10.53.0.2 >dig.out.ns2.test$n || ret=1
  grep -i "CNAME.5.3.2.1.IN-ADDR.ARPA." dig.out.ns2.test$n >/dev/null || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
done

rev=$($ARPANAME 2001:aaaa::10.0.0.1)
regex='..\(.*.IP6.ARPA\)'
rev=$(expr "${rev}" : "${regex}")
fin=$(expr "${rev}" : "............${regex}")
while test "${rev}" != "${fin}"; do
  ret=0
  echo_i "checking $rev ($n)"
  $DIG $DIGOPTS $rev ptr @10.53.0.2 >dig.out.ns2.test$n || ret=1
  grep -i "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
  grep -i "ANSWER: 0," dig.out.ns2.test$n >/dev/null || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
  rev=$(expr "${rev}" : "${regex}")
done

echo_i "checking dns64-server and dns64-contact ($n)"
ret=0
$DIG $DIGOPTS soa 0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.a.a.a.a.1.0.0.2.ip6.arpa @10.53.0.2 >dig.out.ns2.test$n || ret=1
grep "SOA.dns64.example.net..hostmaster.example.net." dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking TTL less than 600 from zone ($n)"
ret=0
#expect 500
$DIG $DIGOPTS aaaa ttl-less-than-600.example +rec @10.53.0.1 >dig.out.ns1.test$n || ret=1
grep -i "ttl-less-than-600.example..500.IN.AAAA" dig.out.ns1.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking TTL more than 600 from zone ($n)"
ret=0
#expect 700
$DIG $DIGOPTS aaaa ttl-more-than-600.example +rec @10.53.0.1 >dig.out.ns1.test$n || ret=1
grep -i "ttl-more-than-600.example..700.IN.AAAA" dig.out.ns1.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking TTL less than minimum from zone ($n)"
ret=0
#expect 1100
$DIG $DIGOPTS aaaa ttl-less-than-minimum.example +rec @10.53.0.1 >dig.out.ns1.test$n || ret=1
grep -i "ttl-less-than-minimum.example..1100.IN.AAAA" dig.out.ns1.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking TTL limited to minimum from zone ($n)"
ret=0
#expect 1200
$DIG $DIGOPTS aaaa ttl-more-than-minimum.example +rec @10.53.0.1 >dig.out.ns1.test$n || ret=1
grep -i "ttl-more-than-minimum.example..1200.IN.AAAA" dig.out.ns1.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking TTL less than 600 via cache ($n)"
ret=0
#expect 500
$DIG $DIGOPTS aaaa ttl-less-than-600.example +rec -b 10.53.0.2 @10.53.0.2 >dig.out.ns1.test$n || ret=1
grep -i "ttl-less-than-600.example..500.IN.AAAA" dig.out.ns1.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking TTL more than 600 via cache ($n)"
ret=0
#expect 700
$DIG $DIGOPTS aaaa ttl-more-than-600.example +rec -b 10.53.0.2 @10.53.0.2 >dig.out.ns2.test$n || ret=1
grep -i "ttl-more-than-600.example..700.IN.AAAA" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking TTL less than minimum via cache ($n)"
ret=0
#expect 1100
$DIG $DIGOPTS aaaa ttl-less-than-minimum.example +rec -b 10.53.0.2 @10.53.0.2 >dig.out.ns2.test$n || ret=1
grep -i "ttl-less-than-minimum.example..1100.IN.AAAA" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking TTL limited to minimum via cache ($n)"
ret=0
#expect 1200
$DIG $DIGOPTS aaaa ttl-more-than-minimum.example +rec -b 10.53.0.2 @10.53.0.2 >dig.out.ns2.test$n || ret=1
grep -i "ttl-more-than-minimum.example..1200.IN.AAAA" dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking synthesis of AAAA from RPZ-remapped A ($n)"
ret=0
$DIG $DIGOPTS aaaa rpz.example +rec -b 10.53.0.7 @10.53.0.2 >dig.out.ns2.test$n || ret=1
grep -i 'rpz.example.*IN.AAAA.2001:96::a0a:a0a' dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking 'dig +dns64prefix' ($n)"
$DIG $DIGOPTS +dns64prefix @10.53.0.1 >dig.out.ns1.test$n || ret=1
grep '^2001:bbbb::/96$' dig.out.ns1.test$n >/dev/null || ret=1
test $(wc -l <dig.out.ns1.test$n) -eq 1 || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

cp ns1/named2.conf ns1/named.conf
rndc_reload ns1 10.53.0.1

echo_i "checking 'dig +dns64prefix' with multiple prefixes ($n)"
$DIG $DIGOPTS +dns64prefix @10.53.0.1 >dig.out.ns1.test$n || ret=1
grep '^2001:bbbb::/96$' dig.out.ns1.test$n >/dev/null || ret=1
grep '2001:aaaa::/64' dig.out.ns1.test$n >/dev/null || ret=1
test $(wc -l <dig.out.ns1.test$n) -eq 2 || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

cp ns1/named3.conf ns1/named.conf
rndc_reload ns1 10.53.0.1

echo_i "checking 'dig +dns64prefix' with no prefixes ($n)"
$DIG $DIGOPTS +dns64prefix @10.53.0.1 >dig.out.ns1.test$n || ret=1
test $(wc -l <dig.out.ns1.test$n) -eq 0 || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking synthesis of AAAA from builtin ipv4only.arpa ($n)"
ret=0
$DIG $DIGOPTS aaaa ipv4only.arpa -b 10.53.0.7 @10.53.0.2 >dig.out.ns2.test$n || ret=1
grep -i 'ipv4only.arpa.*IN.AAAA.2001:96::c000:aa' dig.out.ns2.test$n >/dev/null || ret=1
grep -i 'ipv4only.arpa.*IN.AAAA.2001:96::c000:ab' dig.out.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking reverse of dns64 mapped ipv4only.arpa addresses returns ipv4only.arpa ($n)"
ret=0
$DIG $DIGOPTS ptr -x 2001:96::192.0.0.170 -b 10.53.0.7 @10.53.0.2 >dig.out.170.ns2.test$n || ret=1
$DIG $DIGOPTS ptr -x 2001:96::192.0.0.171 -b 10.53.0.7 @10.53.0.2 >dig.out.171.ns2.test$n || ret=1
grep "ip6\.arpa\..*PTR.*ipv4only\.arpa\." dig.out.170.ns2.test$n >/dev/null || ret=1
grep "ip6\.arpa\..*PTR.*ipv4only\.arpa\." dig.out.171.ns2.test$n >/dev/null || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

if $PERL ../testsock6.pl fd92:7065:b8e:fffe::10.53.0.4 2>/dev/null; then
  echo_i "checking resolver-use-dns64 ($n)"
  ret=0
  $DIG $DIGOPTS @10.53.0.3 no-aaaa aaaa >dig.out.ns3.test$n || ret=1
  grep "status: NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
  grep "fd92:7065:b8e:fffe::102:304" dig.out.ns3.test$n >/dev/null || ret=1
  n=$((n + 1))
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
else
  echo_i "skipped use-dns64 test - fd92:7065:b8e:fffe::10.53.0.4 not configured ($n)"
fi

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
