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

DIGOPTS="+nosea +nocomm +nocmd +noquest +noadd +noauth +nocomm +nostat +short +nocookie"
DIGCMD="$DIG $DIGOPTS -p ${PORT}"

status=0

GOOD_RANDOM="1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24"
GOOD_RANDOM_NO=24

if grep "^#define DNS_RDATASET_FIXED" $TOP/config.h > /dev/null 2>&1 ; then
    test_fixed=true
else
    echo_i "Order 'fixed' disabled at compile time"
    test_fixed=false
fi

#
#
#
if $test_fixed; then
    echo_i "Checking order fixed (primary)"
    ret=0
    for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
    do
    $DIGCMD @10.53.0.1 fixed.example > dig.out.fixed || ret=1
    $DIFF dig.out.fixed dig.out.fixed.good >/dev/null || ret=1
    done
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
else
    echo_i "Checking order fixed behaves as cyclic when disabled (primary)"
    ret=0
    matches=0
    for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
    do
	j=$((i % 4))
	$DIGCMD @10.53.0.1 fixed.example > dig.out.fixed  || ret=1
	if [ $i -le 4 ]; then
	    cp dig.out.fixed dig.out.$j
	else
	    $DIFF dig.out.fixed dig.out.$j >/dev/null && matches=$((matches + 1))
	fi
    done
    $DIFF dig.out.0 dig.out.1 >/dev/null && ret=1
    $DIFF dig.out.0 dig.out.2 >/dev/null && ret=1
    $DIFF dig.out.0 dig.out.3 >/dev/null && ret=1
    $DIFF dig.out.1 dig.out.2 >/dev/null && ret=1
    $DIFF dig.out.1 dig.out.3 >/dev/null && ret=1
    $DIFF dig.out.2 dig.out.3 >/dev/null && ret=1
    if [ $matches -ne 16 ]; then ret=1; fi
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
fi

#
#
#
echo_i "Checking order cyclic (primary + additional)"
ret=0
matches=0
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
do
    j=$((i % 4))
    $DIGCMD @10.53.0.1 cyclic.example > dig.out.cyclic || ret=1
    if [ $i -le 4 ]; then
	cp dig.out.cyclic dig.out.$j
    else
	$DIFF dig.out.cyclic dig.out.$j >/dev/null && matches=$((matches + 1))
    fi
done
$DIFF dig.out.0 dig.out.1 >/dev/null && ret=1
$DIFF dig.out.0 dig.out.2 >/dev/null && ret=1
$DIFF dig.out.0 dig.out.3 >/dev/null && ret=1
$DIFF dig.out.1 dig.out.2 >/dev/null && ret=1
$DIFF dig.out.1 dig.out.3 >/dev/null && ret=1
$DIFF dig.out.2 dig.out.3 >/dev/null && ret=1
if [ $matches -ne 16 ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

#
#
#
echo_i "Checking order cyclic (primary)"
ret=0
matches=0
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
do
    j=$((i % 4))
    $DIGCMD @10.53.0.1 cyclic2.example > dig.out.cyclic2 || ret=1
    if [ $i -le 4 ]; then
	cp dig.out.cyclic2 dig.out.$j
    else
	$DIFF dig.out.cyclic2 dig.out.$j >/dev/null && matches=$((matches + 1))
    fi
done
$DIFF dig.out.0 dig.out.1 >/dev/null && ret=1
$DIFF dig.out.0 dig.out.2 >/dev/null && ret=1
$DIFF dig.out.0 dig.out.3 >/dev/null && ret=1
$DIFF dig.out.1 dig.out.2 >/dev/null && ret=1
$DIFF dig.out.1 dig.out.3 >/dev/null && ret=1
$DIFF dig.out.2 dig.out.3 >/dev/null && ret=1
if [ $matches -ne 16 ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
echo_i "Checking order random (primary)"
ret=0
for i in $GOOD_RANDOM
do
    eval match$i=0
done
for i in a b c d e f g h i j k l m n o p q r s t u v w x y z 0 1 2 3 4 5 6 7 9
do
    $DIGCMD @10.53.0.1 random.example > dig.out.random || ret=1
    match=0
    for j in $GOOD_RANDOM
    do
	eval "$DIFF dig.out.random dig.out.random.good$j >/dev/null && match$j=1 match=1"
	if [ $match -eq 1 ]; then break; fi
    done
    if [ $match -eq 0 ]; then ret=1; fi
done
match=0
for i in $GOOD_RANDOM
do
     eval "match=\$((match + match$i))"
done
echo_i "Random selection return $match of ${GOOD_RANDOM_NO} possible orders in 36 samples"
if [ $match -lt $((GOOD_RANDOM_NO / 3)) ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "Checking order none (primary)"
ret=0
# Fetch the "reference" response and ensure it contains the expected records.
$DIGCMD @10.53.0.1 none.example > dig.out.none || ret=1
for i in 1 2 3 4; do
    grep -F -q 1.2.3.$i dig.out.none || ret=1
done
# Ensure 20 further queries result in the same response as the "reference" one.
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
    $DIGCMD @10.53.0.1 none.example > dig.out.test$i || ret=1
    $DIFF dig.out.none dig.out.test$i >/dev/null || ret=1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

#
#
#
if $test_fixed; then
    echo_i "Checking order fixed (secondary)"
    ret=0
    for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
    do
    $DIGCMD @10.53.0.2 fixed.example > dig.out.fixed || ret=1
    $DIFF dig.out.fixed dig.out.fixed.good || ret=1
    done
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
fi

#
#
#
echo_i "Checking order cyclic (secondary + additional)"
ret=0
matches=0
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
do
    j=$((i % 4))
    $DIGCMD @10.53.0.2 cyclic.example > dig.out.cyclic || ret=1
    if [ $i -le 4 ]; then
	cp dig.out.cyclic dig.out.$j
    else
	$DIFF dig.out.cyclic dig.out.$j >/dev/null && matches=$((matches + 1))
    fi
done
$DIFF dig.out.0 dig.out.1 >/dev/null && ret=1
$DIFF dig.out.0 dig.out.2 >/dev/null && ret=1
$DIFF dig.out.0 dig.out.3 >/dev/null && ret=1
$DIFF dig.out.1 dig.out.2 >/dev/null && ret=1
$DIFF dig.out.1 dig.out.3 >/dev/null && ret=1
$DIFF dig.out.2 dig.out.3 >/dev/null && ret=1
if [ $matches -ne 16 ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

#
#
#
echo_i "Checking order cyclic (secondary)"
ret=0
matches=0
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
do
    j=$((i % 4))
    $DIGCMD @10.53.0.2 cyclic2.example > dig.out.cyclic2 || ret=1
    if [ $i -le 4 ]; then
	cp dig.out.cyclic2 dig.out.$j
    else
	$DIFF dig.out.cyclic2 dig.out.$j >/dev/null && matches=$((matches + 1))
    fi
done
$DIFF dig.out.0 dig.out.1 >/dev/null && ret=1
$DIFF dig.out.0 dig.out.2 >/dev/null && ret=1
$DIFF dig.out.0 dig.out.3 >/dev/null && ret=1
$DIFF dig.out.1 dig.out.2 >/dev/null && ret=1
$DIFF dig.out.1 dig.out.3 >/dev/null && ret=1
$DIFF dig.out.2 dig.out.3 >/dev/null && ret=1
if [ $matches -ne 16 ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "Checking order random (secondary)"
ret=0
for i in $GOOD_RANDOM
do
    eval match$i=0
done
for i in a b c d e f g h i j k l m n o p q r s t u v w x y z 0 1 2 3 4 5 6 7 9
do
    $DIGCMD @10.53.0.2 random.example > dig.out.random || ret=1
    match=0
    for j in $GOOD_RANDOM
    do
	eval "$DIFF dig.out.random dig.out.random.good$j >/dev/null && match$j=1 match=1"
	if [ $match -eq 1 ]; then break; fi
    done
    if [ $match -eq 0 ]; then ret=1; fi
done
match=0
for i in $GOOD_RANDOM
do
     eval "match=\$((match + match$i))"
done
echo_i "Random selection return $match of ${GOOD_RANDOM_NO} possible orders in 36 samples"
if [ $match -lt $((GOOD_RANDOM_NO / 3)) ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "Checking order none (secondary)"
ret=0
# Fetch the "reference" response and ensure it contains the expected records.
$DIGCMD @10.53.0.2 none.example > dig.out.none || ret=1
for i in 1 2 3 4; do
    grep -F -q 1.2.3.$i dig.out.none || ret=1
done
# Ensure 20 further queries result in the same response as the "reference" one.
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
    $DIGCMD @10.53.0.2 none.example > dig.out.test$i || ret=1
    $DIFF dig.out.none dig.out.test$i >/dev/null || ret=1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "Shutting down secondary"

stop_server ns2

echo_i "Checking for secondary's on disk copy of zone"

if [ ! -f ns2/root.bk ]
then
    echo_i "failed";
    status=$((status + 1))
fi

echo_i "Re-starting secondary"

start_server --noclean --restart --port ${PORT} ns2

#
#
#
if $test_fixed; then
    echo_i "Checking order fixed (secondary loaded from disk)"
    ret=0
    for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
    do
    $DIGCMD @10.53.0.2 fixed.example > dig.out.fixed || ret=1
    $DIFF dig.out.fixed dig.out.fixed.good || ret=1
    done
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
fi

#
#
#
echo_i "Checking order cyclic (secondary + additional, loaded from disk)"
ret=0
matches=0
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
do
    j=$((i % 4))
    $DIGCMD @10.53.0.2 cyclic.example > dig.out.cyclic || ret=1
    if [ $i -le 4 ]; then
	cp dig.out.cyclic dig.out.$j
    else
	$DIFF dig.out.cyclic dig.out.$j >/dev/null && matches=$((matches + 1))
    fi
done
$DIFF dig.out.0 dig.out.1 >/dev/null && ret=1
$DIFF dig.out.0 dig.out.2 >/dev/null && ret=1
$DIFF dig.out.0 dig.out.3 >/dev/null && ret=1
$DIFF dig.out.1 dig.out.2 >/dev/null && ret=1
$DIFF dig.out.1 dig.out.3 >/dev/null && ret=1
$DIFF dig.out.2 dig.out.3 >/dev/null && ret=1
if [ $matches -ne 16 ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

#
#
#
echo_i "Checking order cyclic (secondary loaded from disk)"
ret=0
matches=0
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
do
    j=$((i % 4))
    $DIGCMD @10.53.0.2 cyclic2.example > dig.out.cyclic2 || ret=1
    if [ $i -le 4 ]; then
	cp dig.out.cyclic2 dig.out.$j
    else
	$DIFF dig.out.cyclic2 dig.out.$j >/dev/null && matches=$((matches + 1))
    fi
done
$DIFF dig.out.0 dig.out.1 >/dev/null && ret=1
$DIFF dig.out.0 dig.out.2 >/dev/null && ret=1
$DIFF dig.out.0 dig.out.3 >/dev/null && ret=1
$DIFF dig.out.1 dig.out.2 >/dev/null && ret=1
$DIFF dig.out.1 dig.out.3 >/dev/null && ret=1
$DIFF dig.out.2 dig.out.3 >/dev/null && ret=1
if [ $matches -ne 16 ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "Checking order random (secondary loaded from disk)"
ret=0
for i in $GOOD_RANDOM
do
    eval match$i=0
done
for i in a b c d e f g h i j k l m n o p q r s t u v w x y z 0 1 2 3 4 5 6 7 9
do
    $DIGCMD @10.53.0.2 random.example > dig.out.random || ret=1
    match=0
    for j in $GOOD_RANDOM
    do
	eval "$DIFF dig.out.random dig.out.random.good$j >/dev/null && match$j=1 match=1"
	if [ $match -eq 1 ]; then break; fi
    done
    if [ $match -eq 0 ]; then ret=1; fi
done
match=0
for i in $GOOD_RANDOM
do
    eval "match=\$((match + match$i))"
done
echo_i "Random selection return $match of ${GOOD_RANDOM_NO} possible orders in 36 samples"
if [ $match -lt $((GOOD_RANDOM_NO / 3)) ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "Checking order none (secondary loaded from disk)"
ret=0
# Fetch the "reference" response and ensure it contains the expected records.
$DIGCMD @10.53.0.2 none.example > dig.out.none || ret=1
for i in 1 2 3 4; do
    grep -F -q 1.2.3.$i dig.out.none || ret=1
done
# Ensure 20 further queries result in the same response as the "reference" one.
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
    $DIGCMD @10.53.0.2 none.example > dig.out.test$i || ret=1
    $DIFF dig.out.none dig.out.test$i >/dev/null || ret=1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

#
#
#
if $test_fixed; then
    echo_i "Checking order fixed (cache)"
    ret=0
    for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
    do
    $DIGCMD @10.53.0.3 fixed.example > dig.out.fixed || ret=1
    $DIFF dig.out.fixed dig.out.fixed.good || ret=1
    done
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
fi

#
#
#
echo_i "Checking order cyclic (cache + additional)"
ret=0
# prime acache
$DIGCMD @10.53.0.3 cyclic.example > dig.out.cyclic || ret=1
matches=0
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
do
    j=$((i % 4))
    $DIGCMD @10.53.0.3 cyclic.example > dig.out.cyclic || ret=1
    if [ $i -le 4 ]; then
	cp dig.out.cyclic dig.out.$j
    else
	$DIFF dig.out.cyclic dig.out.$j >/dev/null && matches=$((matches + 1))
    fi
done
$DIFF dig.out.0 dig.out.1 >/dev/null && ret=1
$DIFF dig.out.0 dig.out.2 >/dev/null && ret=1
$DIFF dig.out.0 dig.out.3 >/dev/null && ret=1
$DIFF dig.out.1 dig.out.2 >/dev/null && ret=1
$DIFF dig.out.1 dig.out.3 >/dev/null && ret=1
$DIFF dig.out.2 dig.out.3 >/dev/null && ret=1
if [ $matches -ne 16 ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

#
#
#
echo_i "Checking order cyclic (cache)"
ret=0
# prime acache
$DIGCMD @10.53.0.3 cyclic2.example > dig.out.cyclic2 || ret=1
matches=0
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
do
    j=$((i % 4))
    $DIGCMD @10.53.0.3 cyclic2.example > dig.out.cyclic2 || ret=1
    if [ $i -le 4 ]; then
	cp dig.out.cyclic2 dig.out.$j
    else
	$DIFF dig.out.cyclic2 dig.out.$j >/dev/null && matches=$((matches + 1))
    fi
done
$DIFF dig.out.0 dig.out.1 >/dev/null && ret=1
$DIFF dig.out.0 dig.out.2 >/dev/null && ret=1
$DIFF dig.out.0 dig.out.3 >/dev/null && ret=1
$DIFF dig.out.1 dig.out.2 >/dev/null && ret=1
$DIFF dig.out.1 dig.out.3 >/dev/null && ret=1
$DIFF dig.out.2 dig.out.3 >/dev/null && ret=1
if [ $matches -ne 16 ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "Checking order random (cache)"
ret=0
for i in $GOOD_RANDOM
do
    eval match$i=0
done
for i in a b c d e f g h i j k l m n o p q r s t u v w x y z 0 1 2 3 4 5 6 7 9
do
    $DIGCMD @10.53.0.3 random.example > dig.out.random || ret=1
    match=0
    for j in $GOOD_RANDOM
    do
	eval "$DIFF dig.out.random dig.out.random.good$j >/dev/null && match$j=1 match=1"
	if [ $match -eq 1 ]; then break; fi
    done
    if [ $match -eq 0 ]; then ret=1; fi
done
match=0
for i in $GOOD_RANDOM
do
     eval "match=\$((match + match$i))"
done
echo_i "Random selection return $match of ${GOOD_RANDOM_NO} possible orders in 36 samples"
if [ $match -lt $((GOOD_RANDOM_NO / 3)) ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "Checking order none (cache)"
ret=0
# Fetch the "reference" response and ensure it contains the expected records.
$DIGCMD @10.53.0.3 none.example > dig.out.none || ret=1
for i in 1 2 3 4; do
    grep -F -q 1.2.3.$i dig.out.none || ret=1
done
# Ensure 20 further queries result in the same response as the "reference" one.
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
    $DIGCMD @10.53.0.3 none.example > dig.out.test$i || ret=1
    $DIFF dig.out.none dig.out.test$i >/dev/null || ret=1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "Checking default order (cache)"
ret=0
for i in $GOOD_RANDOM
do
    eval match$i=0
done
for i in a b c d e f g h i j k l m n o p q r s t u v w x y z 0 1 2 3 4 5 6 7 9
do
    $DIGCMD @10.53.0.5 random.example > dig.out.random || ret=1
    match=0
    for j in $GOOD_RANDOM
    do
	eval "$DIFF dig.out.random dig.out.random.good$j >/dev/null && match$j=1 match=1"
	if [ $match -eq 1 ]; then break; fi
    done
    if [ $match -eq 0 ]; then ret=1; fi
done
match=0
for i in $GOOD_RANDOM
do
     eval "match=\$((match + match$i))"
done
echo_i "Default selection return $match of ${GOOD_RANDOM_NO} possible orders in 36 samples"
if [ $match -lt $((GOOD_RANDOM_NO / 3)) ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "Checking default order no match in rrset-order (cache)"
ret=0
# Fetch the "reference" response and ensure it contains the expected records.
$DIGCMD @10.53.0.4 nomatch.example > dig.out.nomatch || ret=1
for i in 1 2 3 4; do
    grep -F -q 1.2.3.$i dig.out.nomatch || ret=1
done
# Ensure 20 further queries result in the same response as the "reference" one.
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
    $DIGCMD @10.53.0.4 nomatch.example > dig.out.test$i || ret=1
    $DIFF dig.out.nomatch dig.out.test$i >/dev/null || ret=1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
