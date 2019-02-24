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

rm -f dig.out.*

DIGOPTS="+tcp +noadd +nosea +nostat +nocmd -p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

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

#
# Authoritative tests against:
#	filter-aaaa-on-v4 yes;
#	filter-aaaa { 10.53.0.1; };
#
n=`expr $n + 1`
echo_i "checking that AAAA is returned when only AAAA record exists, signed ($n)"
ret=0
$DIG $DIGOPTS aaaa aaaa-only.signed -b 10.53.0.1 @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep "AUTHORITY: 1," dig.out.ns1.test$n > /dev/null || ret=1
grep ::2 dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when only AAAA record exists, unsigned ($n)"
ret=0
$DIG $DIGOPTS aaaa aaaa-only.unsigned -b 10.53.0.1 @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep "AUTHORITY: 1," dig.out.ns1.test$n > /dev/null || ret=1
grep ::5 dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, signed ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.signed -b 10.53.0.1 @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep "ANSWER: 0" dig.out.ns1.test$n > /dev/null || ret=1
grep "AUTHORITY: 0" dig.out.ns1.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, unsigned ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b 10.53.0.1 @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep "ANSWER: 0" dig.out.ns1.test$n > /dev/null || ret=1
grep "AUTHORITY: 0" dig.out.ns1.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A records exist, signed and DO set ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.signed +dnssec -b 10.53.0.1 @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep "AUTHORITY: 2," dig.out.ns1.test$n > /dev/null || ret=1
grep ::3 dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, unsigned and DO set ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b 10.53.0.1 @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep "ANSWER: 0" dig.out.ns1.test$n > /dev/null || ret=1
grep "AUTHORITY: 0," dig.out.ns1.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A records exist and query source does not match acl ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b 10.53.0.2 @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "AUTHORITY: 1," dig.out.ns1.test$n > /dev/null || ret=1
grep ::6 dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, signed and qtype=ANY ($n)"
ret=0
$DIG $DIGOPTS any dual.signed -b 10.53.0.1 @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "1.0.0.3" dig.out.ns1.test$n > /dev/null || ret=1
grep "::3" dig.out.ns1.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, unsigned and qtype=ANY ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned -b 10.53.0.1 @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "1.0.0.6" dig.out.ns1.test$n > /dev/null || ret=1
grep "::6" dig.out.ns1.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that both A and AAAA are returned when both AAAA and A records exist, signed, qtype=ANY and DO is set ($n)"
ret=0
$DIG $DIGOPTS any dual.signed +dnssec -b 10.53.0.1 @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "AUTHORITY: 2," dig.out.ns1.test$n > /dev/null || ret=1
grep ::3 dig.out.ns1.test$n > /dev/null || ret=1
grep "1.0.0.3" dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, unsigned, qtype=ANY and DO is set ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned +dnssec -b 10.53.0.1 @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "1.0.0.6" dig.out.ns1.test$n > /dev/null || ret=1
grep "::6" dig.out.ns1.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that both A and AAAA are returned when both AAAA and A records exist, qtype=ANY and query source does not match acl ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned -b 10.53.0.2 @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "AUTHORITY: 1," dig.out.ns1.test$n > /dev/null || ret=1
grep 1.0.0.6 dig.out.ns1.test$n > /dev/null || ret=1
grep ::6 dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A record exists, unsigned over IPv6 ($n)"
if $TESTSOCK6 fd92:7065:b8e:ffff::1
then
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b fd92:7065:b8e:ffff::1 @fd92:7065:b8e:ffff::1 > dig.out.ns1.test$n || ret=1
grep 2001:db8::6 dig.out.ns1.test$n > /dev/null || ret=1
grep "AUTHORITY: 1," dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
else
echo_i "skipped."
fi

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=NS ($n)"
ret=0
$DIG $DIGOPTS +add ns unsigned -b 10.53.0.1 @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep AAAA dig.out.ns1.test$n > /dev/null 2>&1 && ret=1
grep "ANSWER: 1," dig.out.ns1.test$n > /dev/null || ret=1
grep "ADDITIONAL: 2" dig.out.ns1.test$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=MX, unsigned ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec mx unsigned -b 10.53.0.1 @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep "^mx.unsigned.*AAAA" dig.out.ns1.test$n > /dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is included in additional section, qtype=MX, signed ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec mx signed -b 10.53.0.1 @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep "^mx.signed.*AAAA" dig.out.ns1.test$n > /dev/null 2>&1 || ret=1
grep "AUTHORITY: 2," dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is included in additional section, qtype=MX, unsigned, over IPv6 ($n)"
if $TESTSOCK6 fd92:7065:b8e:ffff::1
then
ret=0
$DIG $DIGOPTS +add +dnssec mx unsigned -b fd92:7065:b8e:ffff::1 @fd92:7065:b8e:ffff::1 > dig.out.ns1.test$n || ret=1
grep "^mx.unsigned.*AAAA" dig.out.ns1.test$n > /dev/null 2>&1 || ret=1
grep "AUTHORITY: 1," dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
else
echo_i "skipped."
fi

#
# Authoritative tests against:
#	filter-aaaa-on-v4 break-dnssec;
#	filter-aaaa { 10.53.0.4; };
#
n=`expr $n + 1`
echo_i "checking that AAAA is returned when only AAAA record exists, signed with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa aaaa-only.signed -b 10.53.0.4 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "AUTHORITY: 1," dig.out.ns4.test$n > /dev/null || ret=1
grep ::2 dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when only AAAA record exists, unsigned with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa aaaa-only.unsigned -b 10.53.0.4 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "AUTHORITY: 1," dig.out.ns4.test$n > /dev/null || ret=1
grep ::5 dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, signed with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.signed -b 10.53.0.4 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "ANSWER: 0" dig.out.ns4.test$n > /dev/null || ret=1
grep "AUTHORITY: 0," dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, unsigned with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b 10.53.0.4 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "ANSWER: 0" dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, signed and DO set with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.signed +dnssec -b 10.53.0.4 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "ANSWER: 0" dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, unsigned and DO set with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b 10.53.0.4 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "ANSWER: 0" dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A records exist and query source does not match acl with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b 10.53.0.2 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep ::6 dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, signed and qtype=ANY with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.signed -b 10.53.0.4 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "1.0.0.3" dig.out.ns4.test$n > /dev/null || ret=1
grep "::3" dig.out.ns4.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, unsigned and qtype=ANY with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned -b 10.53.0.4 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "1.0.0.6" dig.out.ns4.test$n > /dev/null || ret=1
grep "::6" dig.out.ns4.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, signed, qtype=ANY and DO is set with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.signed +dnssec -b 10.53.0.4 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "1.0.0.3" dig.out.ns4.test$n > /dev/null || ret=1
grep ::3 dig.out.ns4.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, unsigned, qtype=ANY and DO is set with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned +dnssec -b 10.53.0.4 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "1.0.0.6" dig.out.ns4.test$n > /dev/null || ret=1
grep "::6" dig.out.ns4.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that both A and AAAA are returned when both AAAA and A records exist, qtype=ANY and query source does not match acl with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned -b 10.53.0.2 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep 1.0.0.6 dig.out.ns4.test$n > /dev/null || ret=1
grep ::6 dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A record exists, unsigned over IPv6 with break-dnssec ($n)"
if $TESTSOCK6 fd92:7065:b8e:ffff::4
then
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b fd92:7065:b8e:ffff::4 @fd92:7065:b8e:ffff::4 > dig.out.ns4.test$n || ret=1
grep 2001:db8::6 dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
else
echo_i "skipped."
fi

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=NS, with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS +add ns unsigned -b 10.53.0.4 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep AAAA dig.out.ns4.test$n > /dev/null 2>&1 && ret=1
grep "ADDITIONAL: 2" dig.out.ns4.test$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=MX, unsigned, with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec mx unsigned -b 10.53.0.4 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "^mx.unsigned.*AAAA" dig.out.ns4.test$n > /dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=MX, signed, with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec mx signed -b 10.53.0.4 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "^mx.signed.*AAAA" dig.out.ns4.test$n > /dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is included in additional section, qtype=MX, unsigned, over IPv6, with break-dnssec ($n)"
if $TESTSOCK6 fd92:7065:b8e:ffff::4
then
ret=0
$DIG $DIGOPTS +add +dnssec mx unsigned -b fd92:7065:b8e:ffff::4 @fd92:7065:b8e:ffff::4 > dig.out.ns4.test$n || ret=1
grep "^mx.unsigned.*AAAA" dig.out.ns4.test$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
else
echo_i "skipped."
fi


#
# Recursive tests against:
#	filter-aaaa-on-v4 yes;
#	filter-aaaa { 10.53.0.2; };
#
n=`expr $n + 1`
echo_i "checking that AAAA is returned when only AAAA record exists, signed, recursive ($n)"
ret=0
$DIG $DIGOPTS aaaa aaaa-only.signed -b 10.53.0.2 @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null || ret=1
grep ::2 dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when only AAAA record exists, unsigned, recursive ($n)"
ret=0
$DIG $DIGOPTS aaaa aaaa-only.unsigned -b 10.53.0.2 @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep ::5 dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, signed, recursive ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.signed -b 10.53.0.2 @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep "ANSWER: 0" dig.out.ns2.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null && ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, unsigned, recursive ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b 10.53.0.2 @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep "ANSWER: 0" dig.out.ns2.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null && ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A records exist, signed and DO set, recursive ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.signed +dnssec -b 10.53.0.2 @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null || ret=1
grep ::3 dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, unsigned and DO set, recursive ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned +dnssec -b 10.53.0.2 @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep "ANSWER: 0" dig.out.ns2.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null && ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A records exist and query source does not match acl, recursive ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b 10.53.0.1 @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep ::6 dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, signed and qtype=ANY recursive ($n)"
ret=0
$DIG $DIGOPTS any dual.signed -b 10.53.0.2 @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "1.0.0.3" dig.out.ns2.test$n > /dev/null || ret=1
grep "::3" dig.out.ns2.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, unsigned and qtype=ANY recursive ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned -b 10.53.0.2 @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "1.0.0.6" dig.out.ns2.test$n > /dev/null || ret=1
grep "::6" dig.out.ns2.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that both A and AAAA are returned when both AAAA and A records exist, signed, qtype=ANY and DO is set, recursive ($n)"
ret=0
$DIG $DIGOPTS any dual.signed +dnssec -b 10.53.0.2 @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null || ret=1
grep ::3 dig.out.ns2.test$n > /dev/null || ret=1
grep "1.0.0.3" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, unsigned, qtype=ANY and DO is set, recursive ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned +dnssec -b 10.53.0.2 @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "1.0.0.6" dig.out.ns2.test$n > /dev/null || ret=1
grep "::6" dig.out.ns2.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that both A and AAAA are returned when both AAAA and A records exist, qtype=ANY and query source does not match acl, recursive ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned -b 10.53.0.1 @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep 1.0.0.6 dig.out.ns2.test$n > /dev/null || ret=1
grep ::6 dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A record exists, unsigned over IPv6, recursive ($n)"
if $TESTSOCK6 fd92:7065:b8e:ffff::2
then
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b fd92:7065:b8e:ffff::2 @fd92:7065:b8e:ffff::2 > dig.out.ns2.test$n || ret=1
grep 2001:db8::6 dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
else
echo_i "skipped."
fi

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=NS ($n)"
ret=0
$DIG $DIGOPTS +add ns unsigned -b 10.53.0.2 @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep AAAA dig.out.ns2.test$n > /dev/null 2>&1 && ret=1
grep "ADDITIONAL: 2" dig.out.ns2.test$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=MX, unsigned, recursive ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec mx unsigned -b 10.53.0.2 @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep "^mx.unsigned.*AAAA" dig.out.ns2.test$n > /dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is included in additional section, qtype=MX, signed, recursive ($n)"
ret=0
# we need to prime the cache with addresses for the MX, since additional
# section data won't be included unless it's validated, and that doesn't
# necessarily happen otherwise.
$DIG $DIGOPTS +dnssec mx.signed @10.53.0.2 > /dev/null
$DIG $DIGOPTS +dnssec mx.signed aaaa @10.53.0.2 > /dev/null
$DIG $DIGOPTS +add +dnssec mx signed -b 10.53.0.2 @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep "^mx.signed.*AAAA" dig.out.ns2.test$n > /dev/null 2>&1 || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is included in additional section, qtype=MX, unsigned, recursive, over IPv6 ($n)"
if $TESTSOCK6 fd92:7065:b8e:ffff::2
then
ret=0
$DIG $DIGOPTS +add +dnssec mx unsigned -b fd92:7065:b8e:ffff::2 @fd92:7065:b8e:ffff::2 > dig.out.ns2.test$n || ret=1
grep "^mx.unsigned.*AAAA" dig.out.ns2.test$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
else
echo_i "skipped."
fi

#
# Recursive tests against:
#	filter-aaaa-on-v4 break-dnssec;
#	filter-aaaa { 10.53.0.3; };
#
n=`expr $n + 1`
echo_i "checking that AAAA is returned when only AAAA record exists, signed, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa aaaa-only.signed -b 10.53.0.3 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns3.test$n > /dev/null || ret=1
grep ::2 dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when only AAAA record exists, unsigned, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa aaaa-only.unsigned -b 10.53.0.3 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep ::5 dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, signed, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.signed -b 10.53.0.3 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep "ANSWER: 0" dig.out.ns3.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns3.test$n > /dev/null && ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, unsigned, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b 10.53.0.3 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep "ANSWER: 0" dig.out.ns3.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns3.test$n > /dev/null && ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, signed and DO set, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.signed +dnssec -b 10.53.0.3 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep "ANSWER: 0" dig.out.ns3.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns3.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, unsigned and DO set, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned +dnssec -b 10.53.0.3 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep "ANSWER: 0" dig.out.ns3.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns3.test$n > /dev/null && ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A records exist and query source does not match acl, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b 10.53.0.1 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
grep ::6 dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, signed and qtype=ANY with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.signed -b 10.53.0.3 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
grep "1.0.0.3" dig.out.ns3.test$n > /dev/null || ret=1
grep "::3" dig.out.ns3.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, unsigned and qtype=ANY with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned -b 10.53.0.3 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
grep "1.0.0.6" dig.out.ns3.test$n > /dev/null || ret=1
grep "::6" dig.out.ns3.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, signed, qtype=ANY and DO is set with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.signed +dnssec -b 10.53.0.3 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
grep "1.0.0.3" dig.out.ns3.test$n > /dev/null || ret=1
grep ::3 dig.out.ns3.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, unsigned, qtype=ANY and DO is set with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned +dnssec -b 10.53.0.3 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
grep "1.0.0.6" dig.out.ns3.test$n > /dev/null || ret=1
grep "::6" dig.out.ns3.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that both A and AAAA are returned when both AAAA and A records exist, qtype=ANY and query source does not match acl, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned -b 10.53.0.1 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
grep 1.0.0.6 dig.out.ns3.test$n > /dev/null || ret=1
grep ::6 dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A record exists, unsigned over IPv6, recursive with break-dnssec ($n)"
if $TESTSOCK6 fd92:7065:b8e:ffff::3
then
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b fd92:7065:b8e:ffff::3 @fd92:7065:b8e:ffff::3 > dig.out.ns3.test$n || ret=1
grep 2001:db8::6 dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
else
echo_i "skipped."
fi

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=NS, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS +add ns unsigned -b 10.53.0.3 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep AAAA dig.out.ns3.test$n > /dev/null 2>&1 && ret=1
grep "ADDITIONAL: 2" dig.out.ns3.test$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=MX, unsigned, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec mx unsigned -b 10.53.0.3 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep "^mx.unsigned.*AAAA" dig.out.ns3.test$n > /dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=MX, signed, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec mx signed -b 10.53.0.3 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep "^mx.signed.*AAAA" dig.out.ns3.test$n > /dev/null 2>&1 && ret=1
grep "flags:.*ad.*QUERY" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is included in additional section, qtype=MX, unsigned, over IPv6, recursive with break-dnssec ($n)"
if $TESTSOCK6 fd92:7065:b8e:ffff::3
then
ret=0
$DIG $DIGOPTS +add +dnssec mx unsigned -b fd92:7065:b8e:ffff::3 @fd92:7065:b8e:ffff::3 > dig.out.ns3.test$n || ret=1
grep "^mx.unsigned.*AAAA" dig.out.ns3.test$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
else
echo_i "skipped."
fi

$TESTSOCK6 fd92:7065:b8e:ffff::1 || {
        echo_i "IPv6 address not configured; skipping IPv6 query tests"
        echo_i "exit status: $status"
        exit $status
}

# Reconfiguring for IPv6 tests
echo_i "reconfiguring servers"
copy_setports ns1/named2.conf.in ns1/named.conf
rndc_reconfig ns1 10.53.0.1
copy_setports ns2/named2.conf.in ns2/named.conf
rndc_reconfig ns2 10.53.0.2
copy_setports ns3/named2.conf.in ns3/named.conf
rndc_reconfig ns3 10.53.0.3
copy_setports ns4/named2.conf.in ns4/named.conf
rndc_reconfig ns4 10.53.0.4

# BEGIN IPv6 TESTS

#
# Authoritative tests against:
#	filter-aaaa-on-v6 yes;
#	filter-aaaa { fd92:7065:b8e:ffff::1; };
#
n=`expr $n + 1`
echo_i "checking that AAAA is returned when only AAAA record exists, signed ($n)"
ret=0
$DIG $DIGOPTS aaaa aaaa-only.signed -b fd92:7065:b8e:ffff::1 @fd92:7065:b8e:ffff::1 > dig.out.ns1.test$n || ret=1
grep ::2 dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when only AAAA record exists, unsigned ($n)"
ret=0
$DIG $DIGOPTS aaaa aaaa-only.unsigned -b fd92:7065:b8e:ffff::1 @fd92:7065:b8e:ffff::1 > dig.out.ns1.test$n || ret=1
grep ::5 dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, signed ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.signed -b fd92:7065:b8e:ffff::1 @fd92:7065:b8e:ffff::1 > dig.out.ns1.test$n || ret=1
grep "ANSWER: 0" dig.out.ns1.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, unsigned ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b fd92:7065:b8e:ffff::1 @fd92:7065:b8e:ffff::1 > dig.out.ns1.test$n || ret=1
grep "ANSWER: 0" dig.out.ns1.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A records exist, signed and DO set ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.signed +dnssec -b fd92:7065:b8e:ffff::1 @fd92:7065:b8e:ffff::1 > dig.out.ns1.test$n || ret=1
grep ::3 dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, unsigned and DO set ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b fd92:7065:b8e:ffff::1 @fd92:7065:b8e:ffff::1 > dig.out.ns1.test$n || ret=1
grep "ANSWER: 0" dig.out.ns1.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A records exist and query source does not match acl ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b fd92:7065:b8e:ffff::2 @fd92:7065:b8e:ffff::1 > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep ::6 dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, signed and qtype=ANY ($n)"
ret=0
$DIG $DIGOPTS any dual.signed -b fd92:7065:b8e:ffff::1 @fd92:7065:b8e:ffff::1 > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "1.0.0.3" dig.out.ns1.test$n > /dev/null || ret=1
grep "::3" dig.out.ns1.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, unsigned and qtype=ANY ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned -b fd92:7065:b8e:ffff::1 @fd92:7065:b8e:ffff::1 > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "1.0.0.6" dig.out.ns1.test$n > /dev/null || ret=1
grep "::6" dig.out.ns1.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that both A and AAAA are returned when both AAAA and A records exist, signed, qtype=ANY and DO is set ($n)"
ret=0
$DIG $DIGOPTS any dual.signed +dnssec -b fd92:7065:b8e:ffff::1 @fd92:7065:b8e:ffff::1 > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep ::3 dig.out.ns1.test$n > /dev/null || ret=1
grep "1.0.0.3" dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, unsigned, qtype=ANY and DO is set ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned +dnssec -b fd92:7065:b8e:ffff::1 @fd92:7065:b8e:ffff::1 > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "1.0.0.6" dig.out.ns1.test$n > /dev/null || ret=1
grep "::6" dig.out.ns1.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that both A and AAAA are returned when both AAAA and A records exist, qtype=ANY and query source does not match acl ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned -b fd92:7065:b8e:ffff::2 @fd92:7065:b8e:ffff::1 > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep 1.0.0.6 dig.out.ns1.test$n > /dev/null || ret=1
grep ::6 dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A record exists, unsigned over IPv4 ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b 10.53.0.1 @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep 2001:db8::6 dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=NS ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec ns unsigned -b fd92:7065:b8e:ffff::1 @fd92:7065:b8e:ffff::1 > dig.out.ns1.test$n || ret=1
grep AAAA dig.out.ns1.test$n > /dev/null 2>&1 && ret=1
grep "ADDITIONAL: 2" dig.out.ns1.test$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=MX, unsigned ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec mx unsigned -b fd92:7065:b8e:ffff::1 @fd92:7065:b8e:ffff::1 > dig.out.ns1.test$n || ret=1
grep "^mx.unsigned.*AAAA" dig.out.ns1.test$n > /dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is included in additional section, qtype=MX, signed ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec mx signed -b fd92:7065:b8e:ffff::1 @fd92:7065:b8e:ffff::1 > dig.out.ns1.test$n || ret=1
grep "^mx.signed.*AAAA" dig.out.ns1.test$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is included in additional section, qtype=MX, unsigned, over IPv4 ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec mx unsigned -b 10.53.0.1 @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep "^mx.unsigned.*AAAA" dig.out.ns1.test$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`


#
# Authoritative tests against:
#	filter-aaaa-on-v6 break-dnssec;
#	filter-aaaa { fd92:7065:b8e:ffff::4; };
#
n=`expr $n + 1`
echo_i "checking that AAAA is returned when only AAAA record exists, signed with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa aaaa-only.signed -b fd92:7065:b8e:ffff::4 @fd92:7065:b8e:ffff::4 > dig.out.ns4.test$n || ret=1
grep ::2 dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when only AAAA record exists, unsigned with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa aaaa-only.unsigned -b fd92:7065:b8e:ffff::4 @fd92:7065:b8e:ffff::4 > dig.out.ns4.test$n || ret=1
grep ::5 dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, signed with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.signed -b fd92:7065:b8e:ffff::4 @fd92:7065:b8e:ffff::4 > dig.out.ns4.test$n || ret=1
grep "ANSWER: 0" dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, unsigned with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b fd92:7065:b8e:ffff::4 @fd92:7065:b8e:ffff::4 > dig.out.ns4.test$n || ret=1
grep "ANSWER: 0" dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, signed and DO set with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.signed +dnssec -b fd92:7065:b8e:ffff::4 @fd92:7065:b8e:ffff::4 > dig.out.ns4.test$n || ret=1
grep "ANSWER: 0" dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, unsigned and DO set with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b fd92:7065:b8e:ffff::4 @fd92:7065:b8e:ffff::4 > dig.out.ns4.test$n || ret=1
grep "ANSWER: 0" dig.out.ns4.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A records exist and query source does not match acl with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b fd92:7065:b8e:ffff::2 @fd92:7065:b8e:ffff::4 > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep ::6 dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, signed and qtype=ANY with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.signed -b fd92:7065:b8e:ffff::4 @fd92:7065:b8e:ffff::4 > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "1.0.0.3" dig.out.ns4.test$n > /dev/null || ret=1
grep "::3" dig.out.ns4.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, unsigned and qtype=ANY with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned -b fd92:7065:b8e:ffff::4 @fd92:7065:b8e:ffff::4 > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "1.0.0.6" dig.out.ns4.test$n > /dev/null || ret=1
grep "::6" dig.out.ns4.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, signed, qtype=ANY and DO is set with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.signed +dnssec -b fd92:7065:b8e:ffff::4 @fd92:7065:b8e:ffff::4 > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "1.0.0.3" dig.out.ns4.test$n > /dev/null || ret=1
grep ::3 dig.out.ns4.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, unsigned, qtype=ANY and DO is set with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned +dnssec -b fd92:7065:b8e:ffff::4 @fd92:7065:b8e:ffff::4 > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "1.0.0.6" dig.out.ns4.test$n > /dev/null || ret=1
grep "::6" dig.out.ns4.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that both A and AAAA are returned when both AAAA and A records exist, qtype=ANY and query source does not match acl with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned -b fd92:7065:b8e:ffff::2 @fd92:7065:b8e:ffff::4 > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep 1.0.0.6 dig.out.ns4.test$n > /dev/null || ret=1
grep ::6 dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A record exists, unsigned over IPv4 with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b 10.53.0.4 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep 2001:db8::6 dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=NS, with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec ns unsigned -b fd92:7065:b8e:ffff::4 @fd92:7065:b8e:ffff::4 > dig.out.ns4.test$n || ret=1
grep AAAA dig.out.ns4.test$n > /dev/null 2>&1 && ret=1
grep "ADDITIONAL: 2" dig.out.ns4.test$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=MX, unsigned, with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec mx unsigned -b fd92:7065:b8e:ffff::4 @fd92:7065:b8e:ffff::4 > dig.out.ns4.test$n || ret=1
grep "^mx.unsigned.*AAAA" dig.out.ns4.test$n > /dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=MX, signed, with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec mx signed -b fd92:7065:b8e:ffff::4 @fd92:7065:b8e:ffff::4 > dig.out.ns4.test$n || ret=1
grep "^mx.signed.*AAAA" dig.out.ns4.test$n > /dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is included in additional section, qtype=MX, unsigned, over IPv4, with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec mx unsigned -b 10.53.0.4 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep "^mx.unsigned.*AAAA" dig.out.ns4.test$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`


#
# Recursive tests against:
#	filter-aaaa-on-v6 yes;
#	filter-aaaa { fd92:7065:b8e:ffff::2; };
#
n=`expr $n + 1`
echo_i "checking that AAAA is returned when only AAAA record exists, signed, recursive ($n)"
ret=0
$DIG $DIGOPTS aaaa aaaa-only.signed -b fd92:7065:b8e:ffff::2 @fd92:7065:b8e:ffff::2 > dig.out.ns2.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null || ret=1
grep ::2 dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when only AAAA record exists, unsigned, recursive ($n)"
ret=0
$DIG $DIGOPTS aaaa aaaa-only.unsigned -b fd92:7065:b8e:ffff::2 @fd92:7065:b8e:ffff::2 > dig.out.ns2.test$n || ret=1
grep ::5 dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, signed, recursive ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.signed -b fd92:7065:b8e:ffff::2 @fd92:7065:b8e:ffff::2 > dig.out.ns2.test$n || ret=1
grep "ANSWER: 0" dig.out.ns2.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null && ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, unsigned, recursive ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b fd92:7065:b8e:ffff::2 @fd92:7065:b8e:ffff::2 > dig.out.ns2.test$n || ret=1
grep "ANSWER: 0" dig.out.ns2.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null && ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A records exist, signed and DO set, recursive ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.signed +dnssec -b fd92:7065:b8e:ffff::2 @fd92:7065:b8e:ffff::2 > dig.out.ns2.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null || ret=1
grep ::3 dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, unsigned and DO set, recursive ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned +dnssec -b fd92:7065:b8e:ffff::2 @fd92:7065:b8e:ffff::2 > dig.out.ns2.test$n || ret=1
grep "ANSWER: 0" dig.out.ns2.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null && ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A records exist and query source does not match acl, recursive ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b fd92:7065:b8e:ffff::1 @fd92:7065:b8e:ffff::2 > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep ::6 dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, signed and qtype=ANY recursive ($n)"
ret=0
$DIG $DIGOPTS any dual.signed -b fd92:7065:b8e:ffff::2 @fd92:7065:b8e:ffff::2 > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "1.0.0.3" dig.out.ns2.test$n > /dev/null || ret=1
grep "::3" dig.out.ns2.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, unsigned and qtype=ANY recursive ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned -b fd92:7065:b8e:ffff::2 @fd92:7065:b8e:ffff::2 > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "1.0.0.6" dig.out.ns2.test$n > /dev/null || ret=1
grep "::6" dig.out.ns2.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that both A and AAAA are returned when both AAAA and A records exist, signed, qtype=ANY and DO is set, recursive ($n)"
ret=0
$DIG $DIGOPTS any dual.signed +dnssec -b fd92:7065:b8e:ffff::2 @fd92:7065:b8e:ffff::2 > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null || ret=1
grep ::3 dig.out.ns2.test$n > /dev/null || ret=1
grep "1.0.0.3" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, unsigned, qtype=ANY and DO is set, recursive ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned +dnssec -b fd92:7065:b8e:ffff::2 @fd92:7065:b8e:ffff::2 > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "1.0.0.6" dig.out.ns2.test$n > /dev/null || ret=1
grep "::6" dig.out.ns2.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that both A and AAAA are returned when both AAAA and A records exist, qtype=ANY and query source does not match acl, recursive ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned -b fd92:7065:b8e:ffff::1 @fd92:7065:b8e:ffff::2 > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep 1.0.0.6 dig.out.ns2.test$n > /dev/null || ret=1
grep ::6 dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A record exists, unsigned over IPv4, recursive ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b 10.53.0.2 @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep 2001:db8::6 dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=NS ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec ns unsigned -b fd92:7065:b8e:ffff::2 @fd92:7065:b8e:ffff::2 > dig.out.ns2.test$n || ret=1
grep AAAA dig.out.ns2.test$n > /dev/null 2>&1 && ret=1
grep "ADDITIONAL: 2" dig.out.ns2.test$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=MX, unsigned ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec mx unsigned -b fd92:7065:b8e:ffff::2 @fd92:7065:b8e:ffff::2 > dig.out.ns2.test$n || ret=1
grep "^mx.unsigned.*AAAA" dig.out.ns2.test$n > /dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is included in additional section, qtype=MX, signed ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec mx signed -b fd92:7065:b8e:ffff::2 @fd92:7065:b8e:ffff::2 > dig.out.ns2.test$n || ret=1
grep "^mx.signed.*AAAA" dig.out.ns2.test$n > /dev/null 2>&1 || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is included in additional section, qtype=MX, unsigned, over IPv4 ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec mx unsigned -b 10.53.0.2 @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep "^mx.unsigned.*AAAA" dig.out.ns2.test$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`


#
# Recursive tests against:
#	filter-aaaa-on-v6 yes;
#	filter-aaaa { fd92:7065:b8e:ffff::3; };
#
n=`expr $n + 1`
echo_i "checking that AAAA is returned when only AAAA record exists, signed, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa aaaa-only.signed -b fd92:7065:b8e:ffff::3 @fd92:7065:b8e:ffff::3 > dig.out.ns3.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns3.test$n > /dev/null || ret=1
grep ::2 dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when only AAAA record exists, unsigned, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa aaaa-only.unsigned -b fd92:7065:b8e:ffff::3 @fd92:7065:b8e:ffff::3 > dig.out.ns3.test$n || ret=1
grep ::5 dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, signed, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.signed -b fd92:7065:b8e:ffff::3 @fd92:7065:b8e:ffff::3 > dig.out.ns3.test$n || ret=1
grep "ANSWER: 0" dig.out.ns3.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns3.test$n > /dev/null && ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, unsigned, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b fd92:7065:b8e:ffff::3 @fd92:7065:b8e:ffff::3 > dig.out.ns3.test$n || ret=1
grep "ANSWER: 0" dig.out.ns3.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns3.test$n > /dev/null && ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, signed and DO set, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.signed +dnssec -b fd92:7065:b8e:ffff::3 @fd92:7065:b8e:ffff::3 > dig.out.ns3.test$n || ret=1
grep "ANSWER: 0" dig.out.ns3.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns3.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NODATA/NOERROR is returned when both AAAA and A records exist, unsigned and DO set, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned +dnssec -b fd92:7065:b8e:ffff::3 @fd92:7065:b8e:ffff::3 > dig.out.ns3.test$n || ret=1
grep "ANSWER: 0" dig.out.ns3.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns3.test$n > /dev/null && ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A records exist and query source does not match acl, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b fd92:7065:b8e:ffff::1 @fd92:7065:b8e:ffff::3 > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
grep ::6 dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, signed and qtype=ANY with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.signed -b fd92:7065:b8e:ffff::3 @fd92:7065:b8e:ffff::3 > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
grep "1.0.0.3" dig.out.ns3.test$n > /dev/null || ret=1
grep "::3" dig.out.ns3.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, unsigned and qtype=ANY with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned -b fd92:7065:b8e:ffff::3 @fd92:7065:b8e:ffff::3 > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
grep "1.0.0.6" dig.out.ns3.test$n > /dev/null || ret=1
grep "::6" dig.out.ns3.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, signed, qtype=ANY and DO is set with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.signed +dnssec -b fd92:7065:b8e:ffff::3 @fd92:7065:b8e:ffff::3 > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
grep "1.0.0.3" dig.out.ns3.test$n > /dev/null || ret=1
grep ::3 dig.out.ns3.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that A and not AAAA is returned when both AAAA and A records exist, unsigned, qtype=ANY and DO is set with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned +dnssec -b fd92:7065:b8e:ffff::3 @fd92:7065:b8e:ffff::3 > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
grep "1.0.0.6" dig.out.ns3.test$n > /dev/null || ret=1
grep "::6" dig.out.ns3.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that both A and AAAA are returned when both AAAA and A records exist, qtype=ANY and query source does not match acl, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS any dual.unsigned -b fd92:7065:b8e:ffff::1 @fd92:7065:b8e:ffff::3 > dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
grep 1.0.0.6 dig.out.ns3.test$n > /dev/null || ret=1
grep ::6 dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is returned when both AAAA and A record exists, unsigned over IPv4, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS aaaa dual.unsigned -b 10.53.0.3 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep 2001:db8::6 dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=NS, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec ns unsigned -b fd92:7065:b8e:ffff::3 @fd92:7065:b8e:ffff::3 > dig.out.ns3.test$n || ret=1
grep AAAA dig.out.ns3.test$n > /dev/null 2>&1 && ret=1
grep "ADDITIONAL: 2" dig.out.ns3.test$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=MX, unsigned, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec mx unsigned -b fd92:7065:b8e:ffff::3 @fd92:7065:b8e:ffff::3 > dig.out.ns3.test$n || ret=1
grep "^mx.unsigned.*AAAA" dig.out.ns3.test$n > /dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is omitted from additional section, qtype=MX, signed, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec mx signed -b fd92:7065:b8e:ffff::3 @fd92:7065:b8e:ffff::3 > dig.out.ns3.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns3.test$n > /dev/null || ret=1
grep "^mx.signed.*AAAA" dig.out.ns3.test$n > /dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that AAAA is included in additional section, qtype=MX, unsigned, over IPv4, recursive with break-dnssec ($n)"
ret=0
$DIG $DIGOPTS +add +dnssec mx unsigned -b 10.53.0.3 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep "^mx.unsigned.*AAAA" dig.out.ns3.test$n > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# We don't check for the AAAA record here as configuration in ns5 does
# not make sense.  The AAAA record is wanted by filter-aaaa but discarded
# by the dns64 configuration. We just want to ensure the server stays
# running.
n=`expr $n + 1`
echo_i "checking filter-aaaa with dns64 ($n)"
ret=0
$DIG $DIGOPTS aaaa aaaa-only.unsigned @10.53.0.5 > dig.out.ns5.test$n || ret=1
grep "status: NOERROR" dig.out.ns5.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
