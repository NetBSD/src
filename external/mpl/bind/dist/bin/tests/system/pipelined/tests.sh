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

MDIGOPTS="-p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

status=0

echo_i "check pipelined TCP queries"
ret=0
$PIPEQUERIES -p ${PORT} < input > raw || ret=1
awk '{ print $1 " " $5 }' < raw > output
sort < output > output-sorted
$DIFF ref output-sorted || { ret=1 ; echo_i "diff sorted failed"; }
$DIFF ref output > /dev/null && { ret=1 ; echo_i "diff out of order failed"; }
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# flush resolver so queries will be from others again
$RNDCCMD 10.53.0.4 flush
sleep 1

echo_i "check pipelined TCP queries using mdig"
ret=0
$MDIG $MDIGOPTS +noall +answer +vc -f input -b 10.53.0.4 @10.53.0.4 > raw.mdig
awk '{ print $1 " " $5 }' < raw.mdig > output.mdig
sort < output.mdig > output-sorted.mdig
$DIFF ref output-sorted.mdig || { ret=1 ; echo_i "diff sorted failed"; }
$DIFF ref output.mdig > /dev/null && { ret=1 ; echo_i "diff out of order failed"; }
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check keep-response-order"
ret=0
$PIPEQUERIES -p ${PORT} ++ < inputb > rawb || ret=1
awk '{ print $1 " " $5 }' < rawb > outputb
$DIFF refb outputb || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check keep-response-order using mdig"
ret=0
$MDIG $MDIGOPTS +noall +answer +vc -f inputb -b 10.53.0.7 @10.53.0.4 > rawb.mdig
awk '{ print $1 " " $5 }' < rawb.mdig > outputb.mdig
$DIFF refb outputb.mdig || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check mdig -4 -6"
ret=0
$MDIG $MDIGOPTS -4 -6 -f input @10.53.0.4 > output46.mdig 2>&1 && ret=1
grep "only one of -4 and -6 allowed" output46.mdig > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check mdig -4 with an IPv6 server address"
ret=0
$MDIG $MDIGOPTS -4 -f input @fd92:7065:b8e:ffff::2 > output4.mdig 2>&1 && ret=1
grep "address family not supported" output4.mdig > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
