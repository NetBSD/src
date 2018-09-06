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

status=0
n=0

DIGOPTS="+noadd +nosea +nostat +nocmd +noauth +dnssec -p 5300"

ksk=ns1/`cat ns1/keyname`.key
kskpat=`awk '/DNSKEY/ { print $8 }' $ksk`
kskid=`sed 's/^Kexample\.+005+0*//' < ns1/keyname`
rkskid=`expr \( $kskid + 128 \) \% 65536`

echo "I:checking for KSK not yet published ($n)"
ret=0
$DIG $DIGOPTS -t dnskey example. @10.53.0.1 > dig.out.ns1.test$n || ret=1
# Note - this is looking for failure, hence the &&
tr -d ' ' < dig.out.ns1.test$n | grep $kskpat > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

# 5s real, 55s virtual, P +20
sleep 4

echo "I:checking for KSK published but not yet active ($n)"
ret=0
$DIG $DIGOPTS -t dnskey example. @10.53.0.1 > dig.out.ns1.test$n || ret=1
tr -d ' ' < dig.out.ns1.test$n | grep $kskpat > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep 'RRSIG.*'" $kskid "'example\. ' dig.out.ns1.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ] ; then echo "I:failed"; fi
status=`expr $status + $ret`

# 10s real, 2h15mn virtual, A +1h
sleep 5

echo "I:checking for KSK active ($n)"
ret=0
$DIG $DIGOPTS -t dnskey example. @10.53.0.1 > dig.out.ns1.test$n || ret=1
tr -d ' ' < dig.out.ns1.test$n | grep $kskpat > /dev/null || ret=1
grep 'RRSIG.*'" $kskid "'example\. ' dig.out.ns1.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ] ; then echo "I:failed"; fi
status=`expr $status + $ret`

# 11s real, 6h7,m virtual, R +6h
sleep 1

echo "I:checking for KSK revoked ($n)"
ret=0
$DIG $DIGOPTS -t dnskey example. @10.53.0.1 > dig.out.ns1.test$n || ret=1
tr -d ' ' < dig.out.ns1.test$n | grep $kskpat > /dev/null || ret=1
awk 'BEGIN { $noksk=1 } \
/DNSKEY/ { $5==385 && $noksk=0 } \
END { exit $noksk }' < dig.out.ns1.test$n > /dev/null || ret=1
# Note - this is looking for failure, hence the &&
grep 'RRSIG.*'" $kskid "'example\. ' dig.out.ns1.test$n > /dev/null && ret=1
grep 'RRSIG.*'" $rkskid "'example\. ' dig.out.ns1.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ] ; then echo "I:failed"; fi
status=`expr $status + $ret`

# 13s real, 45h virtual, I +1d
sleep 2

echo "I:checking for KSK retired but not yet deleted ($n)"
ret=0
$DIG $DIGOPTS -t dnskey example. @10.53.0.1 > dig.out.ns1.test$n || ret=1
tr -d ' ' < dig.out.ns1.test$n | grep $kskpat > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ] ; then echo "I:failed"; fi
status=`expr $status + $ret`

# 17s real, 103d virtual, D +1mo
sleep 4

echo "I:checking for KSK deleted ($n)"
ret=0
$DIG $DIGOPTS -t dnskey example. @10.53.0.1 > dig.out.ns1.test$n || ret=1
# Note - this is looking for failure, hence the &&
tr -d ' ' < dig.out.ns1.test$n | grep $kskpat > /dev/null && ret=1
# Note - this is looking for failure, hence the &&
grep 'RRSIG.*'" $rkskid "'example\. ' dig.out.ns1.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ] ; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:exit status: $status"
exit $status
