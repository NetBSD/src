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

RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

status=0
n=0

#echo_i "check ans.pl server ($n)"
#$DIG -p ${PORT} @10.53.0.2 example NS
#$DIG -p ${PORT} @10.53.0.2 example SOA
#$DIG -p ${PORT} @10.53.0.2 ns.example A
#$DIG -p ${PORT} @10.53.0.2 ns.example AAAA
#$DIG -p ${PORT} @10.53.0.2 txt enable
#$DIG -p ${PORT} @10.53.0.2 txt disable
#$DIG -p ${PORT} @10.53.0.2 ns.example AAAA
#$DIG -p ${PORT} @10.53.0.2 txt enable
#$DIG -p ${PORT} @10.53.0.2 ns.example AAAA
#$DIG -p ${PORT} @10.53.0.2 data.example TXT
#$DIG -p ${PORT} @10.53.0.2 nodata.example TXT
#$DIG -p ${PORT} @10.53.0.2 nxdomain.example TXT

#
# First test server with serve-stale options set.
#
echo_i "test server with serve-stale options set"

n=`expr $n + 1`
echo_i "prime cache longttl.example ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 longttl.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "prime cache data.example ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "prime cache nodata.example ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nodata.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "prime cache nxdomain.example ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nxdomain.example TXT > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "verify prime cache statistics ($n)"
ret=0
rm -f ns1/named.stats
$RNDCCMD 10.53.0.1 stats > /dev/null 2>&1
[ -f ns1/named.stats ] || ret=1
cp ns1/named.stats ns1/named.stats.$n
# Check first 10 lines of Cache DB statistics.  After prime queries, we expect
# two active TXT one nxrrset TXT, and one NXDOMAIN.
grep -A 10 "++ Cache DB RRsets ++" ns1/named.stats.$n > ns1/named.stats.$n.cachedb || ret=1
grep "2 TXT" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 !TXT" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 NXDOMAIN" ns1/named.stats.$n.cachedb > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "disable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt disable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"0\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

sleep 1

n=`expr $n + 1`
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: on (stale-answer-ttl=2 max-stale-ttl=3600)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale data.example ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*2.*IN.*TXT.*A text record with a 1 second ttl" dig.out.test$n > /dev/null || ret=1
# Run rndc dumpdb, test whether the stale data has correct comment printed.
# The max-stale-ttl is 3600 seconds, so the comment should say the data is
# stale for somewhere between 3500-3599 seconds.
rndc_dumpdb ns1 || ret=1
awk '/; stale/ { x=$0; getline; print x, $0}' ns1/named_dump.db.test$n |
    grep "; stale (will be retained for 35.. more seconds) data\.example.*A text record with a 1 second ttl" > /dev/null 2>&1 || ret=1
# Also make sure the not expired data does not have a stale comment.
awk '/; answer/ { x=$0; getline; print x, $0}' ns1/named_dump.db.test$n |
    grep "; answer longttl\.example.*A text record with a 600 second ttl" > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale nodata.example ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nodata.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*2.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale nxdomain.example ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nxdomain.example TXT > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*2.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
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
grep "1 #TXT" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #!TXT" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #NXDOMAIN" ns1/named.stats.$n.cachedb > /dev/null || ret=1
status=`expr $status + $ret`
if [ $ret != 0 ]; then echo_i "failed"; fi

n=`expr $n + 1`
echo_i "running 'rndc serve-stale off' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale off || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: off (rndc) (stale-answer-ttl=2 max-stale-ttl=3600)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale data.example (serve-stale off) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$n
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale nodata.example (serve-stale off) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nodata.example TXT > dig.out.test$n
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale nxdomain.example (serve-stale off) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nxdomain.example TXT > dig.out.test$n
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "running 'rndc serve-stale on' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale on || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: on (rndc) (stale-answer-ttl=2 max-stale-ttl=3600)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale data.example (serve-stale on) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*2.*IN.*TXT.*A text record with a 1 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale nodata.example (serve-stale on) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nodata.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*2.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale nxdomain.example (serve-stale on) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nxdomain.example TXT > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*2.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "running 'rndc serve-stale no' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale no || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: off (rndc) (stale-answer-ttl=2 max-stale-ttl=3600)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale data.example (serve-stale no) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$n
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale nodata.example (serve-stale no) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nodata.example TXT > dig.out.test$n
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale nxdomain.example (serve-stale no) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nxdomain.example TXT > dig.out.test$n
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "running 'rndc serve-stale yes' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale yes || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: on (rndc) (stale-answer-ttl=2 max-stale-ttl=3600)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale data.example (serve-stale yes) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*2.*IN.*TXT.*A text record with a 1 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale nodata.example (serve-stale yes) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nodata.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*2.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale nxdomain.example (serve-stale yes) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nxdomain.example TXT > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*2.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "running 'rndc serve-stale off' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale off || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "running 'rndc serve-stale reset' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale reset || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: on (stale-answer-ttl=2 max-stale-ttl=3600)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale data.example (serve-stale reset) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*2.*IN.*TXT.*A text record with a 1 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale nodata.example (serve-stale reset) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nodata.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*2.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale nxdomain.example (serve-stale reset) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nxdomain.example TXT > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*2.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "running 'rndc serve-stale off' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale off || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: off (rndc) (stale-answer-ttl=2 max-stale-ttl=3600)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

#
# Update named.conf.
# Test server with low max-stale-ttl.
#
echo_i "test server with serve-stale options set, low max-stale-ttl"

n=`expr $n + 1`
echo_i "updating ns1/named.conf ($n)"
ret=0
copy_setports ns1/named2.conf.in ns1/named.conf
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "running 'rndc reload' ($n)"
ret=0
rndc_reload ns1 10.53.0.1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: off (rndc) (stale-answer-ttl=3 max-stale-ttl=35)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "flush cache, re-enable serve-stale and query again ($n)"
ret=0
$RNDCCMD 10.53.0.1 flushtree example > rndc.out.test$n.1 2>&1 || ret=1
$RNDCCMD 10.53.0.1 serve-stale on > rndc.out.test$n.2 2>&1 || ret=1
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$n
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.1 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: on (rndc) (stale-answer-ttl=3 max-stale-ttl=35)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "enable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt enable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

sleep 1

n=`expr $n + 1`
echo_i "prime cache longttl.example (low max-stale-ttl) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 longttl.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "prime cache data.example (low max-stale-ttl) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "prime cache nodata.example (low max-stale-ttl) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nodata.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "prime cache nxdomain.example (low max-stale-ttl) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nxdomain.example TXT > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "verify prime cache statistics (low max-stale-ttl) ($n)"
ret=0
rm -f ns1/named.stats
$RNDCCMD 10.53.0.1 stats > /dev/null 2>&1
[ -f ns1/named.stats ] || ret=1
cp ns1/named.stats ns1/named.stats.$n
# Check first 10 lines of Cache DB statistics.  After prime queries, we expect
# two active TXT RRsets, one nxrrset TXT, and one NXDOMAIN.
grep -A 10 "++ Cache DB RRsets ++" ns1/named.stats.$n > ns1/named.stats.$n.cachedb || ret=1
grep "2 TXT" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 !TXT" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 NXDOMAIN" ns1/named.stats.$n.cachedb > /dev/null || ret=1
status=`expr $status + $ret`
if [ $ret != 0 ]; then echo_i "failed"; fi

n=`expr $n + 1`
echo_i "disable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt disable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"0\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

sleep 1

n=`expr $n + 1`
echo_i "check stale data.example (low max-stale-ttl) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*3.*IN.*TXT.*A text record with a 1 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale nodata.example (low max-stale-ttl) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nodata.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*3.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check stale nxdomain.example (low max-stale-ttl) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nxdomain.example TXT > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*3.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
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
grep "1 #!TXT" ns1/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #NXDOMAIN" ns1/named.stats.$n.cachedb > /dev/null || ret=1
status=`expr $status + $ret`
if [ $ret != 0 ]; then echo_i "failed"; fi

sleep 1

n=`expr $n + 1`
echo_i "check ancient data.example (low max-stale-ttl) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 data.example TXT > dig.out.test$n
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check ancient nodata.example (low max-stale-ttl) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nodata.example TXT > dig.out.test$n
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check ancient nxdomain.example (low max-stale-ttl) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.1 nxdomain.example TXT > dig.out.test$n
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

#
# Now test server with no serve-stale options set.
#
echo_i "test server with no serve-stale options set"

n=`expr $n + 1`
echo_i "enable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt enable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "prime cache longttl.example (max-stale-ttl default) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 longttl.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "prime cache data.example (max-stale-ttl default) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*1.*IN.*TXT.*A text record with a 1 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "prime cache nodata.example (max-stale-ttl default) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 nodata.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*1.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "prime cache nxdomain.example (max-stale-ttl default) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 nxdomain.example TXT > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*1.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "verify prime cache statistics (max-stale-ttl default) ($n)"
ret=0
rm -f ns3/named.stats
$RNDCCMD 10.53.0.3 stats > /dev/null 2>&1
[ -f ns3/named.stats ] || ret=1
cp ns3/named.stats ns3/named.stats.$n
# Check first 10 lines of Cache DB statistics.  After prime queries, we expect
# two active TXT RRsets, one nxrrset TXT, and one NXDOMAIN.
grep -A 10 "++ Cache DB RRsets ++" ns3/named.stats.$n > ns3/named.stats.$n.cachedb || ret=1
grep "2 TXT" ns3/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 !TXT" ns3/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 NXDOMAIN" ns3/named.stats.$n.cachedb > /dev/null || ret=1
status=`expr $status + $ret`
if [ $ret != 0 ]; then echo_i "failed"; fi

n=`expr $n + 1`
echo_i "disable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt disable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"0\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

sleep 1

n=`expr $n + 1`
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.3 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: off (stale-answer-ttl=1 max-stale-ttl=604800)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check fail of data.example (max-stale-ttl default) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$n
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check fail of nodata.example (max-stale-ttl default) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 nodata.example TXT > dig.out.test$n
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check fail of nxdomain.example (max-stale-ttl default) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 nxdomain.example TXT > dig.out.test$n
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "verify stale cache statistics (max-stale-ttl default) ($n)"
ret=0
rm -f ns3/named.stats
$RNDCCMD 10.53.0.3 stats > /dev/null 2>&1
[ -f ns3/named.stats ] || ret=1
cp ns3/named.stats ns3/named.stats.$n
# Check first 10 lines of Cache DB statistics. After last queries, we expect
# one active TXT RRset, one stale TXT, one stale nxrrset TXT, and one
# stale NXDOMAIN.
grep -A 10 "++ Cache DB RRsets ++" ns3/named.stats.$n > ns3/named.stats.$n.cachedb || ret=1
grep "1 TXT" ns3/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #TXT" ns3/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #!TXT" ns3/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #NXDOMAIN" ns3/named.stats.$n.cachedb > /dev/null || ret=1
status=`expr $status + $ret`
if [ $ret != 0 ]; then echo_i "failed"; fi

n=`expr $n + 1`
echo_i "check 'rndc serve-stale on' ($n)"
ret=0
$RNDCCMD 10.53.0.3 serve-stale on > rndc.out.test$n 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.3 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: on (rndc) (stale-answer-ttl=1 max-stale-ttl=604800)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check data.example (max-stale-ttl default) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*1.*IN.*TXT.*A text record with a 1 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check nodata.example (max-stale-ttl default) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 nodata.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*1.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check nxdomain.example (max-stale-ttl default) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.3 nxdomain.example TXT > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*1.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

#
# Now test server with serve-stale disabled.
#
echo_i "test server with serve-stale disabled"

n=`expr $n + 1`
echo_i "enable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt enable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"1\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "prime cache longttl.example (serve-stale disabled) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.4 longttl.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "prime cache data.example (serve-stale disabled) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.4 data.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "data\.example\..*1.*IN.*TXT.*A text record with a 1 second ttl" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "prime cache nodata.example (serve-stale disabled) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.4 nodata.example TXT > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*1.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "prime cache nxdomain.example (serve-stale disabled) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.4 nxdomain.example TXT > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
grep "example\..*1.*IN.*SOA" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "verify prime cache statistics (serve-stale disabled) ($n)"
ret=0
rm -f ns4/named.stats
$RNDCCMD 10.53.0.4 stats > /dev/null 2>&1
[ -f ns4/named.stats ] || ret=1
cp ns4/named.stats ns4/named.stats.$n
# Check first 10 lines of Cache DB statistics.  After prime queries, we expect
# two active TXT RRsets, one nxrrset TXT, and one NXDOMAIN.
grep -A 10 "++ Cache DB RRsets ++" ns4/named.stats.$n > ns4/named.stats.$n.cachedb || ret=1
grep "2 TXT" ns4/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 !TXT" ns4/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 NXDOMAIN" ns4/named.stats.$n.cachedb > /dev/null || ret=1
status=`expr $status + $ret`
if [ $ret != 0 ]; then echo_i "failed"; fi

n=`expr $n + 1`
echo_i "disable responses from authoritative server ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.2 txt disable  > dig.out.test$n
grep "ANSWER: 1," dig.out.test$n > /dev/null || ret=1
grep "TXT.\"0\"" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

sleep 1

n=`expr $n + 1`
echo_i "check 'rndc serve-stale status' ($n)"
ret=0
$RNDCCMD 10.53.0.4 serve-stale status > rndc.out.test$n 2>&1 || ret=1
grep '_default: off (stale-answer-ttl=1 max-stale-ttl=604800)' rndc.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check fail of data.example (serve-stale disabled) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.4 data.example TXT > dig.out.test$n
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check fail of nodata.example (serve-stale disabled) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.4 nodata.example TXT > dig.out.test$n
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check fail of nxdomain.example (serve-stale disabled) ($n)"
ret=0
$DIG -p ${PORT} @10.53.0.4 nxdomain.example TXT > dig.out.test$n
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "verify stale cache statistics (serve-stale disabled) ($n)"
ret=0
rm -f ns4/named.stats
$RNDCCMD 10.53.0.4 stats > /dev/null 2>&1
[ -f ns4/named.stats ] || ret=1
cp ns4/named.stats ns4/named.stats.$n
# Check first 10 lines of Cache DB statistics. After last queries, we expect
# one active TXT RRset, one stale TXT, one stale nxrrset TXT, and one
# stale NXDOMAIN.
grep -A 10 "++ Cache DB RRsets ++" ns4/named.stats.$n > ns4/named.stats.$n.cachedb || ret=1
grep "1 TXT" ns4/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #TXT" ns4/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #!TXT" ns4/named.stats.$n.cachedb > /dev/null || ret=1
grep "1 #NXDOMAIN" ns4/named.stats.$n.cachedb > /dev/null || ret=1
status=`expr $status + $ret`
if [ $ret != 0 ]; then echo_i "failed"; fi

# Dump the cache.
n=`expr $n + 1`
echo_i "dump the cache (serve-stale disabled) ($n)"
ret=0
$RNDCCMD 10.53.0.4 dumpdb -cache > rndc.out.test$n 2>&1 || ret=1
done=0
for i in 0 1 2 3 4 5 6 7 8 9; do
	grep '^; Dump complete$' ns4/named_dump4.db > /dev/null 2>&1 && done=1
	if [ $done != 1 ]; then sleep 1; fi
done
if [ $done != 1 ]; then ret=1; fi
status=`expr $status + $ret`
if [ $ret != 0 ]; then echo_i "failed"; fi

echo_i "stop ns4"
$PERL $SYSTEMTESTTOP/stop.pl --use-rndc --port ${CONTROLPORT} serve-stale ns4

# Load the cache as if it was five minutes (RBTDB_VIRTUAL) older.
# Since max-stale-ttl defaults to a week, we need to adjust the date by
# one week and five minutes.
LASTWEEK=`TZ=UTC perl -e 'my $now = time();
        my $oneWeekAgo = $now - 604800;
        my $fiveMinutesAgo = $oneWeekAgo - 300;
        my ($s, $m, $h, $d, $mo, $y) = (localtime($fiveMinutesAgo))[0, 1, 2, 3, 4, 5];
        printf("%04d%02d%02d%02d%02d%02d", $y+1900, $mo+1, $d, $h, $m, $s);'`

n=`expr $n + 1`
echo_i "mock the cache date to $LASTWEEK (serve-stale disabled) ($n)"
ret=0
sed -E "s/DATE [0-9]{14}/DATE $LASTWEEK/g" ns4/named_dump4.db > ns4/named_dumpdb4.db.out || ret=1
cp ns4/named_dumpdb4.db.out ns4/named_dumpdb4.db
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "start ns4"
$PERL $SYSTEMTESTTOP/start.pl --noclean --restart --port ${PORT} serve-stale ns4

sleep 1

n=`expr $n + 1`
echo_i "verify ancient cache statistics (serve-stale disabled) ($n)"
ret=0
rm -f ns4/named.stats
$RNDCCMD 10.53.0.4 stats #> /dev/null 2>&1
[ -f ns4/named.stats ] || ret=1
cp ns4/named.stats ns4/named.stats.$n
# Check first 10 lines of Cache DB statistics. After last queries, we expect
# everything to be removed or scheduled to be removed.
grep -A 10 "++ Cache DB RRsets ++" ns4/named.stats.$n > ns4/named.stats.$n.cachedb || ret=1
grep "#TXT" ns4/named.stats.$n.cachedb > /dev/null && ret=1
grep "#!TXT" ns4/named.stats.$n.cachedb > /dev/null && ret=1
grep "#NXDOMAIN" ns4/named.stats.$n.cachedb > /dev/null && ret=1
status=`expr $status + $ret`
if [ $ret != 0 ]; then echo_i "failed"; fi

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
