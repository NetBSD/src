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

DIGOPTS="-p ${PORT}"
RNDCCMD="$RNDC  -c ../common/rndc.conf -s 10.53.0.2 -p ${CONTROLPORT}"

n=0
status=0

echo_i "checking that dig handles TCP keepalive ($n)"
ret=0
n=`expr $n + 1`
$DIG $DIGOPTS +qr +keepalive foo.example @10.53.0.2 > dig.out.test$n
grep "; TCP KEEPALIVE" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that dig added TCP keepalive ($n)"
ret=0
n=`expr $n + 1`
$RNDCCMD stats
grep "EDNS TCP keepalive option received" ns2/named.stats > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that TCP keepalive is added for TCP responses ($n)"
ret=0
n=`expr $n + 1`
$DIG $DIGOPTS +vc +keepalive foo.example @10.53.0.2 > dig.out.test$n
grep "; TCP KEEPALIVE" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that TCP keepalive requires TCP ($n)"
ret=0
n=`expr $n + 1`
$DIG $DIGOPTS +keepalive foo.example @10.53.0.2 > dig.out.test$n
grep "; TCP KEEPALIVE" dig.out.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking default value ($n)"
ret=0
n=`expr $n + 1`
$DIG $DIGOPTS +vc +keepalive foo.example @10.53.0.3 > dig.out.test$n
grep "; TCP KEEPALIVE: 30.0 secs" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking configured value ($n)"
ret=0
n=`expr $n + 1`
$DIG $DIGOPTS +vc +keepalive foo.example @10.53.0.2 > dig.out.test$n
grep "; TCP KEEPALIVE: 15.0 secs" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking re-configured value ($n)"
ret=0
n=`expr $n + 1`
$RNDCCMD tcp-timeouts 300 300 300 200 > output
diff -b output expected || ret=1
$DIG $DIGOPTS +vc +keepalive foo.example @10.53.0.2 > dig.out.test$n
grep "; TCP KEEPALIVE: 20.0 secs" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking server config entry ($n)"
ret=0
n=`expr $n + 1`
$RNDCCMD stats
oka=`grep  "EDNS TCP keepalive option received" ns2/named.stats | \
    tail -1 | awk '{ print $1}'`
$DIG $DIGOPTS bar.example @10.53.0.3 > dig.out.test$n
$RNDCCMD stats
nka=`grep  "EDNS TCP keepalive option received" ns2/named.stats | \
    tail -1 | awk '{ print $1}'`
#echo oka ':' $oka
#echo nka ':' $nka
if [ "$oka" -eq "$nka" ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
