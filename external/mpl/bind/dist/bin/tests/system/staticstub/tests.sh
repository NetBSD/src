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
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

status=0
n=0

for conf in conf/good*.conf
do
	n=`expr $n + 1`
	echo_i "checking that $conf is accepted ($n)"
	ret=0
	$CHECKCONF "$conf" || ret=1
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`
done

for conf in conf/bad*.conf
do
	n=`expr $n + 1`
	echo_i "checking that $conf is rejected ($n)"
	ret=0
	$CHECKCONF "$conf" >/dev/null && ret=1
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`
done

n=`expr $n + 1`
echo_i "trying an axfr that should be denied (NOTAUTH) ($n)"
ret=0
$DIG $DIGOPTS +tcp data.example. @10.53.0.2 axfr > dig.out.ns2.test$n || ret=1
grep "; Transfer failed." dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "non recursive query for a static-stub zone with server name should be rejected ($n)"
ret=0
 $DIG $DIGOPTS +tcp +norec data.example. @10.53.0.2 txt > dig.out.ns2.test$n \
 	|| ret=1
grep "REFUSED" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "non recursive query for a static-stub zone with server name should be rejected ($n)"
ret=0
$DIG $DIGOPTS +tcp +norec data.example.org. @10.53.0.2 txt > dig.out.ns2.test$n \
	|| ret=1
grep "REFUSED" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "allow-query ACL ($n)"
ret=0
$DIG $DIGOPTS +tcp +norec data.example. @10.53.0.2 txt -b 10.53.0.7 \
	> dig.out.ns2.test$n || ret=1
grep "REFUSED" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "look for static-stub zone data with recursion (should be found) ($n)"
ret=0
$DIG $DIGOPTS +tcp +noauth data.example. @10.53.0.2 txt > dig.out.ns2.test$n || ret=1
digcomp knowngood.dig.out.rec dig.out.ns2.test$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking authoritative NS is ignored for delegation ($n)"
ret=0
# the auth server returns a different (and incorrect) NS for .example.
$DIG $DIGOPTS +tcp example. @10.53.0.2 ns > dig.out.ns2.test1.$n || ret=1
grep "ns4.example." dig.out.ns2.test1.$n > /dev/null || ret=1
# but static-stub configuration should still be used
$DIG $DIGOPTS +tcp data2.example. @10.53.0.2 txt > dig.out.ns2.test2.$n || ret=1
grep "2nd test data" dig.out.ns2.test2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking queries for a child zone of the static-stub zone ($n)"
ret=0
# prime the delegation to a child zone of the static-stub zone
$DIG $DIGOPTS +tcp data1.sub.example. @10.53.0.2 txt > dig.out.ns2.test1.$n || ret=1
grep "1st sub test data" dig.out.ns2.test1.$n > /dev/null || ret=1
# temporarily disable the the parent zone
copy_setports ns3/named.conf.in tmp
sed 's/EXAMPLE_ZONE_PLACEHOLDER//' tmp > ns3/named.conf
rndc_reload ns3 10.53.0.3
# query the child zone again.  this should directly go to the child and
# succeed.
for i in 0 1 2 3 4 5 6 7 8 9
do
	$DIG $DIGOPTS +tcp data2.sub.example. @10.53.0.2 txt > dig.out.ns2.test2.$n || ret=1
	grep "2nd sub test data" dig.out.ns2.test2.$n > /dev/null && break
	sleep 1
done
grep "2nd sub test data" dig.out.ns2.test2.$n > /dev/null || ret=1
# re-enable the parent
copy_setports ns3/named.conf.in tmp
sed 's/EXAMPLE_ZONE_PLACEHOLDER/zone "example" { type master; file "example.db.signed"; };/' tmp > ns3/named.conf
rndc_reload ns3 10.53.0.3
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking authoritative NS addresses are ignored for delegation ($n)"
ret=0
# the auth server returns a different (and incorrect) A/AAA RR for .example.
$DIG $DIGOPTS +tcp example. @10.53.0.2 a > dig.out.ns2.test1.$n || ret=1
grep "10.53.0.4" dig.out.ns2.test1.$n > /dev/null || ret=1
$DIG $DIGOPTS +tcp example. @10.53.0.2 aaaa > dig.out.ns2.test2.$n || ret=1
grep "::1" dig.out.ns2.test2.$n > /dev/null || ret=1
# reload the server.  this will flush the ADB.
rndc_reload ns2 10.53.0.2
# ask another RR that would require delegation.  static-stub configuration
# should still be used instead of the authoritative A/AAAA cached above.
$DIG $DIGOPTS +tcp data3.example. @10.53.0.2 txt > dig.out.ns2.test3.$n || ret=1
grep "3rd test data" dig.out.ns2.test3.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# the authoritative server of the query domain (example.com) is the apex
# name of the static-stub zone (example).  in this case the static-stub
# configuration must be ignored and cached information must be used.
n=`expr $n + 1`
echo_i "checking NS of static-stub is ignored when referenced from other domain ($n)"
ret=0
$DIG $DIGOPTS +tcp data.example.com. @10.53.0.2 txt > dig.out.ns2.test$n || ret=1
grep "example com data" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# check server-names
n=`expr $n + 1`
echo_i "checking static-stub with a server-name ($n)"
ret=0
$DIG $DIGOPTS +tcp data.example.org. @10.53.0.2 txt > dig.out.ns2.test$n || ret=1
grep "example org data" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
# Note: for a short term workaround we use ::1, assuming it's configured and
# usable for our tests.  We should eventually use the test ULA and available
# checks introduced in change 2916.
if $TESTSOCK6 ../testsock6.pl ::1 2> /dev/null
then
    echo_i "checking IPv6 static-stub address ($n)"
    ret=0
    $DIG $DIGOPTS +tcp data.example.info. @10.53.0.2 txt > dig.out.ns2.test$n || ret=1
    grep "example info data" dig.out.ns2.test$n > /dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
else
    echo_i "SKIPPED: checking IPv6 static-stub address ($n)"
fi

n=`expr $n + 1`
echo_i "look for static-stub zone data with DNSSEC validation ($n)"
ret=0
$DIG $DIGOPTS +tcp +dnssec data4.example. @10.53.0.2 txt > dig.out.ns2.test$n || ret=1
grep "ad; QUERY" dig.out.ns2.test$n > /dev/null || ret=1
grep "4th test data" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "look for a child of static-stub zone data with DNSSEC validation ($n)"
ret=0
$DIG $DIGOPTS +tcp +dnssec data3.sub.example. @10.53.0.2 txt > dig.out.ns2.test$n || ret=1
grep "ad; QUERY" dig.out.ns2.test$n > /dev/null || ret=1
grep "3rd sub test data" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# reload with a different name server: exisitng zone shouldn't be reused.
n=`expr $n + 1`
echo_i "checking server reload with a different static-stub config ($n)"
ret=0
copy_setports ns2/named.conf.in tmp
sed 's/SERVER_CONFIG_PLACEHOLDER/server-addresses { 10.53.0.4; };/' tmp > ns2/named.conf
rndc_reload ns2 10.53.0.2
$DIG $DIGOPTS +tcp data2.example.org. @10.53.0.2 txt > dig.out.ns2.test$n || ret=1
grep "2nd example org data" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking static-stub of a undelegated tld resolves after DS query ($n)"
ret=0
$DIG $DIGOPTS undelegated. @10.53.0.2 ds > dig.out.ns2.ds.test$n
$DIG $DIGOPTS undelegated. @10.53.0.2 soa > dig.out.ns2.soa.test$n
grep "status: NXDOMAIN" dig.out.ns2.ds.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns2.soa.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
