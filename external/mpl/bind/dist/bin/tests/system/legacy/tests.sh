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

status=0
n=0

n=`expr $n + 1`
echo_i "checking formerr edns server setup ($n)"
ret=0
$DIG $DIGOPTS +edns @10.53.0.8 ednsformerr soa > dig.out.1.test$n || ret=1
grep "status: FORMERR" dig.out.1.test$n > /dev/null || ret=1
$DIG $DIGOPTS +noedns @10.53.0.8 ednsformerr soa > dig.out.2.test$n || ret=1
grep "status: NOERROR" dig.out.2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking recursive lookup to formerr edns server succeeds ($n)"
ret=0
$DIG $DIGOPTS +tcp @10.53.0.1 ednsformerr soa > dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking notimp edns server setup ($n)"
ret=0
$DIG $DIGOPTS +edns @10.53.0.9 ednsnotimp soa > dig.out.1.test$n || ret=1
grep "status: NOTIMP" dig.out.1.test$n > /dev/null || ret=1
$DIG $DIGOPTS +noedns @10.53.0.9 ednsnotimp soa > dig.out.2.test$n || ret=1
grep "status: NOERROR" dig.out.2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking recursive lookup to notimp edns server fails ($n)"
ret=0
$DIG $DIGOPTS +tcp @10.53.0.1 ednsnotimp soa > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking refused edns server setup ($n)"
ret=0
$DIG $DIGOPTS +edns @10.53.0.10 ednsrefused soa > dig.out.1.test$n || ret=1
grep "status: REFUSED" dig.out.1.test$n > /dev/null || ret=1
$DIG $DIGOPTS +noedns @10.53.0.10 ednsrefused soa > dig.out.2.test$n || ret=1
grep "status: NOERROR" dig.out.2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking recursive lookup to refused edns server fails ($n)"
ret=0
$DIG $DIGOPTS +tcp @10.53.0.1 ednsrefused soa > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking drop edns server setup ($n)"
ret=0
$DIG $DIGOPTS +edns @10.53.0.2 dropedns soa > dig.out.1.test$n
grep "connection timed out; no servers could be reached" dig.out.1.test$n > /dev/null || ret=1
$DIG $DIGOPTS +noedns @10.53.0.2 dropedns soa > dig.out.2.test$n || ret=1
grep "status: NOERROR" dig.out.2.test$n > /dev/null || ret=1
grep "EDNS: version:" dig.out.2.test$n > /dev/null && ret=1
$DIG $DIGOPTS +noedns +tcp @10.53.0.2 dropedns soa > dig.out.3.test$n || ret=1
grep "status: NOERROR" dig.out.3.test$n > /dev/null || ret=1
grep "EDNS: version:" dig.out.3.test$n > /dev/null && ret=1
$DIG $DIGOPTS +edns +tcp @10.53.0.2 dropedns soa > dig.out.4.test$n
grep "connection timed out; no servers could be reached" dig.out.4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking recursive lookup to drop edns server fails ($n)"
ret=0
$DIG $DIGOPTS +tcp @10.53.0.1 dropedns soa > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking drop edns + no tcp server setup ($n)"
ret=0
$DIG $DIGOPTS +edns @10.53.0.3 dropedns-notcp soa > dig.out.1.test$n
grep "connection timed out; no servers could be reached" dig.out.1.test$n > /dev/null || ret=1
$DIG $DIGOPTS +noedns +tcp @10.53.0.3 dropedns-notcp soa > dig.out.2.test$n
grep "connection timed out; no servers could be reached" dig.out.2.test$n > /dev/null
$DIG $DIGOPTS +noedns @10.53.0.3 dropedns-notcp soa > dig.out.3.test$n || ret=1
grep "status: NOERROR" dig.out.3.test$n > /dev/null || ret=1
grep "EDNS: version:" dig.out.3.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking recursive lookup to drop edns + no tcp server fails ($n)"
ret=0
$DIG $DIGOPTS +tcp @10.53.0.1 dropedns-notcp soa > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking plain dns server setup ($n)"
ret=0
$DIG $DIGOPTS +edns @10.53.0.4 plain soa > dig.out.1.test$n || ret=1
grep "status: NOERROR" dig.out.1.test$n > /dev/null || ret=1
grep "EDNS: version:" dig.out.1.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking recursive lookup to plain dns server succeeds ($n)"
ret=0
$DIG $DIGOPTS +tcp @10.53.0.1 plain soa > dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking plain dns + no tcp server setup ($n)"
ret=0
$DIG $DIGOPTS +edns @10.53.0.5 plain-notcp soa > dig.out.1.test$n || ret=1
grep "status: NOERROR" dig.out.1.test$n > /dev/null || ret=1
grep "EDNS: version:" dig.out.1.test$n > /dev/null && ret=1
$DIG $DIGOPTS +edns +tcp @10.53.0.5 plain-notcp soa > dig.out.2.test$n
grep "connection timed out; no servers could be reached" dig.out.2.test$n > /dev/null
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking recursive lookup to plain dns + no tcp server succeeds ($n)"
ret=0
$DIG $DIGOPTS +tcp @10.53.0.1 plain-notcp soa > dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "checking edns 512 server setup ($n)"
ret=0
$DIG $DIGOPTS +edns @10.53.0.6 edns512 soa > dig.out.1.test$n || ret=1
grep "status: NOERROR" dig.out.1.test$n > /dev/null || ret=1
$DIG $DIGOPTS +edns +tcp @10.53.0.6 edns512 soa > dig.out.2.test$n || ret=1
grep "status: NOERROR" dig.out.1.test$n > /dev/null || ret=1
$DIG $DIGOPTS +edns @10.53.0.6 txt500.edns512 txt > dig.out.3.test$n
grep "connection timed out; no servers could be reached" dig.out.3.test$n > /dev/null
$DIG $DIGOPTS +edns +bufsize=512 +ignor @10.53.0.6 txt500.edns512 txt > dig.out.4.test$n
grep "status: NOERROR" dig.out.4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking recursive lookup to edns 512 server succeeds ($n)"
ret=0
$DIG $DIGOPTS +tcp @10.53.0.1 txt500.edns512 txt > dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking edns 512 + no tcp server setup ($n)"
ret=0
$DIG $DIGOPTS +noedns @10.53.0.7 edns512-notcp soa > dig.out.1.test$n || ret=1
grep "status: NOERROR" dig.out.1.test$n > /dev/null || ret=1
$DIG $DIGOPTS +noedns +tcp @10.53.0.7 edns512-notcp soa > dig.out.2.test$n
grep "connection timed out; no servers could be reached" dig.out.2.test$n > /dev/null
$DIG $DIGOPTS +edns @10.53.0.7 edns512-notcp soa > dig.out.3.test$n
grep "connection timed out; no servers could be reached" dig.out.3.test$n > /dev/null
$DIG $DIGOPTS +edns +bufsize=512 +ignor @10.53.0.7 edns512-notcp soa > dig.out.4.test$n
grep "status: NOERROR" dig.out.4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking recursive lookup to edns 512 + no tcp server fails ($n)"
ret=0
$DIG $DIGOPTS +tcp @10.53.0.1 edns512-notcp soa > dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

$PERL $SYSTEMTESTTOP/stop.pl --use-rndc --port ${CONTROLPORT} legacy ns1
copy_setports ns1/named2.conf.in ns1/named.conf
$PERL $SYSTEMTESTTOP/start.pl --noclean --restart --port ${PORT} legacy ns1

n=`expr $n + 1`
echo_i "checking recursive lookup to edns 512 + no tcp + trust anchor fails ($n)"
ret=0
$DIG $DIGOPTS +tcp @10.53.0.1 edns512-notcp soa > dig.out.test$n
grep "status: SERVFAIL" dig.out.test$n > /dev/null ||
    grep "connection timed out;" dig.out.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
