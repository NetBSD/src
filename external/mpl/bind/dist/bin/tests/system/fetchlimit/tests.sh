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

DIGCMD="$DIG @10.53.0.3 -p ${PORT} +tries=1 +time=1"
RNDCCMD="$RNDC -p ${CONTROLPORT} -s 10.53.0.3 -c ../common/rndc.conf"

burst() {
    num=${3:-20}
    rm -f burst.input.$$
    while [ $num -gt 0 ]; do
        num=`expr $num - 1`
        echo "${num}${1}${2}.lamesub.example A" >> burst.input.$$
    done
    $PERL ../ditch.pl -p ${PORT} -s 10.53.0.3 burst.input.$$
    rm -f burst.input.$$
}

stat() {
    clients=`$RNDCCMD status | grep "recursive clients" | 
            sed 's;.*: \([^/][^/]*\)/.*;\1;'`
    echo_i "clients: $clients"
    [ "$clients" = "" ] && return 1
    [ "$clients" -le $1 ]
}

status=0

echo_i "checking recursing clients are dropped at the per-server limit"
ret=0
# make the server lame and restart
$RNDCCMD flush
touch ans4/norespond
for try in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
    burst a $try
    # fetches-per-server is at 400, but at 20qps against a lame server,
    # we'll reach 200 at the tenth second, and the quota should have been
    # tuned to less than that by then
    stat 200 || ret=1
    [ $ret -eq 1 ] && break
    sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "dumping ADB data"
$RNDCCMD dumpdb -adb
info=`grep '10.53.0.4' ns3/named_dump.db | sed 's/.*\(atr [.0-9]*\).*\(quota [0-9]*\).*/\1 \2/'`
echo_i $info
set -- $info
quota=$5
[ ${5:-200} -lt 200 ] || ret=1

echo_i "checking servfail statistics"
ret=0
rm -f ns3/named.stats
$RNDCCMD stats
for try in 1 2 3 4 5; do
    [ -f ns3/named.stats ] && break
    sleep 1
done
sspill=`grep 'spilled due to server' ns3/named.stats | sed 's/\([0-9][0-9]*\) spilled.*/\1/'`
[ -z "$sspill" ] && sspill=0
fails=`grep 'queries resulted in SERVFAIL' ns3/named.stats | sed 's/\([0-9][0-9]*\) queries.*/\1/'`
[ -z "$fails" ] && fails=0
[ "$fails" -ge "$sspill" ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking lame server recovery"
ret=0
rm -f ans4/norespond
for try in 1 2 3 4 5; do
    burst b $try
    stat 200 || ret=1
    [ $ret -eq 1 ] && break
    sleep 1
done

echo_i "dumping ADB data"
$RNDCCMD dumpdb -adb
info=`grep '10.53.0.4' ns3/named_dump.db | sed 's/.*\(atr [.0-9]*\).*\(quota [0-9]*\).*/\1 \2/'`
echo_i $info
set -- $info
[ ${5:-${quota}} -lt $quota ] || ret=1
quota=$5

for try in 1 2 3 4 5 6 7 8 9 10; do
    burst c $try
    stat 20 || ret=1
    [ $ret -eq 1 ] && break
    sleep 1
done

echo_i "dumping ADB data"
$RNDCCMD dumpdb -adb
info=`grep '10.53.0.4' ns3/named_dump.db | sed 's/.*\(atr [.0-9]*\).*\(quota [0-9]*\).*/\1 \2/'`
echo_i $info
set -- $info
[ ${5:-${quota}} -gt $quota ] || ret=1
quota=$5
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

copy_setports ns3/named2.conf.in ns3/named.conf
$RNDCCMD reconfig 2>&1 | sed 's/^/ns3 /' | cat_i

echo_i "checking lame server clients are dropped at the per-domain limit"
ret=0
fail=0
success=0
touch ans4/norespond
for try in 1 2 3 4 5; do
    burst b $try 300
    $DIGCMD a ${try}.example > dig.out.ns3.$try
    grep "status: NOERROR" dig.out.ns3.$try > /dev/null 2>&1 && \
            success=`expr $success + 1`
    grep "status: SERVFAIL" dig.out.ns3.$try > /dev/null 2>&1 && \
            fail=`expr $fail + 1`
    stat 50 || ret=1
    [ $ret -eq 1 ] && break
    $RNDCCMD recursing 2>&1 | sed 's/^/ns3 /' | cat_i
    sleep 1
done
echo_i "$success successful valid queries, $fail SERVFAIL"
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking drop statistics"
rm -f ns3/named.stats
$RNDCCMD stats
for try in 1 2 3 4 5; do
    [ -f ns3/named.stats ] && break
    sleep 1
done
zspill=`grep 'spilled due to zone' ns3/named.stats | sed 's/\([0-9][0-9]*\) spilled.*/\1/'`
[ -z "$zspill" ] && zspill=0
drops=`grep 'queries dropped' ns3/named.stats | sed 's/\([0-9][0-9]*\) queries.*/\1/'`
[ -z "$drops" ] && drops=0
[ "$drops" -ge "$zspill" ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

copy_setports ns3/named3.conf.in ns3/named.conf
$RNDCCMD reconfig 2>&1 | sed 's/^/ns3 /' | cat_i

echo_i "checking lame server clients are dropped at the soft limit"
ret=0
fail=0
exceeded=0
success=0
touch ans4/norespond
for try in 1 2 3 4 5; do
    burst b $try 400
    $DIG @10.53.0.3 -p ${PORT}  a ${try}.example > dig.out.ns3.$try
    stat 360 || exceeded=`expr $exceeded + 1`
    grep "status: NOERROR" dig.out.ns3.$try > /dev/null 2>&1 && \
            success=`expr $success + 1`
    grep "status: SERVFAIL" dig.out.ns3.$try > /dev/null 2>&1 && \
            fail=`expr $fail + 1`
    sleep 1
done
echo_i "$success successful valid queries (expected 5)"
[ "$success" -eq 5 ] || { echo_i "failed"; ret=1; }
echo_i "$fail SERVFAIL responses (expected 0)"
[ "$fail" -eq 0 ] || { echo_i "failed"; ret=1; }
echo_i "clients count exceeded 360 on $exceeded trials (expected 0)"
[ "$exceeded" -eq 0 ] || { echo_i "failed"; ret=1; }
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
