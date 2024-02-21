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
echo_i "Check A only lookup ($n)"
ret=0
$HOST -p ${PORT} a-only.example.net 10.53.0.1 2>host.err${n} >host.out${n} || ret=1
lines=$(wc -l <host.err${n})
test $lines -eq 0 || ret=1
lines=$(grep -c "Address:" host.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c a-only.example.net host.out${n})
test $lines -eq 1 || ret=1
grep "1.2.3.4" host.out${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Check AAAA only lookup ($n)"
ret=0
$HOST -p ${PORT} aaaa-only.example.net 10.53.0.1 2>host.err${n} >host.out${n} || ret=1
lines=$(wc -l <host.err${n})
test $lines -eq 0 || ret=1
lines=$(grep -c "Address:" host.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c aaaa-only.example.net host.out${n})
test $lines -eq 1 || ret=1
grep "2001::ffff" host.out${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Check dual A + AAAA lookup ($n)"
ret=0
$HOST -p ${PORT} dual.example.net 10.53.0.1 2>host.err${n} >host.out${n} || ret=1
lines=$(wc -l <host.err${n})
test $lines -eq 0 || ret=1
lines=$(grep -c "Address:" host.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c dual.example.net host.out${n})
test $lines -eq 2 || ret=1
grep "1.2.3.4" host.out${n} >/dev/null || ret=1
grep "2001::ffff" host.out${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Check CNAME to A only lookup ($n)"
ret=0
$HOST -p ${PORT} cname-a-only.example.net 10.53.0.1 2>host.err${n} >host.out${n} || ret=1
lines=$(wc -l <host.err${n})
test $lines -eq 0 || ret=1
lines=$(grep -c "Address:" host.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c "cname-a-only.example.net is an alias for a-only.example.net" host.out${n})
test $lines -eq 1 || ret=1
lines=$(grep a-only.example.net host.out${n} | grep -cv "is an alias for")
test $lines -eq 1 || ret=1
grep "1.2.3.4" host.out${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Check CNAME to AAAA only lookup ($n)"
ret=0
$HOST -p ${PORT} cname-aaaa-only.example.net 10.53.0.1 2>host.err${n} >host.out${n} || ret=1
lines=$(wc -l <host.err${n})
test $lines -eq 0 || ret=1
lines=$(grep -c "Address:" host.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c "cname-aaaa-only.example.net is an alias for aaaa-only.example.net" host.out${n})
test $lines -eq 1 || ret=1
lines=$(grep aaaa-only.example.net host.out${n} | grep -cv "is an alias for")
test $lines -eq 1 || ret=1
grep "2001::ffff" host.out${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Check CNAME to dual A + AAAA lookup ($n)"
ret=0
$HOST -p ${PORT} cname-dual.example.net 10.53.0.1 2>host.err${n} >host.out${n} || ret=1
lines=$(wc -l <host.err${n})
test $lines -eq 0 || ret=1
lines=$(grep -c "Address:" host.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c "cname-dual.example.net is an alias for dual.example.net." host.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c "dual.example.net has address 1.2.3.4" host.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c "dual.example.net has IPv6 address 2001::ffff" host.out${n})
test $lines -eq 1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Check ANY lookup ($n)"
ret=0
$HOST -p ${PORT} -t ANY example.net 10.53.0.1 2>host.err${n} >host.out${n} || ret=1
lines=$(grep -c 'Address:.10\.53\.0\.1#'"${PORT}" host.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c 'example.net has SOA record ns1.example.net. hostmaster.example.net. 1397051952 5 5 1814400 3600' host.out${n})
test $lines -eq 1 || ret=1
lines=$(grep -c 'example.net name server ns1.example.net.' host.out${n})
test $lines -eq 1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
