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

# shellcheck disable=SC2086
. ../conf.sh

status=0
n=0

n=$((n + 1))
echo_i "Check that domain names that are too big when applying a search list entry are handled cleanly ($n)"
ret=0
l=012345678901234567890123456789012345678901234567890123456789012
t=0123456789012345678901234567890123456789012345678901234567890
d=$l.$l.$l.$t
$NSLOOKUP -port=${PORT} -domain=$d -type=soa example 10.53.0.1 2>nslookup.err${n} >nslookup.out${n} || ret=1
lines=$(wc -l <nslookup.err${n})
test $lines -eq 0 || ret=1
grep "origin = ns1.example" nslookup.out${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Check A only lookup ($n)"
ret=0
$NSLOOKUP -port=${PORT} a-only.example.net 10.53.0.1 2>nslookup.err${n} >nslookup.out${n} || ret=1
lines=$(wc -l <nslookup.err${n})
test $lines -eq 0 || ret=1
lines=$(grep -c "Server:" nslookup.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c a-only.example.net nslookup.out${n})
test $lines -eq 1 || ret=1
grep "1.2.3.4" nslookup.out${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# See [GL #4044]
n=$((n + 1))
echo_i "Check A only lookup with a delayed stdin input ($n)"
ret=0
(sleep 6 && echo "server 10.53.0.1" && echo "a-only.example.net.") | $NSLOOKUP -port=${PORT} 2>nslookup.err${n} >nslookup.out${n} || ret=1
lines=$(wc -l <nslookup.err${n})
test $lines -eq 0 || ret=1
lines=$(grep -c "Server:" nslookup.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c a-only.example.net nslookup.out${n})
test $lines -eq 1 || ret=1
grep "1.2.3.4" nslookup.out${n} >/dev/null || ret=1
grep "timed out" nslookup.out${n} >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Check AAAA only lookup ($n)"
ret=0
$NSLOOKUP -port=${PORT} aaaa-only.example.net 10.53.0.1 2>nslookup.err${n} >nslookup.out${n} || ret=1
lines=$(wc -l <nslookup.err${n})
test $lines -eq 0 || ret=1
lines=$(grep -c "Server:" nslookup.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c aaaa-only.example.net nslookup.out${n})
test $lines -eq 1 || ret=1
grep "2001::ffff" nslookup.out${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Check dual A + AAAA lookup ($n)"
ret=0
$NSLOOKUP -port=${PORT} dual.example.net 10.53.0.1 2>nslookup.err${n} >nslookup.out${n} || ret=1
lines=$(wc -l <nslookup.err${n})
test $lines -eq 0 || ret=1
lines=$(grep -c "Server:" nslookup.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c dual.example.net nslookup.out${n})
test $lines -eq 2 || ret=1
grep "1.2.3.4" nslookup.out${n} >/dev/null || ret=1
grep "2001::ffff" nslookup.out${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Check CNAME to A only lookup ($n)"
ret=0
$NSLOOKUP -port=${PORT} cname-a-only.example.net 10.53.0.1 2>nslookup.err${n} >nslookup.out${n} || ret=1
lines=$(wc -l <nslookup.err${n})
test $lines -eq 0 || ret=1
lines=$(grep -c "Server:" nslookup.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c "canonical name" nslookup.out${n})
test $lines -eq 1 || ret=1
lines=$(grep a-only.example.net nslookup.out${n} | grep -cv "canonical name")
test $lines -eq 1 || ret=1
grep "1.2.3.4" nslookup.out${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Check CNAME to AAAA only lookup ($n)"
ret=0
$NSLOOKUP -port=${PORT} cname-aaaa-only.example.net 10.53.0.1 2>nslookup.err${n} >nslookup.out${n} || ret=1
lines=$(wc -l <nslookup.err${n})
test $lines -eq 0 || ret=1
lines=$(grep -c "Server:" nslookup.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c "canonical name" nslookup.out${n})
test $lines -eq 1 || ret=1
lines=$(grep aaaa-only.example.net nslookup.out${n} | grep -cv "canonical name")
test $lines -eq 1 || ret=1
grep "2001::ffff" nslookup.out${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Check CNAME to dual A + AAAA lookup ($n)"
ret=0
$NSLOOKUP -port=${PORT} cname-dual.example.net 10.53.0.1 2>nslookup.err${n} >nslookup.out${n} || ret=1
lines=$(wc -l <nslookup.err${n})
test $lines -eq 0 || ret=1
lines=$(grep -c "Server:" nslookup.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c "canonical name" nslookup.out${n})
test $lines -eq 1 || ret=1
lines=$(grep dual.example.net nslookup.out${n} | grep -cv "canonical name")
test $lines -eq 2 || ret=1
grep "1.2.3.4" nslookup.out${n} >/dev/null || ret=1
grep "2001::ffff" nslookup.out${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Check ANY lookup ($n)"
ret=0
$NSLOOKUP -port=${PORT} -type=ANY example.net 10.53.0.1 2>nslookup.err${n} >nslookup.out${n} || ret=1
lines=$(grep -c 'Address:.10\.53\.0\.1#'"${PORT}" nslookup.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c 'origin = ns1\.example\.net' nslookup.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c 'mail addr = hostmaster\.example\.net' nslookup.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c 'nameserver = ns1\.example\.net.' nslookup.out${n})
test $lines -eq 1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
