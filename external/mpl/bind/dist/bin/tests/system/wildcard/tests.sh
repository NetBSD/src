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

rm -f dig.out.*

DIGOPTS="+tcp +noadd +nosea +nostat +nocmd +dnssec -p ${PORT}"

n=`expr $n + 1`
echo_i "checking that NSEC wildcard non-existence proof is returned auth ($n)"
ret=0
$DIG $DIGOPTS a b.wild.nsec +norec @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep -i 'a\.wild\.nsec\..*NSEC.*nsec\..*NSEC'  dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NSEC wildcard non-existence proof is returned non-validating ($n)"
ret=0
$DIG $DIGOPTS a b.wild.nsec @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep -i 'a\.wild\.nsec\..*NSEC.*nsec\..*NSEC'  dig.out.ns2.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns2.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NSEC wildcard non-existence proof is returned validating ($n)"
ret=0
$DIG $DIGOPTS a b.wild.nsec @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep -i 'a\.wild\.nsec\..*NSEC.*nsec\..*NSEC'  dig.out.ns3.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NSEC wildcard non-existence proof is returned validating + CD ($n)"
ret=0
$DIG $DIGOPTS +cd a b.wild.nsec @10.53.0.5 > dig.out.ns5.test$n || ret=1
grep -i 'a\.wild\.nsec\..*NSEC.*nsec\..*NSEC'  dig.out.ns5.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns5.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "checking that returned NSEC wildcard non-existence proof validates ($n)"
ret=0
$DIG $DIGOPTS a b.wild.nsec @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep -i 'a\.wild\.nsec\..*NSEC.*nsec\..*NSEC'  dig.out.ns4.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NSEC wildcard non-existence proof is returned private, validating ($n)"
ret=0
$DIG $DIGOPTS a b.wild.private.nsec @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep -i 'a\.wild\.private\.nsec\..*NSEC.*private\.nsec\..*NSEC'  dig.out.ns3.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns3.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that returned NSEC wildcard non-existence proof for private zone validates ($n)"
ret=0
$DIG $DIGOPTS a b.wild.private.nsec @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep -i 'a\.wild\.private\.nsec\..*NSEC.*private\.nsec\..*NSEC'  dig.out.ns4.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NSEC3 wildcard non-existence proof is returned auth ($n)"
ret=0
$DIG $DIGOPTS a b.wild.nsec3 +norec @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep -i 'O3TJ8D9AJ54CBTFCQCJ3QK49CH7SF6H9\.nsec3\..*V5DLFB6UJNHR94LQ61FO607KGK12H88A'  dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NSEC3 wildcard non-existence proof is returned non-validating ($n)"
ret=0
$DIG $DIGOPTS a b.wild.nsec3 @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep -i 'O3TJ8D9AJ54CBTFCQCJ3QK49CH7SF6H9\.nsec3\..*V5DLFB6UJNHR94LQ61FO607KGK12H88A'  dig.out.ns2.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns2.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NSEC3 wildcard non-existence proof is returned validating ($n)"
ret=0
$DIG $DIGOPTS a b.wild.nsec3 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep -i 'O3TJ8D9AJ54CBTFCQCJ3QK49CH7SF6H9\.nsec3\..*V5DLFB6UJNHR94LQ61FO607KGK12H88A'  dig.out.ns3.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NSEC3 wildcard non-existence proof is returned validating + CD ($n)"
ret=0
$DIG $DIGOPTS +cd a b.wild.nsec3 @10.53.0.5 > dig.out.ns5.test$n || ret=1
grep -i 'O3TJ8D9AJ54CBTFCQCJ3QK49CH7SF6H9\.nsec3\..*V5DLFB6UJNHR94LQ61FO607KGK12H88A'  dig.out.ns5.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns5.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that returned NSEC3 wildcard non-existence proof validates ($n)"
ret=0
$DIG $DIGOPTS a b.wild.nsec3 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep -i 'O3TJ8D9AJ54CBTFCQCJ3QK49CH7SF6H9\.nsec3\..*V5DLFB6UJNHR94LQ61FO607KGK12H88A'  dig.out.ns4.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NSEC3 wildcard non-existence proof is returned private, validating ($n)"
ret=0
$DIG $DIGOPTS a b.wild.private.nsec3 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep -i 'UDBSP4R8OUOT6HSO39VD8B5LMOSHRD5N\.private\.nsec3\..*NSEC3.*ASDRUIB7GO00OR92S5OUGI404LT27RNU' dig.out.ns3.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns3.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that returned NSEC3 wildcard non-existence proof for private zone validates ($n)"
ret=0
$DIG $DIGOPTS a b.wild.private.nsec3 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep -i 'UDBSP4R8OUOT6HSO39VD8B5LMOSHRD5N\.private\.nsec3\..*NSEC3.*ASDRUIB7GO00OR92S5OUGI404LT27RNU' dig.out.ns4.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking RFC 4592 responses ..."

n=`expr $n + 1`
echo_i "checking RFC 4592: host3.example. QTYPE=MX, QCLASS=IN ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.1 host3.example. MX IN > dig.out.ns1.test$n || ret=1
grep '^host3.example..*IN.MX.10 host1.example.' dig.out.ns1.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking RFC 4592: host3.example. QTYPE=A, QCLASS=IN ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.1 host3.example. A IN > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking RFC 4592: foo.bar.example. QTYPE=TXT, QCLASS=IN ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.1 foo.bar.example TXT IN > dig.out.ns1.test$n || ret=1
grep '^foo.bar.example..*IN.TXT."this is a wildcard"' dig.out.ns1.test$n > /dev/null || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking RFC 4592: host1.example. QTYPE=MX, QCLASS=IN ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.1 host1.example MX IN > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking RFC 4592: host1.example. QTYPE=MX, QCLASS=IN ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.1 host1.example MX IN > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking RFC 4592: sub.*.example. QTYPE=MX, QCLASS=IN ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.1 "sub.*.example." MX IN > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking RFC 4592: _telnet._tcp.host1.example. QTYPE=SRV, QCLASS=IN ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.1 _telnet._tcp.host1.example. SRV IN > dig.out.ns1.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns1.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking RFC 4592: host.subdel.example. QTYPE=A, QCLASS=IN ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.1 host.subdel.example A IN > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.ns1.test$n > /dev/null || ret=1
grep "AUTHORITY: 2," dig.out.ns1.test$n > /dev/null || ret=1
grep "subdel.example..*IN.NS.ns.example.com." dig.out.ns1.test$n > /dev/null || ret=1
grep "subdel.example..*IN.NS.ns.example.net." dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking RFC 4592: ghost.*.example. QTYPE=MX, QCLASS=IN ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.1 "ghost.*.example" MX IN > dig.out.ns1.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns1.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check wild card expansions by code point ($n)"
ret=0
i=0
while test $i -lt 256
do
	x=`expr 00$i : '.*\(...\)$'`
	$DIG $DIGOPTS @10.53.0.1 "\\$x.example" TXT > dig.out.ns1.$x.test$n
	if test $i -le 32 -o $i -ge 127
	then
		grep '^\\'"$x"'\.example\..*TXT.*"this is a wildcard"$' dig.out.ns1.$x.test$n > /dev/null || { echo_i "code point $x failed" ; ret=1; }
        # "=34 $=36 (=40 )=41 .=46 ;=59 \=92 @=64
	elif test $i -eq 34 -o $i -eq 36 -o $i -eq 40 -o $i -eq 41 -o \
                  $i -eq 46 -o $i -eq 59 -o $i -eq 64 -o $i -eq 92
	then
		case $i in
		34) a='"';;
		36) a='$';;
		40) a='(';;
		41) a=')';;
		46) a='\.';;
		59) a=';';;
		64) a='@';;
		92) a='\\';;
		*) a=''; echo_i "code point $x failed" ; ret=1 ;;
		esac
		grep '^\\'"$a"'\.example.*.*TXT.*"this is a wildcard"$' dig.out.ns1.$x.test$n > /dev/null || { echo_i "code point $x failed" ; ret=1; }
	else
		grep '^\\' dig.out.ns1.$x.test$n && { echo_i "code point $x failed" ; ret=1; }
	fi
	i=`expr $i + 1`
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
