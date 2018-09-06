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

DIGOPTS="+norec -p ${PORT}"

status=0
n=0
zone=.

n=`expr $n + 1`
echo_i "check +edns=100 sets version 100 ($n)"
ret=0 reason=
$DIG $DIGOPTS  @10.53.0.1 +qr +edns=100 soa $zone > dig.out$n
grep "EDNS: version: 100," dig.out$n > /dev/null || { ret=1; reason="version"; }
if [ $ret != 0 ]; then echo_i "failed $reason"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
ret=0 reason=
echo_i "check +ednsopt=100 adds option 100 ($n)"
$DIG $DIGOPTS  @10.53.0.1 +qr +ednsopt=100 soa $zone > dig.out$n
grep "; OPT=100" dig.out$n > /dev/null || { ret=1; reason="option"; }
if [ $ret != 0 ]; then echo_i "failed $reason"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check +ednsflags=0x80 sets flags to 0x0080 ($n)"
ret=0 reason=
$DIG $DIGOPTS  @10.53.0.1 +qr +ednsflags=0x80 soa $zone > dig.out$n
grep "MBZ: 0x0080," dig.out$n > /dev/null || { ret=1; reason="flags"; }
if [ $ret != 0 ]; then echo_i "failed $reason"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "Unknown EDNS version ($n)"
ret=0 reason=
$DIG $DIGOPTS  @10.53.0.1 +edns=100 +noednsnegotiation soa $zone > dig.out$n
grep "status: BADVERS," dig.out$n > /dev/null || { ret=1; reason="status"; }
grep "EDNS: version: 0," dig.out$n > /dev/null || { ret=1; reason="version"; }
grep "IN.SOA." dig.out$n > /dev/null && { ret=1; reaons="soa"; }
if [ $ret != 0 ]; then echo_i "failed $reason"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "Unknown EDNS option ($n)"
ret=0 reason=
$DIG $DIGOPTS  @10.53.0.1 +ednsopt=100 soa $zone > dig.out$n
grep "status: NOERROR," dig.out$n > /dev/null || { ret=1; reason="status"; }
grep "EDNS: version: 0," dig.out$n > /dev/null || { ret=1; reason="version"; }
grep "; OPT=100" dig.out$n > /dev/null && { ret=1; reason="option"; }
grep "IN.SOA." dig.out$n > /dev/null || { ret=1; reason="nosoa"; }
if [ $ret != 0 ]; then echo_i "failed $reason"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "Unknown EDNS version + option ($n)"
ret=0 reason=
$DIG $DIGOPTS  @10.53.0.1 +edns=100 +noednsneg +ednsopt=100 soa $zone > dig.out$n
grep "status: BADVERS," dig.out$n > /dev/null || { ret=1; reason="status"; }
grep "EDNS: version: 0," dig.out$n > /dev/null || { ret=1; reason="version"; }
grep "; OPT=100" dig.out$n > /dev/null && { ret=1; reason="option"; }
grep "IN.SOA." dig.out$n > /dev/null &&  { ret=1; reason="soa"; }
if [ $ret != 0 ]; then echo_i "failed: $reason"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "Unknown EDNS flag ($n)"
ret=0 reason=
$DIG $DIGOPTS  @10.53.0.1 +ednsflags=0x80 soa $zone > dig.out$n
grep "status: NOERROR," dig.out$n > /dev/null || { ret=1; reason="status"; }
grep "EDNS: version: 0," dig.out$n > /dev/null || { ret=1; reason="version"; }
grep "EDNS:.*MBZ" dig.out$n > /dev/null > /dev/null && { ret=1; reason="mbz"; }
grep ".IN.SOA." dig.out$n > /dev/null || { ret=1; reason="nosoa"; }
if [ $ret != 0 ]; then echo_i "failed $reason"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "Unknown EDNS version + flag ($n)"
ret=0 reason=
$DIG $DIGOPTS  @10.53.0.1 +edns=100 +noednsneg +ednsflags=0x80 soa $zone > dig.out$n
grep "status: BADVERS," dig.out$n > /dev/null || { ret=1; reason="status"; }
grep "EDNS: version: 0," dig.out$n > /dev/null || { ret=1; reason="version"; }
grep "EDNS:.*MBZ" dig.out$n > /dev/null > /dev/null && { ret=1; reason="mbz"; }
grep "IN.SOA." dig.out$n > /dev/null && { ret=1; reason="soa"; }
if [ $ret != 0 ]; then echo_i "failed $reason"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "DiG's EDNS negotiation ($n)"
ret=0 reason=
$DIG $DIGOPTS  @10.53.0.1 +edns=100 soa $zone > dig.out$n
grep "status: NOERROR," dig.out$n > /dev/null || { ret=1; reason="status"; }
grep "EDNS: version: 0," dig.out$n > /dev/null || { ret=1; reason="version"; }
grep "IN.SOA." dig.out$n > /dev/null || { ret=1; reason="soa"; }
if [ $ret != 0 ]; then echo_i "failed $reason"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
