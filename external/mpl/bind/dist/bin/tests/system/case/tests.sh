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

DIGOPTS="+tcp +nosea +nostat +noquest +nocomm +nocmd -p ${PORT}"

status=0
n=0

n=`expr $n + 1`
echo_i "waiting for zone transfer to complete ($n)"
ret=0
for i in 1 2 3 4 5 6 7 8 9
do
	$DIG $DIGOPTS soa example. @10.53.0.2 > dig.ns2.test$n
	grep SOA dig.ns2.test$n > /dev/null && break
	sleep 1
done
for i in 1 2 3 4 5 6 7 8 9
do
	$DIG $DIGOPTS soa dynamic. @10.53.0.2 > dig.ns2.test$n
	grep SOA dig.ns2.test$n > /dev/null && break
	sleep 1
done

n=`expr $n + 1`
echo_i "testing case preserving responses - no acl ($n)"
ret=0
$DIG $DIGOPTS mx example. @10.53.0.1 > dig.ns1.test$n
grep "0.mail.eXaMpLe" dig.ns1.test$n > /dev/null || ret=1
grep "mAiL.example" dig.ns1.test$n > /dev/null || ret=1
test $ret -eq 0 || echo_i "failed"
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing no-case-compress acl '{ 10.53.0.2; }' ($n)"
ret=0

# check that we preserve zone case for non-matching query (10.53.0.1)
$DIG $DIGOPTS mx example. -b 10.53.0.1 @10.53.0.1 > dig.ns1.test$n
grep "0.mail.eXaMpLe" dig.ns1.test$n > /dev/null || ret=1
grep "mAiL.example" dig.ns1.test$n > /dev/null || ret=1

# check that we don't preserve zone case for match (10.53.0.2)
$DIG $DIGOPTS mx example. -b 10.53.0.2 @10.53.0.2 > dig.ns2.test$n
grep "0.mail.example" dig.ns2.test$n > /dev/null || ret=1
grep "mail.example" dig.ns2.test$n > /dev/null || ret=1

test $ret -eq 0 || echo_i "failed"
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing load of dynamic zone with various \$ORIGIN values ($n)"
ret=0
$DIG $DIGOPTS axfr dynamic @10.53.0.1 > dig.ns1.test$n
digcomp dig.ns1.test$n dynamic.good || ret=1

test $ret -eq 0 || echo_i "failed"
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "transfer of dynamic zone with various \$ORIGIN values ($n)"
ret=0
$DIG $DIGOPTS axfr dynamic @10.53.0.2 > dig.ns2.test$n
digcomp dig.ns2.test$n dynamic.good || ret=1

test $ret -eq 0 || echo_i "failed"
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "change SOA owner case via update ($n)"
$NSUPDATE << EOF
server 10.53.0.1 ${PORT}
zone dynamic
update add dYNAMIc 0 SOA mname1. . 2000042408 20 20 1814400 3600
send
EOF
$DIG $DIGOPTS axfr dynamic @10.53.0.1 > dig.ns1.test$n
digcomp dig.ns1.test$n postupdate.good || ret=1

test $ret -eq 0 || echo_i "failed"
status=`expr $status + $ret`

for i in 1 2 3 4 5 6 7 8 9
do
	$DIG $DIGOPTS soa dynamic @10.53.0.2 | grep 2000042408 > /dev/null && break
	sleep 1
done

n=`expr $n + 1`
echo_i "check SOA owner case is transfered to slave ($n)"
ret=0
$DIG $DIGOPTS axfr dynamic @10.53.0.2 > dig.ns2.test$n
digcomp dig.ns2.test$n postupdate.good || ret=1

test $ret -eq 0 || echo_i "failed"
status=`expr $status + $ret`

#update delete Ns1.DyNaMIC. 300 IN A 10.53.0.1
n=`expr $n + 1`
echo_i "change A record owner case via update ($n)"
$NSUPDATE << EOF
server 10.53.0.1 ${PORT}
zone dynamic
update add Ns1.DyNaMIC. 300 IN A 10.53.0.1
send
EOF
$DIG $DIGOPTS axfr dynamic @10.53.0.1 > dig.ns1.test$n
digcomp dig.ns1.test$n postns1.good || ret=1

test $ret -eq 0 || echo_i "failed"
status=`expr $status + $ret`

for i in 1 2 3 4 5 6 7 8 9
do
	$DIG $DIGOPTS soa dynamic @10.53.0.2 | grep 2000042409 > /dev/null && break
	sleep 1
done

n=`expr $n + 1`
echo_i "check A owner case is transfered to slave ($n)"
ret=0
$DIG $DIGOPTS axfr dynamic @10.53.0.2 > dig.ns2.test$n
digcomp dig.ns2.test$n postns1.good || ret=1
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
