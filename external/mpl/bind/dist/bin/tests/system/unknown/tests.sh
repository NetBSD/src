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

status=0
n=0

DIGOPTS="-p ${PORT}"

n=$((n+1))
echo_i "querying for various representations of an IN A record ($n)"
for i in 1 2 3 4 5 6 7 8 9 10 11 12
do
	ret=0
	$DIG +short $DIGOPTS @10.53.0.1 a$i.example a in > dig.out.$i.test$n || ret=1
	echo 10.0.0.1 | $DIFF - dig.out.$i.test$n || ret=1
	if [ $ret != 0 ]
	then
		echo_i "#$i failed"
	fi
	status=`expr $status + $ret`
done

n=$((n+1))
echo_i "querying for various representations of an IN TXT record ($n)"
for i in 1 2 3 4 5 6 7
do
	ret=0
	$DIG +short $DIGOPTS @10.53.0.1 txt$i.example txt in > dig.out.$i.test$n || ret=1
	echo '"hello"' | $DIFF - dig.out.$i.test$n || ret=1
	if [ $ret != 0 ]
	then
		echo_i "#$i failed"
	fi
	status=`expr $status + $ret`
done

n=$((n+1))
echo_i "querying for various representations of an IN TYPE123 record ($n)"
for i in 1 2 3
do
	ret=0
	$DIG +short $DIGOPTS @10.53.0.1 unk$i.example type123 in > dig.out.$i.test$n || ret=1
	echo '\# 1 00' | $DIFF - dig.out.$i.test$n || ret=1
	if [ $ret != 0 ]
	then
		echo_i "#$i failed"
	fi
	status=`expr $status + $ret`
done

n=$((n+1))
echo_i "querying for NULL record ($n)"
ret=0
$DIG +short $DIGOPTS @10.53.0.1 null.example null in > dig.out.test$n || ret=1
echo '\# 1 00' | $DIFF - dig.out.test$n || ret=1
[ $ret = 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=$((n+1))
echo_i "querying for empty NULL record ($n)"
ret=0
$DIG +short $DIGOPTS @10.53.0.1 empty.example null in > dig.out.test$n || ret=1
echo '\# 0' | $DIFF - dig.out.test$n || ret=1
[ $ret = 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=$((n+1))
echo_i "querying for various representations of a CLASS10 TYPE1 record ($n)"
for i in 1 2
do
	ret=0
	$DIG +short $DIGOPTS @10.53.0.1 a$i.example a class10 > dig.out.$i.test$n || ret=1
	echo '\# 4 0A000001' | $DIFF - dig.out.$i.test$n || ret=1
	if [ $ret != 0 ]
	then
		echo_i "#$i failed"
	fi
	status=`expr $status + $ret`
done

n=$((n+1))
echo_i "querying for various representations of a CLASS10 TXT record ($n)"
for i in 1 2 3 4
do
	ret=0
	$DIG +short $DIGOPTS @10.53.0.1 txt$i.example txt class10 > dig.out.$i.test$n || ret=1
	echo '"hello"' | $DIFF - dig.out.$i.test$n || ret=1
	if [ $ret != 0 ]
	then
		echo_i "#$i failed"
	fi
	status=`expr $status + $ret`
done

n=$((n+1))
echo_i "querying for various representations of a CLASS10 TYPE123 record ($n)"
for i in 1 2
do
	ret=0
	$DIG +short $DIGOPTS @10.53.0.1 unk$i.example type123 class10 > dig.out.$i.test$n || ret=1
	echo '\# 1 00' | $DIFF - dig.out.$i.test$n || ret=1
	if [ $ret != 0 ]
	then
		echo_i "#$i failed"
	fi
	status=`expr $status + $ret`
done

n=$((n+1))
echo_i "querying for SOAs of zone that should have failed to load ($n)"
for i in 1 2 3 4
do
	ret=0
	$DIG $DIGOPTS @10.53.0.1 broken$i. soa in > dig.out.$i.test$n || ret=1
	grep "SERVFAIL" dig.out.$i.test$n > /dev/null || ret=1
	if [ $ret != 0 ]
	then
		echo_i "#$i failed"
	fi
	status=`expr $status + $ret`
done

n=$((n+1))
echo_i "checking large unknown record loading on primary ($n)"
for try in 0 1 2 3 4 5 6 7 8 9; do
    ret=0
    $DIG $DIGOPTS @10.53.0.1 +tcp +short large.example TYPE45234 > dig.out.$i.test$n || { ret=1 ; echo_i "dig failed" ; }
    $DIFF -s large.out dig.out.$i.test$n > /dev/null || { ret=1 ; echo_i "$DIFF failed"; }
    [ "$ret" -eq 0 ] && break
    sleep 1
done
[ $ret = 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=$((n+1))
echo_i "checking large unknown record loading on secondary ($n)"
for try in 0 1 2 3 4 5 6 7 8 9; do
    ret=0
    $DIG $DIGOPTS @10.53.0.2 +tcp +short large.example TYPE45234 > dig.out.$i.test$n || { ret=1 ; echo_i "dig failed" ; }
    $DIFF -s large.out dig.out.$i.test$n > /dev/null || { ret=1 ; echo_i "$DIFF failed"; }
    [ "$ret" -eq 0 ] && break
    sleep 1
done
[ $ret = 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "stop and restart secondary"
stop_server ns2
start_server --noclean --restart --port ${PORT} ns2

# server may be answering queries before zones are loaded,
# so retry a few times if this query fails
n=$((n+1))
echo_i "checking large unknown record loading on secondary ($n)"
for try in 0 1 2 3 4 5 6 7 8 9; do
    ret=0
    $DIG $DIGOPTS @10.53.0.2 +tcp +short large.example TYPE45234 > dig.out.$i.test$n || { ret=1 ; echo_i "dig failed" ; }
    $DIFF -s large.out dig.out.$i.test$n > /dev/null || { ret=1 ; echo_i "$DIFF failed"; }
    [ "$ret" -eq 0 ] && break
    sleep 1
done
[ $ret = 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=$((n+1))
echo_i "checking large unknown record loading on inline secondary ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.3 +tcp +short large.example TYPE45234 > dig.out.test$n || { ret=1 ; echo_i "dig failed" ; }
$DIFF large.out dig.out.test$n > /dev/null || { ret=1 ; echo_i "$DIFF failed"; }
[ $ret = 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "stop and restart inline secondary"
stop_server ns3
start_server --noclean --restart --port ${PORT} ns3

# server may be answering queries before zones are loaded,
# so retry a few times if this query fails
n=$((n+1))
echo_i "checking large unknown record loading on inline secondary ($n)"
for try in 0 1 2 3 4 5 6 7 8 9; do
    ret=0
    $DIG $DIGOPTS @10.53.0.3 +tcp +short large.example TYPE45234 > dig.out.$i.test$n || { ret=1 ; echo_i "dig failed" ; }
    $DIFF large.out dig.out.$i.test$n > /dev/null || { ret=1 ; echo_i "$DIFF failed"; }
    [ "$ret" -eq 0 ] && break
    sleep 1
done
[ $ret = 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=$((n+1))
echo_i "check that '"'"\\#"'"' is not treated as the unknown escape sequence ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.1 +tcp +short txt8.example txt > dig.out.test$n
echo '"#" "2" "0145"' | $DIFF - dig.out.test$n || ret=1
[ $ret = 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=$((n+1))
echo_i "check that 'TXT \# text' is not treated as the unknown escape sequence ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.1 +tcp +short txt9.example txt > dig.out.test$n
echo '"#" "text"' | $DIFF - dig.out.test$n || ret=1
[ $ret = 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=$((n+1))
echo_i "check that 'TYPE353 \# cat' produces 'not a valid number' ($n)"
ret=0
$CHECKZONE nan.bad zones/nan.bad > check.out 2>&1
grep "not a valid number" check.out > /dev/null || ret=1
[ $ret = 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
