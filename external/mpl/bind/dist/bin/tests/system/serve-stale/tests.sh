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

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

RNDCCMD="$RNDC -c ../common/rndc.conf -p ${CONTROLPORT} -s"
DIG="$DIG +time=11"

max_stale_ttl=$(sed -ne 's,^[[:space:]]*max-stale-ttl \([[:digit:]]*\).*,\1,p' $TOP_SRCDIR/bin/named/config.c)
stale_answer_ttl=$(sed -ne 's,^[[:space:]]*stale-answer-ttl \([[:digit:]]*\).*,\1,p' $TOP_SRCDIR/bin/named/config.c)

status=0
n=0

#
# First test server with serve-stale options set.
#
echo_i "test server with serve-stale options set"

n=$((n+1))
echo_i "prime cache longttl.example TXT ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 longttl.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache data.example TXT ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache othertype.example CAA ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 othertype.example CAA > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache nodata.example TXT ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nodata.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache nxdomain.example TXT ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nxdomain.example TXT > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "verify prime cache statistics ($n)"
ret=0
rm -f ns1/named.stats
$RNDCCMD 10.53.0.1 stats > /dev/null 2>&1
[ -f ns1/named.stats ] || ret=1
cp ns1/named.stats ns1/named.stats.$n
# Check first 10 lines of Cache DB statistics.  After prime queries, we expect
# two active TXT, one active Others, one nxrrset TXT, and one NXDOMAIN.
grep -A 10 "++ Cache DB RRsets ++" ns1/named.stats.$n > ns1/named.stats.$n.cachedb || ret=1
grep "1 Others" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "2 TXT" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 !TXT" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 NXDOMAIN" ns1/named.stats.$n.cachedb > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "disable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt disable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"0\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: on (stale-answer-ttl=4 max-stale-ttl=3600 stale-refresh-time=30)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

sleep 2

# Run rndc dumpdb, test whether the stale data has correct comment printed.
# The max-stale-ttl is 3600 seconds, so the comment should say the data is
# stale for somewhere between 3500-3599 seconds.
echo_i "check rndc dump stale data.example ($n)"
rndc_dumpdb ns1 || ret=1
awk '/; stale/ { x=$0; getline; print x, $0}' ns1/named_dump.db.test$n |
    grep "; stale data\.example.*3[56]...*TXT.*A text record with a 2 second ttl" > /dev/null 2>&1 || ret=1
# Also make sure the not expired data does not have a stale comment.
awk '/; answer/ { x=$0; getline; print x, $0}' ns1/named_dump.db.test$n |
    grep "; answer longttl\.example.*[56]...*TXT.*A text record with a 600 second ttl" > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "sending queries for tests $((n+1))-$((n+4))..."
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$((n+1)) &
$DIG -p ${PORT} @10.53.0.1 longttl.example TXT > dig.out.test$((n+2)) &
$DIG -p ${PORT} @10.53.0.1 othertype.example CAA > dig.out.test$((n+3)) &
$DIG -p ${PORT} @10.53.0.1 nodata.example TXT > dig.out.test$((n+4)) &
$DIG -p ${PORT} @10.53.0.1 nxdomain.example TXT > dig.out.test$((n+5))

wait

n=$((n+1))
echo_i "check stale data.example TXT ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*4.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check non-stale longttl.example TXT ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "longttl\.example\..*59[0-9].*IN.*TXT.*A text record with a 600 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check stale othertype.example CAA ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "othertype\.example\..*4.*IN.*CAA.*0.*issue" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check stale nodata.example TXT ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*4.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check stale nxdomain.example TXT ($n)"
ret=0
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*4.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "verify stale cache statistics ($n)"
ret=0
rm -f ns1/named.stats
$RNDCCMD 10.53.0.1 stats > /dev/null 2>&1
[ -f ns1/named.stats ] || ret=1
cp ns1/named.stats ns1/named.stats.$n
# Check first 10 lines of Cache DB statistics.  After serve-stale queries, we
# expect one active TXT RRset, one stale TXT, one stale nxrrset TXT, and one
# stale NXDOMAIN.
grep -A 10 "++ Cache DB RRsets ++" ns1/named.stats.$n > ns1/named.stats.$n.cachedb || ret=1
grep "1 TXT" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #Others" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #TXT" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #!TXT" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #NXDOMAIN" ns1/named.stats.$n.cachedb > /dev/null || ret=1
status=$((status+ret))
if [ $ret != 0 ]; then echo_i "failed"; fi

# Test stale-refresh-time when serve-stale is enabled via configuration.
# Steps for testing stale-refresh-time option (default).
# 1. Prime cache data.example txt
# 2. Disable responses from authoritative server.
# 3. Sleep for TTL duration so rrset TTL expires (2 sec)
# 4. Query data.example
# 5. Check if response come from stale rrset (4 sec TTL)
# 6. Enable responses from authoritative server.
# 7. Query data.example
# 8. Check if response come from stale rrset, since the query
#    is within stale-refresh-time window.
n=$((n+1))
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: on (stale-answer-ttl=4 max-stale-ttl=3600 stale-refresh-time=30)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Step 1-3 done above.

# Step 4.
n=$((n+1))
echo_i "sending query for test ($n)"
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$n

# Step 5.
echo_i "check stale data.example TXT (stale-refresh-time) ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*4.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Step 6.
n=$((n+1))
echo_i "enable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt enable > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Step 7.
echo_i "sending query for test $((n+1))"
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$((n+1))

# Step 8.
n=$((n+1))
echo_i "check stale data.example TXT comes from cache (stale-refresh-time) ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*4.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

#
# Test disabling serve-stale via rndc.
#
n=$((n+1))
echo_i "disable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt disable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"0\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "running 'rndc serve-stale off' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale off || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: off (rndc) (stale-answer-ttl=4 max-stale-ttl=3600 stale-refresh-time=30)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "sending queries for tests $((n+1))-$((n+4))..."
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$((n+1)) &
$DIG -p ${PORT} @10.53.0.1 othertype.example CAA > dig.out.test$((n+2)) &
$DIG -p ${PORT} @10.53.0.1 nodata.example TXT > dig.out.test$((n+3)) &
$DIG -p ${PORT} @10.53.0.1 nxdomain.example TXT > dig.out.test$((n+4))

wait

n=$((n+1))
echo_i "check stale data.example TXT (serve-stale off) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check stale othertype.example CAA (serve-stale off) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check stale nodata.example TXT (serve-stale off) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check stale nxdomain.example TXT (serve-stale off) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

#
# Test enabling serve-stale via rndc.
#
n=$((n+1))
echo_i "running 'rndc serve-stale on' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale on || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: on (rndc) (stale-answer-ttl=4 max-stale-ttl=3600 stale-refresh-time=30)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "sending queries for tests $((n+1))-$((n+4))..."
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$((n+1)) &
$DIG -p ${PORT} @10.53.0.1 othertype.example CAA > dig.out.test$((n+2)) &
$DIG -p ${PORT} @10.53.0.1 nodata.example TXT > dig.out.test$((n+3)) &
$DIG -p ${PORT} @10.53.0.1 nxdomain.example TXT > dig.out.test$((n+4))

wait

n=$((n+1))
echo_i "check stale data.example TXT (serve-stale on) ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*4.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check stale othertype.example CAA (serve-stale on) ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "othertype\.example\..*4.*IN.*CAA.*0.*issue" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check stale nodata.example TXT (serve-stale on) ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*4.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check stale nxdomain.example TXT (serve-stale on) ($n)"
ret=0
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*4.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "running 'rndc serve-stale off' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale off || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "running 'rndc serve-stale reset' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale reset || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: on (stale-answer-ttl=4 max-stale-ttl=3600 stale-refresh-time=30)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "sending queries for tests $((n+1))-$((n+4))..."
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$((n+1)) &
$DIG -p ${PORT} @10.53.0.1 othertype.example CAA > dig.out.test$((n+2)) &
$DIG -p ${PORT} @10.53.0.1 nodata.example TXT > dig.out.test$((n+3)) &
$DIG -p ${PORT} @10.53.0.1 nxdomain.example TXT > dig.out.test$((n+4))

wait

n=$((n+1))
echo_i "check stale data.example TXT (serve-stale reset) ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*4.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check stale othertype.example CAA (serve-stale reset) ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "othertype.example\..*4.*IN.*CAA.*0.*issue" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check stale nodata.example TXT (serve-stale reset) ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*4.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check stale nxdomain.example TXT (serve-stale reset) ($n)"
ret=0
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*4.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "running 'rndc serve-stale off' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale off || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: off (rndc) (stale-answer-ttl=4 max-stale-ttl=3600 stale-refresh-time=30)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

#
# Update named.conf.
# Test server with low max-stale-ttl.
#
echo_i "test server with serve-stale options set, low max-stale-ttl"

n=$((n+1))
echo_i "updating ns1/named.conf ($n)"
ret=0
copy_setports ns1/named2.conf.in ns1/named.conf
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "running 'rndc reload' ($n)"
ret=0
rndc_reload ns1 10.53.0.1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: off (rndc) (stale-answer-ttl=3 max-stale-ttl=20 stale-refresh-time=30)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "flush cache, re-enable serve-stale and query again ($n)"
ret=0
$RNDCCMD 10.53.0.1 flushtree example > rndc.out.test$n.1 2>&1 || ret=1
$RNDCCMD 10.53.0.1 serve-stale on > rndc.out.test$n.2 2>&1 || ret=1
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$n
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: on (rndc) (stale-answer-ttl=3 max-stale-ttl=20 stale-refresh-time=30)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "enable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt enable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache longttl.example TXT (low max-stale-ttl) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 longttl.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache data.example TXT (low max-stale-ttl) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache othertype.example CAA (low max-stale-ttl) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 othertype.example CAA > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache nodata.example TXT (low max-stale-ttl) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nodata.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache nxdomain.example TXT (low max-stale-ttl) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nxdomain.example TXT > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Keep track of time so we can access these RRset later, when we expect them
# to become ancient.
t1=`$PERL -e 'print time()'`

n=$((n+1))
echo_i "verify prime cache statistics (low max-stale-ttl) ($n)"
ret=0
rm -f ns1/named.stats
$RNDCCMD 10.53.0.1 stats > /dev/null 2>&1
[ -f ns1/named.stats ] || ret=1
cp ns1/named.stats ns1/named.stats.$n
# Check first 10 lines of Cache DB statistics.  After prime queries, we expect
# two active TXT RRsets, one active Others, one nxrrset TXT, and one NXDOMAIN.
grep -A 10 "++ Cache DB RRsets ++" ns1/named.stats.$n > ns1/named.stats.$n.cachedb || ret=1
grep "2 TXT" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 Others" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 !TXT" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 NXDOMAIN" ns1/named.stats.$n.cachedb > /dev/null || ret=1
status=$((status+ret))
if [ $ret != 0 ]; then echo_i "failed"; fi

n=$((n+1))
echo_i "disable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt disable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"0\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

sleep 2

echo_i "sending queries for tests $((n+1))-$((n+4))..."
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$((n+1)) &
$DIG -p ${PORT} @10.53.0.1 othertype.example CAA > dig.out.test$((n+2)) &
$DIG -p ${PORT} @10.53.0.1 nodata.example TXT > dig.out.test$((n+3)) &
$DIG -p ${PORT} @10.53.0.1 nxdomain.example TXT > dig.out.test$((n+4))

wait

n=$((n+1))
echo_i "check stale data.example TXT (low max-stale-ttl) ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*3.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check stale othertype.example CAA (low max-stale-ttl) ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "othertype\.example\..*3.*IN.*CAA.*0.*issue" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check stale nodata.example TXT (low max-stale-ttl) ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*3.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check stale nxdomain.example TXT (low max-stale-ttl) ($n)"
ret=0
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*3.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "verify stale cache statistics (low max-stale-ttl) ($n)"
ret=0
rm -f ns1/named.stats
$RNDCCMD 10.53.0.1 stats > /dev/null 2>&1
[ -f ns1/named.stats ] || ret=1
cp ns1/named.stats ns1/named.stats.$n
# Check first 10 lines of Cache DB statistics.  After serve-stale queries, we
# expect one active TXT RRset, one stale TXT, one stale nxrrset TXT, and one
# stale NXDOMAIN.
grep -A 10 "++ Cache DB RRsets ++" ns1/named.stats.$n > ns1/named.stats.$n.cachedb || ret=1
grep "1 TXT" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #TXT" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #Others" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #!TXT" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #NXDOMAIN" ns1/named.stats.$n.cachedb > /dev/null || ret=1

status=$((status+ret))
if [ $ret != 0 ]; then echo_i "failed"; fi

# Retrieve max-stale-ttl value.
interval_to_ancient=`grep 'max-stale-ttl' ns1/named2.conf.in  | awk '{ print $2 }' | tr -d ';'`
# We add 2 seconds to it since this is the ttl value of the records being
# tested.
interval_to_ancient=$((interval_to_ancient + 2))
t2=`$PERL -e 'print time()'`
elapsed=$((t2 - t1))

# If elapsed time so far is less than max-stale-ttl + 2 seconds, then we sleep
# enough to ensure that we'll ask for ancient RRsets in the next queries.
if [ $elapsed -lt $interval_to_ancient ]; then
    sleep $((interval_to_ancient - elapsed))
fi

echo_i "sending queries for tests $((n+1))-$((n+4))..."
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$((n+1)) &
$DIG -p ${PORT} @10.53.0.1 othertype.example CAA > dig.out.test$((n+2)) &
$DIG -p ${PORT} @10.53.0.1 nodata.example TXT > dig.out.test$((n+3)) &
$DIG -p ${PORT} @10.53.0.1 nxdomain.example TXT > dig.out.test$((n+4))

wait

n=$((n+1))
echo_i "check ancient data.example TXT (low max-stale-ttl) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check ancient othertype.example CAA (low max-stale-ttl) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check ancient nodata.example TXT (low max-stale-ttl) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check ancient nxdomain.example TXT (low max-stale-ttl) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Test stale-refresh-time when serve-stale is enabled via rndc.
# Steps for testing stale-refresh-time option (default).
# 1. Prime cache data.example txt
# 2. Disable responses from authoritative server.
# 3. Sleep for TTL duration so rrset TTL expires (2 sec)
# 4. Query data.example
# 5. Check if response come from stale rrset (3 sec TTL)
# 6. Enable responses from authoritative server.
# 7. Query data.example
# 8. Check if response come from stale rrset, since the query
#    is within stale-refresh-time window.
n=$((n+1))
echo_i "flush cache, enable responses from authoritative server ($n)"
ret=0
$RNDCCMD 10.53.0.1 flushtree example > rndc.out.test$n.1 2>&1 || ret=1
$DIG -p ${PORT} @10.53.0.2 txt enable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: on (rndc) (stale-answer-ttl=3 max-stale-ttl=20 stale-refresh-time=30)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Step 1.
n=$((n+1))
echo_i "prime cache data.example TXT (stale-refresh-time rndc) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*2.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Step 2.
n=$((n+1))
echo_i "disable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt disable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"0\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Step 3.
sleep 2

# Step 4.
n=$((n+1))
echo_i "sending query for test ($n)"
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$n

# Step 5.
echo_i "check stale data.example TXT (stale-refresh-time rndc) ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*3.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Step 6.
n=$((n+1))
echo_i "enable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt enable > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Step 7.
echo_i "sending query for test $((n+1))"
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$((n+1))

# Step 8.
n=$((n+1))
echo_i "check stale data.example TXT comes from cache (stale-refresh-time rndc) ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*3.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Steps for testing stale-refresh-time option (disabled).
# 1. Prime cache data.example txt
# 2. Disable responses from authoritative server.
# 3. Sleep for TTL duration so rrset TTL expires (2 sec)
# 4. Query data.example
# 5. Check if response come from stale rrset (3 sec TTL)
# 6. Enable responses from authoritative server.
# 7. Query data.example
# 8. Check if response come from stale rrset, since the query
#    is within stale-refresh-time window.
n=$((n+1))
echo_i "updating ns1/named.conf ($n)"
ret=0
copy_setports ns1/named3.conf.in ns1/named.conf
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "running 'rndc reload' ($n)"
ret=0
rndc_reload ns1 10.53.0.1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: on (rndc) (stale-answer-ttl=3 max-stale-ttl=20 stale-refresh-time=0)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "flush cache, enable responses from authoritative server ($n)"
ret=0
$RNDCCMD 10.53.0.1 flushtree example > rndc.out.test$n.1 2>&1 || ret=1
$DIG -p ${PORT} @10.53.0.2 txt enable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Step 1.
n=$((n+1))
echo_i "prime cache data.example TXT (stale-refresh-time disabled) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*2.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Step 2.
n=$((n+1))
echo_i "disable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt disable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"0\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Step 3.
sleep 2

# Step 4.
n=$((n+1))
echo_i "sending query for test ($n)"
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$n

# Step 5.
echo_i "check stale data.example TXT (stale-refresh-time disabled) ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*3.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Step 6.
n=$((n+1))
echo_i "enable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt enable > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Step 7.
echo_i "sending query for test $((n+1))"
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$((n+1))

# Step 8.
n=$((n+1))
echo_i "check data.example TXT comes from authoritative (stale-refresh-time disabled) ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*2.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

#
# Now test server with no serve-stale options set.
#
echo_i "test server with no serve-stale options set"

n=$((n+1))
echo_i "enable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt enable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache longttl.example TXT (max-stale-ttl default) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 longttl.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache data.example TXT (max-stale-ttl default) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*2.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache othertype.example CAA (max-stale-ttl default) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 othertype.example CAA > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "othertype\.example\..*2.*IN.*CAA.*0.*issue" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache nodata.example TXT (max-stale-ttl default) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 nodata.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*2.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache nxdomain.example TXT (max-stale-ttl default) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 nxdomain.example TXT > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*2.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "verify prime cache statistics (max-stale-ttl default) ($n)"
ret=0
rm -f ns3/named.stats
$RNDCCMD 10.53.0.3 stats > /dev/null 2>&1
[ -f ns3/named.stats ] || ret=1
cp ns3/named.stats ns3/named.stats.$n
# Check first 10 lines of Cache DB statistics.  After prime queries, we expect
# two active TXT RRsets, one active Others, one nxrrset TXT, and one NXDOMAIN.
grep -A 10 "++ Cache DB RRsets ++" ns3/named.stats.$n > ns3/named.stats.$n.cachedb || ret=1
grep "2 TXT" ns3/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 Others" ns3/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 !TXT" ns3/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 NXDOMAIN" ns3/named.stats.$n.cachedb > /dev/null || ret=1
status=$((status+ret))
if [ $ret != 0 ]; then echo_i "failed"; fi

n=$((n+1))
echo_i "disable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt disable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"0\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.3 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep "_default: off (stale-answer-ttl=$stale_answer_ttl max-stale-ttl=$max_stale_ttl stale-refresh-time=30)" rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

sleep 2

echo_i "sending queries for tests $((n+1))-$((n+4))..."
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$((n+1)) &
$DIG -p ${PORT} @10.53.0.3 othertype.example CAA > dig.out.test$((n+2)) &
$DIG -p ${PORT} @10.53.0.3 nodata.example TXT > dig.out.test$((n+3)) &
$DIG -p ${PORT} @10.53.0.3 nxdomain.example TXT > dig.out.test$((n+4))

wait

n=$((n+1))
echo_i "check fail of data.example TXT (max-stale-ttl default) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check fail of othertype.example CAA (max-stale-ttl default) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check fail of nodata.example TXT (max-stale-ttl default) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check fail of nxdomain.example TXT (max-stale-ttl default) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "verify stale cache statistics (max-stale-ttl default) ($n)"
ret=0
rm -f ns3/named.stats
$RNDCCMD 10.53.0.3 stats > /dev/null 2>&1
[ -f ns3/named.stats ] || ret=1
cp ns3/named.stats ns3/named.stats.$n
# Check first 10 lines of Cache DB statistics. After last queries, we expect
# one active TXT RRset, one stale TXT, one stale nxrrset TXT, and one stale
# NXDOMAIN.
grep -A 10 "++ Cache DB RRsets ++" ns3/named.stats.$n > ns3/named.stats.$n.cachedb || ret=1
grep "1 TXT" ns3/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #TXT" ns3/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #Others" ns3/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #!TXT" ns3/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #NXDOMAIN" ns3/named.stats.$n.cachedb > /dev/null || ret=1

status=$((status+ret))
if [ $ret != 0 ]; then echo_i "failed"; fi

n=$((n+1))
echo_i "check 'rndc serve-stale on' ($n)"
ret=0
$RNDCCMD 10.53.0.3 serve-stale on > rndc.out.test$n 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.3 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep "_default: on (rndc) (stale-answer-ttl=$stale_answer_ttl max-stale-ttl=$max_stale_ttl stale-refresh-time=30)" rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

sleep 2

# Check that if we don't have stale data for a domain name, we will
# not answer anything until the resolver query timeout.
n=$((n+1))
echo_i "check notincache.example TXT times out (max-stale-ttl default) ($n)"
ret=0
$DIG -p ${PORT} +tries=1 +timeout=3  @10.53.0.3 notfound.example TXT > dig.out.test$n 2>&1
grep "connection timed out" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "sending queries for tests $((n+1))-$((n+4))..."
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$((n+1)) &
$DIG -p ${PORT} @10.53.0.3 othertype.example CAA > dig.out.test$((n+2)) &
$DIG -p ${PORT} @10.53.0.3 nodata.example TXT > dig.out.test$((n+3)) &
$DIG -p ${PORT} @10.53.0.3 nxdomain.example TXT > dig.out.test$((n+4)) &
$DIG -p ${PORT} @10.53.0.3 notfound.example TXT > dig.out.test$((n+5))

wait

n=$((n+1))
echo_i "check data.example TXT (max-stale-ttl default) ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*30.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check othertype.example CAA (max-stale-ttl default) ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "example\..*30.*IN.*CAA.*0.*issue" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check nodata.example TXT (max-stale-ttl default) ($n)"
ret=0
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*30.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check nxdomain.example TXT (max-stale-ttl default) ($n)"
ret=0
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*30.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# The notfound.example check is different than nxdomain.example because
# we didn't send a prime query to add notfound.example to the cache.
n=$((n+1))
echo_i "check notfound.example TXT (max-stale-ttl default) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

#
# Now test server with serve-stale answers disabled.
#
echo_i "test server with serve-stale disabled"

n=$((n+1))
echo_i "enable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt enable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache longttl.example TTL (serve-stale answers disabled) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.4 longttl.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache data.example TTL (serve-stale answers disabled) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.4 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*2.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache othertype.example CAA (serve-stale answers disabled) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.4 othertype.example CAA > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "othertype\.example\..*2.*IN.*CAA.*0.*issue" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache nodata.example TXT (serve-stale answers disabled) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.4 nodata.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*2.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache nxdomain.example TXT (serve-stale answers disabled) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.4 nxdomain.example TXT > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*2.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "verify prime cache statistics (serve-stale answers disabled) ($n)"
ret=0
rm -f ns4/named.stats
$RNDCCMD 10.53.0.4 stats > /dev/null 2>&1
[ -f ns4/named.stats ] || ret=1
cp ns4/named.stats ns4/named.stats.$n
# Check first 10 lines of Cache DB statistics.  After prime queries, we expect
# two active TXT RRsets, one active Others, one nxrrset TXT, and one NXDOMAIN.
grep -A 10 "++ Cache DB RRsets ++" ns4/named.stats.$n > ns4/named.stats.$n.cachedb || ret=1
grep "2 TXT" ns4/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 Others" ns4/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 !TXT" ns4/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 NXDOMAIN" ns4/named.stats.$n.cachedb > /dev/null || ret=1
status=$((status+ret))
if [ $ret != 0 ]; then echo_i "failed"; fi

n=$((n+1))
echo_i "disable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt disable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"0\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.4 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep "_default: off (stale-answer-ttl=$stale_answer_ttl max-stale-ttl=$max_stale_ttl stale-refresh-time=30)" rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

sleep 2

echo_i "sending queries for tests $((n+1))-$((n+4))..."
$DIG -p ${PORT} @10.53.0.4 data.example TXT > dig.out.test$((n+1)) &
$DIG -p ${PORT} @10.53.0.4 othertype.example CAA > dig.out.test$((n+2)) &
$DIG -p ${PORT} @10.53.0.4 nodata.example TXT > dig.out.test$((n+3)) &
$DIG -p ${PORT} @10.53.0.4 nxdomain.example TXT > dig.out.test$((n+4))

wait

n=$((n+1))
echo_i "check fail of data.example TXT (serve-stale answers disabled) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check fail of othertype.example TXT (serve-stale answers disabled) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check fail of nodata.example TXT (serve-stale answers disabled) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check fail of nxdomain.example TXT (serve-stale answers disabled) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "verify stale cache statistics (serve-stale answers disabled) ($n)"
ret=0
rm -f ns4/named.stats
$RNDCCMD 10.53.0.4 stats > /dev/null 2>&1
[ -f ns4/named.stats ] || ret=1
cp ns4/named.stats ns4/named.stats.$n
# Check first 10 lines of Cache DB statistics. After last queries, we expect
# one active TXT RRset, one stale TXT, one stale nxrrset TXT, and one stale
# NXDOMAIN.
grep -A 10 "++ Cache DB RRsets ++" ns4/named.stats.$n > ns4/named.stats.$n.cachedb || ret=1
grep "1 TXT" ns4/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #TXT" ns4/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #Others" ns4/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #!TXT" ns4/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #NXDOMAIN" ns4/named.stats.$n.cachedb > /dev/null || ret=1
status=$((status+ret))
if [ $ret != 0 ]; then echo_i "failed"; fi

# Dump the cache.
n=$((n+1))
echo_i "dump the cache (serve-stale answers disabled) ($n)"
ret=0
rndc_dumpdb ns4 -cache || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "stop ns4"
stop_server --use-rndc --port ${CONTROLPORT} ns4

# Load the cache as if it was five minutes (RBTDB_VIRTUAL) older. Since
# max-stale-ttl defaults to a week, we need to adjust the date by one week and
# five minutes.
LASTWEEK=`TZ=UTC perl -e 'my $now = time();
        my $oneWeekAgo = $now - 604800;
        my $fiveMinutesAgo = $oneWeekAgo - 300;
        my ($s, $m, $h, $d, $mo, $y) = (localtime($fiveMinutesAgo))[0, 1, 2, 3, 4, 5];
        printf("%04d%02d%02d%02d%02d%02d", $y+1900, $mo+1, $d, $h, $m, $s);'`

echo_i "mock the cache date to $LASTWEEK (serve-stale answers disabled) ($n)"
ret=0
sed -E "s/DATE [0-9]{14}/DATE $LASTWEEK/g" ns4/named_dump.db.test$n > ns4/named_dump.db.out || ret=1
cp ns4/named_dump.db.out ns4/named_dump.db
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "start ns4"
start_server --noclean --restart --port ${PORT} ns4

n=$((n+1))
echo_i "verify ancient cache statistics (serve-stale answers disabled) ($n)"
ret=0
rm -f ns4/named.stats
$RNDCCMD 10.53.0.4 stats #> /dev/null 2>&1
[ -f ns4/named.stats ] || ret=1
cp ns4/named.stats ns4/named.stats.$n
# Check first 10 lines of Cache DB statistics. After last queries, we expect
# everything to be removed or scheduled to be removed.
grep -A 10 "++ Cache DB RRsets ++" ns4/named.stats.$n > ns4/named.stats.$n.cachedb || ret=1
grep "#TXT" ns4/named.stats.$n.cachedb > /dev/null && ret=1
grep "#Others" ns4/named.stats.$n.cachedb > /dev/null && ret=1
grep "#!TXT" ns4/named.stats.$n.cachedb > /dev/null && ret=1
grep "#NXDOMAIN" ns4/named.stats.$n.cachedb > /dev/null && ret=1
status=$((status+ret))
if [ $ret != 0 ]; then echo_i "failed"; fi

#
# Test the server with stale-cache disabled.
#
echo_i "test server with serve-stale cache disabled"

n=$((n+1))
echo_i "enable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt enable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache longttl.example TXT (serve-stale cache disabled) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.5 longttl.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache data.example TXT (serve-stale cache disabled) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.5 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*2.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache othertype.example CAA (serve-stale cache disabled) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.5 othertype.example CAA > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "othertype\.example\..*2.*IN.*CAA.*0.*issue" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache nodata.example TXT (serve-stale cache disabled) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.5 nodata.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*2.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache nxdomain.example TXT (serve-stale cache disabled) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.5 nxdomain.example TXT > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*2.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "verify prime cache statistics (serve-stale cache disabled) ($n)"
ret=0
rm -f ns5/named.stats
$RNDCCMD 10.53.0.5 stats > /dev/null 2>&1
[ -f ns5/named.stats ] || ret=1
cp ns5/named.stats ns5/named.stats.$n
# Check first 10 lines of Cache DB statistics.  After serve-stale queries,
# we expect two active TXT RRsets, one active Others, one nxrrset TXT, and
# one NXDOMAIN.
grep -A 10 "++ Cache DB RRsets ++" ns5/named.stats.$n > ns5/named.stats.$n.cachedb || ret=1
grep "2 TXT" ns5/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 Others" ns5/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 !TXT" ns5/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 NXDOMAIN" ns5/named.stats.$n.cachedb > /dev/null || ret=1
status=$((status+ret))
if [ $ret != 0 ]; then echo_i "failed"; fi

n=$((n+1))
echo_i "disable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt disable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"0\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.5 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep "_default: off (not-cached)" rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

sleep 2

echo_i "sending queries for tests $((n+1))-$((n+4))..."
$DIG -p ${PORT} @10.53.0.5 data.example TXT > dig.out.test$((n+1)) &
$DIG -p ${PORT} @10.53.0.5 othertype.example CAA > dig.out.test$((n+2)) &
$DIG -p ${PORT} @10.53.0.5 nodata.example TXT > dig.out.test$((n+3)) &
$DIG -p ${PORT} @10.53.0.5 nxdomain.example TXT > dig.out.test$((n+4))

wait

n=$((n+1))
echo_i "check fail of data.example TXT (serve-stale cache disabled) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check fail of othertype.example CAA (serve-stale cache disabled) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check fail of nodata.example TXT (serve-stale cache disabled) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check fail of nxdomain.example TXT (serve-stale cache disabled) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "verify stale cache statistics (serve-stale cache disabled) ($n)"
ret=0
rm -f ns5/named.stats
$RNDCCMD 10.53.0.5 stats > /dev/null 2>&1
[ -f ns5/named.stats ] || ret=1
cp ns5/named.stats ns5/named.stats.$n
# Check first 10 lines of Cache DB statistics.  After serve-stale queries,
# we expect one active TXT (longttl) and the rest to be expired from cache,
# but since we keep everything for 5 minutes (RBTDB_VIRTUAL) in the cache
# after expiry, they still show up in the stats.
grep -A 10 "++ Cache DB RRsets ++" ns5/named.stats.$n > ns5/named.stats.$n.cachedb || ret=1
grep -F "1 Others" ns5/named.stats.$n.cachedb > /dev/null || ret=1
grep -F "2 TXT" ns5/named.stats.$n.cachedb > /dev/null || ret=1
grep -F "1 !TXT" ns5/named.stats.$n.cachedb > /dev/null || ret=1
grep -F "1 NXDOMAIN" ns5/named.stats.$n.cachedb > /dev/null || ret=1
status=$((status+ret))
if [ $ret != 0 ]; then echo_i "failed"; fi

# Dump the cache.
n=$((n+1))
echo_i "dump the cache (serve-stale cache disabled) ($n)"
ret=0
rndc_dumpdb ns5 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))
# Check that expired records are not dumped.
ret=0
grep "; expired since .* (awaiting cleanup)" ns5/named_dump.db.test$n && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Dump the cache including expired entries.
n=$((n+1))
echo_i "dump the cache including expired entries (serve-stale cache disabled) ($n)"
ret=0
rndc_dumpdb ns5 -expired || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Check that expired records are dumped.
echo_i "check rndc dump expired data.example ($n)"
ret=0
awk '/; expired/ { x=$0; getline; print x, $0}' ns5/named_dump.db.test$n |
    grep "; expired since .* (awaiting cleanup) data\.example\..*A text record with a 2 second ttl" > /dev/null 2>&1 || ret=1
awk '/; expired/ { x=$0; getline; print x, $0}' ns5/named_dump.db.test$n |
    grep "; expired since .* (awaiting cleanup) nodata\.example\." > /dev/null 2>&1 || ret=1
awk '/; expired/ { x=$0; getline; print x, $0}' ns5/named_dump.db.test$n |
    grep "; expired since .* (awaiting cleanup) nxdomain\.example\." > /dev/null 2>&1 || ret=1
awk '/; expired/ { x=$0; getline; print x, $0}' ns5/named_dump.db.test$n |
    grep "; expired since .* (awaiting cleanup) othertype\.example\." > /dev/null 2>&1 || ret=1
# Also make sure the not expired data does not have an expired comment.
awk '/; answer/ { x=$0; getline; print x, $0}' ns5/named_dump.db.test$n |
    grep "; answer longttl\.example.*A text record with a 600 second ttl" > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "stop ns5"
stop_server --use-rndc --port ${CONTROLPORT} ns5

# Load the cache as if it was five minutes (RBTDB_VIRTUAL) older.
cp ns5/named_dump.db.test$n ns5/named_dump.db
FIVEMINUTESAGO=`TZ=UTC perl -e 'my $now = time();
        my $fiveMinutesAgo = 300;
        my ($s, $m, $h, $d, $mo, $y) = (localtime($fiveMinutesAgo))[0, 1, 2, 3, 4, 5];
        printf("%04d%02d%02d%02d%02d%02d", $y+1900, $mo+1, $d, $h, $m, $s);'`

n=$((n+1))
echo_i "mock the cache date to $FIVEMINUTESAGO (serve-stale cache disabled) ($n)"
ret=0
sed -E "s/DATE [0-9]{14}/DATE $FIVEMINUTESAGO/g" ns5/named_dump.db > ns5/named_dump.db.out || ret=1
cp ns5/named_dump.db.out ns5/named_dump.db
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "start ns5"
start_server --noclean --restart --port ${PORT} ns5

n=$((n+1))
echo_i "verify ancient cache statistics (serve-stale cache disabled) ($n)"
ret=0
rm -f ns5/named.stats
$RNDCCMD 10.53.0.5 stats #> /dev/null 2>&1
[ -f ns5/named.stats ] || ret=1
cp ns5/named.stats ns5/named.stats.$n
# Check first 10 lines of Cache DB statistics. After last queries, we expect
# everything to be removed or scheduled to be removed.
grep -A 10 "++ Cache DB RRsets ++" ns5/named.stats.$n > ns5/named.stats.$n.cachedb || ret=1
grep -F "#TXT" ns5/named.stats.$n.cachedb > /dev/null && ret=1
grep -F "#Others" ns5/named.stats.$n.cachedb > /dev/null && ret=1
grep -F "#!TXT" ns5/named.stats.$n.cachedb > /dev/null && ret=1
grep -F "#NXDOMAIN" ns5/named.stats.$n.cachedb > /dev/null && ret=1
status=$((status+ret))
if [ $ret != 0 ]; then echo_i "failed"; fi

################################################
# Test for stale-answer-client-timeout (1.8s). #
################################################
echo_i "test stale-answer-client-timeout (1.8)"

n=$((n+1))
echo_i "updating ns3/named.conf ($n)"
ret=0
copy_setports ns3/named2.conf.in ns3/named.conf
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "restart ns3"
stop_server --use-rndc --port ${CONTROLPORT} ns3
start_server --noclean --restart --port ${PORT} ns3

n=$((n+1))
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.3 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: on (stale-answer-ttl=3 max-stale-ttl=3600 stale-refresh-time=0)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "enable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt enable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache data.example TXT (stale-answer-client-timeout) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache nodata.example TXT (stale-answer-client-timeout) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 nodata.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "disable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt disable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"0\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Allow RRset to become stale.
sleep 2

nextpart ns3/named.run > /dev/null

echo_i "sending queries for tests $((n+1))-$((n+2))..."
t1=`$PERL -e 'print time()'`
$DIG -p ${PORT} +tries=1 +timeout=10  @10.53.0.3 data.example TXT > dig.out.test$((n+1)) &
$DIG -p ${PORT} +tries=1 +timeout=10  @10.53.0.3 nodata.example TXT > dig.out.test$((n+2))
wait
t2=`$PERL -e 'print time()'`

# We configured a long value of 30 seconds for resolver-query-timeout.
# That should give us enough time to receive an stale answer from cache
# after stale-answer-client-timeout timer of 1.8 sec triggers.
n=$((n+1))
echo_i "check stale data.example TXT comes from cache (stale-answer-client-timeout 1.8) ($n)"
ret=0
wait_for_log 5 "data.example client timeout, stale answer used" ns3/named.run || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*3.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
# Configured stale-answer-client-timeout is 1.8s, we allow some extra time
# just in case other tests are taking too much cpu.
[ $((t2 - t1)) -le 10 ] || { echo_i "query took $((t2 - t1))s to resolve.";  ret=1; }
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check stale nodata.example TXT comes from cache (stale-answer-client-timeout 1.8) ($n)"
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*3.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Now query for RRset not in cache. The first query should time out, but once
# we enable the authoritative server, the second query should be able to get a
# response.

nextpart ns3/named.run > /dev/null

echo_i "sending queries for tests $((n+2))-$((n+3))..."
$DIG -p ${PORT} +tries=1 +timeout=3   @10.53.0.3 longttl.example TXT > dig.out.test$((n+2)) &
$DIG -p ${PORT} +tries=1 +timeout=10  @10.53.0.3 longttl.example TXT > dig.out.test$((n+3)) &

# Enable the authoritative name server after stale-answer-client-timeout.
n=$((n+1))
echo_i "enable responses from authoritative server ($n)"
ret=0
sleep 4
$DIG -p ${PORT} @10.53.0.2 txt enable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check not in cache longttl.example TXT times out (stale-answer-client-timeout 1.8) ($n)"
ret=0
wait_for_log 4 "longttl.example client timeout, stale answer unavailable" ns3/named.run || ret=1
check_results() {
    [ -s "$1" ] || return 1
    grep "connection timed out" "$1" > /dev/null || return 1
    return 0
}
retry_quiet 4 check_results dig.out.test$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check not in cache longttl.example TXT comes from authoritative (stale-answer-client-timeout 1.8) ($n)"
ret=0
check_results() {
    [ -s "$1" ] || return 1
    grep "status: NOERROR" "$1" > /dev/null || return 1
    grep "ANSWER: 1," "$1" > /dev/null || return 1
    return 0
}
retry_quiet 8 check_results dig.out.test$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

#############################################
# Test for stale-answer-client-timeout off. #
#############################################
echo_i "test stale-answer-client-timeout (off)"

n=$((n+1))
echo_i "updating ns3/named.conf ($n)"
ret=0
copy_setports ns3/named3.conf.in ns3/named.conf
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "running 'rndc reload' ($n)"
ret=0
rndc_reload ns3 10.53.0.3
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Send a query, auth server is disabled, we will enable it after a while in
# order to receive an answer before resolver-query-timeout expires. Since
# stale-answer-client-timeout is disabled we must receive an answer from
# authoritative server.
echo_i "sending query for test $((n+2))"
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$((n+2)) &
sleep 3

n=$((n+1))
echo_i "enable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt enable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Wait until dig is done.
wait

n=$((n+1))
echo_i "check data.example TXT comes from authoritative server (stale-answer-client-timeout off) ($n)"
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*[12].*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

##############################################################
# Test for stale-answer-client-timeout off and CNAME record. #
##############################################################
echo_i "test stale-answer-client-timeout (0) and CNAME record"

n=$((n+1))
echo_i "prime cache shortttl.cname.example (stale-answer-client-timeout off) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 shortttl.cname.example A > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 2," dig.out.test$n > /dev/null || ret=1
grep "shortttl\.cname\.example\..*1.*IN.*CNAME.*longttl\.target\.example\." dig.out.test$n > /dev/null || ret=1
grep "longttl\.target\.example\..*600.*IN.*A.*10\.53\.0\.2" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Allow RRset to become stale.
sleep 1

n=$((n+1))
echo_i "disable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt disable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"0\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
ret=0
echo_i "check stale shortttl.cname.example comes from cache (stale-answer-client-timeout off) ($n)"
nextpart ns3/named.run > /dev/null
$DIG -p ${PORT} @10.53.0.3 shortttl.cname.example A > dig.out.test$n
wait_for_log 5 "shortttl.cname.example resolver failure, stale answer used" ns3/named.run || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 2," dig.out.test$n > /dev/null || ret=1
grep "shortttl\.cname\.example\..*3.*IN.*CNAME.*longttl\.target\.example\." dig.out.test$n > /dev/null || ret=1
# We can't reliably test the TTL of the longttl.target.example A record.
grep "longttl\.target\.example\..*IN.*A.*10\.53\.0\.2" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "enable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt enable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check server is alive or restart ($n)"
ret=0
$RNDCCMD 10.53.0.3 status > rndc.out.test$n 2>&1 || ret=1
if [ $ret != 0 ]; then
    echo_i "failed"
    echo_i "restart ns3"
    start_server --noclean --restart --port ${PORT} serve-stale ns3
fi
status=$((status+ret))

n=$((n+1))
echo_i "check server is alive or restart ($n)"
ret=0
$RNDCCMD 10.53.0.3 status > rndc.out.test$n 2>&1 || ret=1
if [ $ret != 0 ]; then
    echo_i "failed"
    echo_i "restart ns3"
    start_server --noclean --restart --port ${PORT} serve-stale ns3
fi
status=$((status+ret))

#############################################
# Test for stale-answer-client-timeout 0.   #
#############################################
echo_i "test stale-answer-client-timeout (0)"

n=$((n+1))
echo_i "updating ns3/named.conf ($n)"
ret=0
copy_setports ns3/named4.conf.in ns3/named.conf
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "restart ns3"
stop_server --use-rndc --port ${CONTROLPORT} ns3
start_server --noclean --restart --port ${PORT} ns3

n=$((n+1))
echo_i "prime cache data.example TXT (stale-answer-client-timeout 0)"
ret=0
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache nodata.example TXT (stale-answer-client-timeout 0)"
ret=0
$DIG -p ${PORT} @10.53.0.3 nodata.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "disable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt disable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"0\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Allow RRset to become stale.
sleep 2

n=$((n+1))
ret=0
echo_i "check stale nodata.example TXT comes from cache (stale-answer-client-timeout 0) ($n)"
nextpart ns3/named.run > /dev/null
$DIG -p ${PORT} @10.53.0.3 nodata.example TXT > dig.out.test$n
wait_for_log 5 "nodata.example stale answer used, an attempt to refresh the RRset" ns3/named.run || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*3.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
ret=0
echo_i "check stale data.example TXT comes from cache (stale-answer-client-timeout 0) ($n)"
nextpart ns3/named.run > /dev/null
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$n
wait_for_log 5 "data.example stale answer used, an attempt to refresh the RRset" ns3/named.run || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*3.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "enable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt enable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

wait_for_rrset_refresh() {
	nextpart ns3/named.run | grep 'data.example.*2.*TXT.*"A text record with a 2 second ttl"' > /dev/null && return 0
	return 1
}

# This test ensures that after we get stale data due to
# stale-answer-client-timeout 0, enabling the authoritative server will allow
# the RRset to be updated.
n=$((n+1))
ret=0
echo_i "check stale data.example TXT was refreshed (stale-answer-client-timeout 0) ($n)"
retry_quiet 10 wait_for_rrset_refresh || ret=1
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*[12].*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

wait_for_nodata_refresh() {
	$DIG -p ${PORT} @10.53.0.3 nodata.example TXT > dig.out.test$n
	grep "status: NOERROR" dig.out.test$n > /dev/null || return 1
	grep "ANSWER: 0," dig.out.test$n > /dev/null || return 1
	grep "example\..*[12].*IN.*SOA" dig.out.test$n > /dev/null || return 1
	return 0
}

n=$((n+1))
ret=0
echo_i "check stale nodata.example TXT was refreshed (stale-answer-client-timeout 0) ($n)"
retry_quiet 10 wait_for_nodata_refresh || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

############################################################
# Test for stale-answer-client-timeout 0 and CNAME record. #
############################################################
echo_i "test stale-answer-client-timeout (0) and CNAME record"

n=$((n+1))
echo_i "prime cache cname1.stale.test A (stale-answer-client-timeout 0) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 cname1.stale.test A > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 2," dig.out.test$n > /dev/null || ret=1
grep "cname1\.stale\.test\..*1.*IN.*CNAME.*a1\.stale\.test\." dig.out.test$n > /dev/null || ret=1
grep "a1\.stale\.test\..*1.*IN.*A.*192\.0\.2\.1" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Allow RRset to become stale.
sleep 1

n=$((n+1))
ret=0
echo_i "check stale cname1.stale.test A comes from cache (stale-answer-client-timeout 0) ($n)"
nextpart ns3/named.run > /dev/null
$DIG -p ${PORT} @10.53.0.3 cname1.stale.test A > dig.out.test$n
wait_for_log 5 "cname1.stale.test stale answer used, an attempt to refresh the RRset" ns3/named.run || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 2," dig.out.test$n > /dev/null || ret=1
grep "cname1\.stale\.test\..*3.*IN.*CNAME.*a1\.stale\.test\." dig.out.test$n > /dev/null || ret=1
grep "a1\.stale\.test\..*3.*IN.*A.*192\.0\.2\.1" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check server is alive or restart ($n)"
ret=0
$RNDCCMD 10.53.0.3 status > rndc.out.test$n 2>&1 || ret=1
if [ $ret != 0 ]; then
    echo_i "failed"
    echo_i "restart ns3"
    start_server --noclean --restart --port ${PORT} ns3
fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache cname2.stale.test A (stale-answer-client-timeout 0) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 cname2.stale.test A > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 2," dig.out.test$n > /dev/null || ret=1
grep "cname2\.stale\.test\..*1.*IN.*CNAME.*a2\.stale\.test\." dig.out.test$n > /dev/null || ret=1
grep "a2\.stale\.test\..*300.*IN.*A.*192\.0\.2\.2" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Allow CNAME record in the RRSET to become stale.
sleep 1

n=$((n+1))
ret=0
echo_i "check stale cname2.stale.test A comes from cache (stale-answer-client-timeout 0) ($n)"
nextpart ns3/named.run > /dev/null
$DIG -p ${PORT} @10.53.0.3 cname2.stale.test A > dig.out.test$n
wait_for_log 5 "cname2.stale.test stale answer used, an attempt to refresh the RRset" ns3/named.run || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 2," dig.out.test$n > /dev/null || ret=1
grep "cname2\.stale\.test\..*3.*IN.*CNAME.*a2\.stale\.test\." dig.out.test$n > /dev/null || ret=1
# We can't reliably test the TTL of the a2.stale.test A record.
grep "a2\.stale\.test\..*IN.*A.*192\.0\.2\.2" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check server is alive or restart ($n)"
ret=0
$RNDCCMD 10.53.0.3 status > rndc.out.test$n 2>&1 || ret=1
if [ $ret != 0 ]; then
    echo_i "failed"
    echo_i "restart ns3"
    start_server --noclean --restart --port ${PORT} ns3
fi
status=$((status+ret))

####################################################################
# Test for stale-answer-client-timeout 0 and stale-refresh-time 4. #
####################################################################
echo_i "test stale-answer-client-timeout (0) and stale-refresh-time (4)"

n=$((n+1))
echo_i "updating ns3/named.conf ($n)"
ret=0
copy_setports ns3/named5.conf.in ns3/named.conf
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "running 'rndc reload' ($n)"
ret=0
rndc_reload ns3 10.53.0.3
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "flush cache, enable responses from authoritative server ($n)"
ret=0
$RNDCCMD 10.53.0.3 flushtree example > rndc.out.test$n.1 2>&1 || ret=1
$DIG -p ${PORT} @10.53.0.2 txt enable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "prime cache data.example TXT (stale-answer-client-timeout 0, stale-refresh-time 4) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*2.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Allow RRset to become stale.
sleep 2

n=$((n+1))
echo_i "disable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt disable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"0\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
ret=0
echo_i "check stale data.example TXT comes from cache (stale-answer-client-timeout 0 stale-refresh-time 4) ($n)"
nextpart ns3/named.run > /dev/null
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$n
wait_for_log 5 "data.example stale answer used, an attempt to refresh the RRset" ns3/named.run || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*3.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "enable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt enable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# This test ensures that after we get stale data due to
# stale-answer-client-timeout 0, enabling the authoritative server will allow
# the RRset to be updated.
n=$((n+1))
ret=0
echo_i "check stale data.example TXT was refreshed (stale-answer-client-timeout 0 stale-refresh-time 4) ($n)"
retry_quiet 10 wait_for_rrset_refresh || ret=1
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*[12].*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Allow RRset to become stale.
sleep 2

n=$((n+1))
echo_i "disable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt disable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"0\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
ret=0
echo_i "check stale data.example TXT comes from cache (stale-answer-client-timeout 0 stale-refresh-time 4) ($n)"
nextpart ns3/named.run > /dev/null
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$n
wait_for_log 5 "data.example stale answer used, an attempt to refresh the RRset" ns3/named.run || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*3.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Allow stale-refresh-time to be activated.
n=$((n+1))
ret=0
echo_i "wait until resolver query times out, activating stale-refresh-time"
wait_for_log 15 "data.example resolver failure, stale answer used" ns3/named.run || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
ret=0
echo_i "check stale data.example TXT comes from cache within stale-refresh-time (stale-answer-client-timeout 0 stale-refresh-time 4) ($n)"
nextpart ns3/named.run > /dev/null
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$n
wait_for_log 5 "data.example query within stale refresh time" ns3/named.run || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*3.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "enable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt enable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# We give BIND some time to ensure that after we enable authoritative server,
# this RRset is still not refreshed because it was hit during
# stale-refresh-time window.
sleep 1

n=$((n+1))
ret=0
echo_i "check stale data.example TXT was not refreshed (stale-answer-client-timeout 0 stale-refresh-time 4) ($n)"
nextpart ns3/named.run > /dev/null
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$n
wait_for_log 5 "data.example query within stale refresh time" ns3/named.run || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*3.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# After the refresh-time-window, the RRset will be refreshed.
sleep 4

n=$((n+1))
ret=0
echo_i "check stale data.example TXT comes from cache (stale-answer-client-timeout 0 stale-refresh-time 4) ($n)"
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$n
wait_for_log 5 "data.example stale answer used, an attempt to refresh the RRset" ns3/named.run || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*3.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
ret=0
echo_i "check stale data.example TXT was refreshed (stale-answer-client-timeout 0 stale-refresh-time 4) ($n)"
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*[12].*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

####################################################################
# Test serve-stale's interaction with fetch limits (cache only) #
#################################################################
echo_i "test serve-stale's interaction with fetch-limits (cache only)"

# We update the named configuration to enable fetch-limits. The fetch-limits
# are set to 1, which is ridiciously low, but that is because for this test we
# want to reach the fetch-limits.
n=$((n+1))
echo_i "updating ns3/named.conf ($n)"
ret=0
copy_setports ns3/named6.conf.in ns3/named.conf
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "running 'rndc reload' ($n)"
ret=0
rndc_reload ns3 10.53.0.3
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Disable responses from authoritative server. If we can't resolve the example
# zone, fetch limits will be reached.
n=$((n+1))
echo_i "disable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt disable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"0\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Allow RRset to become stale.
sleep 2

# Turn on serve-stale.
n=$((n+1))
echo_i "running 'rndc serve-stale on' ($n)"
ret=0
$RNDCCMD 10.53.0.3 serve-stale on || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.3 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: on (rndc) (stale-answer-ttl=3 max-stale-ttl=3600 stale-refresh-time=4)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Hit the fetch-limits. We burst the name server with a small batch of queries.
# Only 2 queries are required to hit the fetch-limits. The first query will
# start to resolve, the second one hit the fetch-limits.
burst() {
	num=${1}
	rm -f burst.input.$$
	while [ $num -gt 0 ]; do
		num=`expr $num - 1`
		echo "fetch${num}.example A" >> burst.input.$$
	done
	$PERL ../ditch.pl -p ${PORT} -s 10.53.0.3 burst.input.$$
	rm -f burst.input.$$
}

wait_for_fetchlimits() {
	burst 2
	# We expect a query for nx.example to fail because fetch-limits for
	# the domain 'example.' (and everything below) has been reached.
	$DIG -p ${PORT} +tries=1 +timeout=1 @10.53.0.3 nx.example > dig.out.test$n
	grep "status: SERVFAIL" dig.out.test$n > /dev/null || return 1
}

n=$((n+1))
echo_i "hit fetch limits ($n)"
ret=0
retry_quiet 10 wait_for_fetchlimits || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Expect stale data now (because fetch-limits for the domain 'example.' (and
# everything below) has been reached. But we have a stale RRset for
# 'data.example/TXT' that can be used.
n=$((n+1))
ret=0
echo_i "check stale data.example TXT comes from cache (fetch-limits) ($n)"
nextpart ns3/named.run > /dev/null
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$n
wait_for_log 5 "data.example resolver failure, stale answer used" ns3/named.run || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*3.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# The previous query should not have started the stale-refresh-time window.
n=$((n+1))
ret=0
echo_i "check stale data.example TXT comes from cache again (fetch-limits) ($n)"
nextpart ns3/named.run > /dev/null
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$n
wait_for_log 5 "data.example resolver failure, stale answer used" ns3/named.run || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*3.*IN.*TXT.*A text record with a 2 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

########################################################################
# Test serve-stale's interaction with fetch limits (dual-mode) #
########################################################################
echo_i "test serve-stale's interaction with fetch limits (dual-mode)"

# Update named configuration so that ns3 becomes a recursive resolver which is
# also a secondary server for the root zone.
n=$((n+1))
echo_i "updating ns3/named.conf ($n)"
ret=0
copy_setports ns3/named7.conf.in ns3/named.conf
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "running 'rndc reload' ($n)"
ret=0
rndc_reload ns3 10.53.0.3
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Flush the cache to ensure the example/NS RRset cached during previous tests
# does not override the authoritative delegation found in the root zone.
n=$((n+1))
echo_i "flush cache ($n)"
ret=0
$RNDCCMD 10.53.0.3 flush > rndc.out.test$n 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Query name server with low fetch limits. The authoritative server (ans2) is
# not responding. Sending queries for multiple names in the 'example' zone
# in parallel causes the fetch limit for that zone (set to 1) to be
# reached. This should not trigger a crash.
echo_i "sending queries for tests $((n+1))-$((n+4))..."
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$((n+1)) &
$DIG -p ${PORT} @10.53.0.3 othertype.example CAA > dig.out.test$((n+2)) &
$DIG -p ${PORT} @10.53.0.3 nodata.example TXT > dig.out.test$((n+3)) &
$DIG -p ${PORT} @10.53.0.3 nxdomain.example TXT > dig.out.test$((n+4))

wait

# Expect SERVFAIL for the entries not in cache.
n=$((n+1))
echo_i "check stale data.example TXT (fetch-limits dual-mode) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check stale othertype.example CAA (fetch-limits dual-mode) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check stale nodata.example TXT (fetch-limits dual-mode) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check stale nxdomain.example TXT (fetch-limits dual-mode) ($n)"
ret=0
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "check DNS64 processing of a stale negative answer ($n)"
ret=0
# configure ns3 with dns64
copy_setports ns3/named8.conf.in ns3/named.conf
rndc_reload ns3 10.53.0.3
# flush cache, enable ans2 responses, make sure serve-stale is on
$RNDCCMD 10.53.0.3 flush > rndc.out.test$n.1 2>&1 || ret=1
$DIG -p ${PORT} @10.53.0.2 txt enable > /dev/null
$RNDCCMD 10.53.0.3 serve-stale on > rndc.out.test$n.2 2>&1 || ret=1
# prime the cache with an AAAA NXRRSET response
$DIG -p ${PORT} @10.53.0.3 a-only.example AAAA > dig.out.1.test$n
grep "status: NOERROR" dig.out.1.test$n > /dev/null || ret=1
grep "2001:aaaa" dig.out.1.test$n > /dev/null || ret=1
# disable responses from the auth server
$DIG -p ${PORT} @10.53.0.2 txt disable > /dev/null
# wait two seconds for the previous answer to become stale
sleep 2
# resend the query and wait in the background; we should get a stale answer
$DIG -p ${PORT} @10.53.0.3 a-only.example AAAA > dig.out.2.test$n &
# re-enable queries after a pause, so the server gets a real answer too
sleep 2
$DIG -p ${PORT} @10.53.0.2 txt enable > /dev/null
wait
grep "status: NOERROR" dig.out.2.test$n > /dev/null || ret=1
grep "2001:aaaa" dig.out.2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

###########################################################
# Test serve-stale's interaction with prefetch processing #
###########################################################
echo_i "test serve-stale's interaction with prefetch processing"

# Test case for #2733, ensuring that prefetch queries do not trigger
# a lookup due to stale-answer-client-timeout.
#
# 1. Cache the following records:
#    cname.example 7 IN CNAME target.example.
#    target.example 9 IN A <addr>.
# 2. Let the CNAME RRset expire.
# 3. Query for 'cname.example/A'.
#
# This starts recursion because cname.example/CNAME is expired.
# The authoritative server is up so likely it will respond before
#  stale-answer-client-timeout is triggered.
# The 'target.example/A' RRset is found in cache with a positive value
#  and is eligble for prefetching.
# A prefetch is done for 'target.example/A', our ans2 server will
#  delay the request.
# The 'prefetch_done()' callback should have the right event type
#  (DNS_EVENT_FETCHDONE).

# flush cache
n=$((n+1))
echo_i "flush cache ($n)"
ret=0
$RNDCCMD 10.53.0.3 flushtree example > rndc.out.test$n.1 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# prime the cache with CNAME and A; CNAME expires sooner
n=$((n+1))
echo_i "prime cache cname.example A (stale-answer-client-timeout 1.8) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 cname.example A > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 2," dig.out.test$n > /dev/null || ret=1
grep "cname\.example\..*7.*IN.*CNAME.*target\.example\." dig.out.test$n > /dev/null || ret=1
grep "target\.example\..*9.*IN.*A" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# wait for the CNAME to be stale; A will still be valid and in prefetch window.
# (the longer TTL is needed, otherwise data won't be prefetch-eligible.)
sleep 7

# re-enable auth responses, but with a delay answering the A
n=$((n+1))
echo_i "delay responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt slowdown  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# resend the query and wait in the background; we should get a stale answer
n=$((n+1))
echo_i "check prefetch processing of a stale CNAME target ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 cname.example A > dig.out.test$n &
sleep 2
wait
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 2," dig.out.test$n > /dev/null || ret=1
grep "cname\.example\..*7.*IN.*CNAME.*target\.example\." dig.out.test$n > /dev/null || ret=1
grep "target\.example\..*[1-2].*IN.*A" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
