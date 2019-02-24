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
echo .

DIGOPTS="-p ${PORT}"
RESOLVOPTS="-p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

status=0
n=0

n=`expr $n + 1`
echo_i "checking non-cachable NXDOMAIN response handling ($n)"
ret=0
$DIG $DIGOPTS +tcp nxdomain.example.net @10.53.0.1 a > dig.out.ns1.test${n} || ret=1
grep "status: NXDOMAIN" dig.out.ns1.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${RESOLVE} ] ; then
   n=`expr $n + 1`
   echo_i "checking non-cachable NXDOMAIN response handling using dns_client ($n)"
   ret=0
   $RESOLVE $RESOLVOPTS -t a -s 10.53.0.1 nxdomain.example.net 2> resolve.out.ns1.test${n} || ret=1
   grep "resolution failed: ncache nxdomain" resolve.out.ns1.test${n} > /dev/null || ret=1
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

if [ -x ${RESOLVE} ] ; then
   n=`expr $n + 1`
   echo_i "checking that local bound address can be set (Can't query from a denied address) ($n)"
   ret=0
   ${RESOLVE} -b 10.53.0.8 $RESOLVOPTS -t a -s 10.53.0.1 www.example.org 2> resolve.out.ns1.test${n} || ret=1
   grep "resolution failed: SERVFAIL" resolve.out.ns1.test${n} > /dev/null || ret=1
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`

   n=`expr $n + 1`
   echo_i "checking that local bound address can be set (Can query from an allowed address) ($n)"
   ret=0
   ${RESOLVE} -b 10.53.0.1 $RESOLVOPTS -t a -s 10.53.0.1 www.example.org > resolve.out.ns1.test${n} || ret=1
   grep "www.example.org..*.192.0.2.1" resolve.out.ns1.test${n} > /dev/null || ret=1
   if [ $ret != 0 ]; then echo_i "failed"; fi
   status=`expr $status + $ret`
fi

n=`expr $n + 1`
echo_i "checking non-cachable NODATA response handling ($n)"
ret=0
$DIG $DIGOPTS +tcp nodata.example.net @10.53.0.1 a > dig.out.ns1.test${n} || ret=1
grep "status: NOERROR" dig.out.ns1.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${RESOLVE} ] ; then
    n=`expr $n + 1`
    echo_i "checking non-cachable NODATA response handling using dns_client ($n)"
    ret=0
    $RESOLVE $RESOLVOPTS -t a -s 10.53.0.1 nodata.example.net 2> resolve.out.ns1.test${n} || ret=1
    grep "resolution failed: ncache nxrrset" resolve.out.ns1.test${n} > /dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
fi

n=`expr $n + 1`
echo_i "checking handling of bogus referrals ($n)"
# If the server has the "INSIST(!external)" bug, this query will kill it.
$DIG $DIGOPTS +tcp www.example.com. a @10.53.0.1 >/dev/null || { echo_i "failed"; status=`expr $status + 1`; }

if [ -x ${RESOLVE} ] ; then
    n=`expr $n + 1`
    echo_i "checking handling of bogus referrals using dns_client ($n)"
    ret=0
    $RESOLVE $RESOLVOPTS -t a -s 10.53.0.1 www.example.com 2> resolve.out.ns1.test${n} || ret=1
    grep "resolution failed: SERVFAIL" resolve.out.ns1.test${n} > /dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
fi

n=`expr $n + 1`
echo_i "check handling of cname + other data / 1 ($n)"
$DIG $DIGOPTS +tcp cname1.example.com. a @10.53.0.1 >/dev/null || { echo_i "failed"; status=`expr $status + 1`; }

n=`expr $n + 1`
echo_i "check handling of cname + other data / 2 ($n)"
$DIG $DIGOPTS +tcp cname2.example.com. a @10.53.0.1 >/dev/null || { echo_i "failed"; status=`expr $status + 1`; }

n=`expr $n + 1`
echo_i "check that server is still running ($n)"
$DIG $DIGOPTS +tcp www.example.com. a @10.53.0.1 >/dev/null || { echo_i "failed"; status=`expr $status + 1`; }

n=`expr $n + 1`
echo_i "checking answer IPv4 address filtering (deny) ($n)"
ret=0
$DIG $DIGOPTS +tcp www.example.net @10.53.0.1 a > dig.out.ns1.test${n} || ret=1
grep "status: SERVFAIL" dig.out.ns1.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking answer IPv6 address filtering (deny) ($n)"
ret=0
$DIG $DIGOPTS +tcp www.example.net @10.53.0.1 aaaa > dig.out.ns1.test${n} || ret=1
grep "status: SERVFAIL" dig.out.ns1.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking answer IPv4 address filtering (accept) ($n)"
ret=0
$DIG $DIGOPTS +tcp www.example.org @10.53.0.1 a > dig.out.ns1.test${n} || ret=1
grep "status: NOERROR" dig.out.ns1.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`


if [ -x ${RESOLVE} ] ; then
    n=`expr $n + 1`
    echo_i "checking answer IPv4 address filtering using dns_client (accept) ($n)"
    ret=0
    $RESOLVE $RESOLVOPTS -t a -s 10.53.0.1 www.example.org > resolve.out.ns1.test${n} || ret=1
    grep "www.example.org..*.192.0.2.1" resolve.out.ns1.test${n} > /dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
fi

n=`expr $n + 1`
echo_i "checking answer IPv6 address filtering (accept) ($n)"
ret=0
$DIG $DIGOPTS +tcp www.example.org @10.53.0.1 aaaa > dig.out.ns1.test${n} || ret=1
grep "status: NOERROR" dig.out.ns1.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${RESOLVE} ] ; then
    n=`expr $n + 1`
    echo_i "checking answer IPv6 address filtering using dns_client (accept) ($n)"
    ret=0
    $RESOLVE $RESOLVOPTS -t aaaa -s 10.53.0.1 www.example.org > resolve.out.ns1.test${n} || ret=1
    grep "www.example.org..*.2001:db8:beef::1" resolve.out.ns1.test${n} > /dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
fi

n=`expr $n + 1`
echo_i "checking CNAME target filtering (deny) ($n)"
ret=0
$DIG $DIGOPTS +tcp badcname.example.net @10.53.0.1 a > dig.out.ns1.test${n} || ret=1
grep "status: SERVFAIL" dig.out.ns1.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking CNAME target filtering (accept) ($n)"
ret=0
$DIG $DIGOPTS +tcp goodcname.example.net @10.53.0.1 a > dig.out.ns1.test${n} || ret=1
grep "status: NOERROR" dig.out.ns1.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${RESOLVE} ] ; then
    n=`expr $n + 1`
    echo_i "checking CNAME target filtering using dns_client (accept) ($n)"
    ret=0
    $RESOLVE $RESOLVOPTS -t a -s 10.53.0.1 goodcname.example.net > resolve.out.ns1.test${n} || ret=1
    grep "goodcname.example.net..*.goodcname.example.org." resolve.out.ns1.test${n} > /dev/null || ret=1
    grep "goodcname.example.org..*.192.0.2.1" resolve.out.ns1.test${n} > /dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
fi

n=`expr $n + 1`
echo_i "checking CNAME target filtering (accept due to subdomain) ($n)"
ret=0
$DIG $DIGOPTS +tcp cname.sub.example.org @10.53.0.1 a > dig.out.ns1.test${n} || ret=1
grep "status: NOERROR" dig.out.ns1.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${RESOLVE} ] ; then
    n=`expr $n + 1`
    echo_i "checking CNAME target filtering using dns_client (accept due to subdomain) ($n)"
    ret=0
    $RESOLVE $RESOLVOPTS -t a -s 10.53.0.1 cname.sub.example.org > resolve.out.ns1.test${n} || ret=1
    grep "cname.sub.example.org..*.ok.sub.example.org." resolve.out.ns1.test${n} > /dev/null || ret=1
    grep "ok.sub.example.org..*.192.0.2.1" resolve.out.ns1.test${n} > /dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
fi

n=`expr $n + 1`
echo_i "checking DNAME target filtering (deny) ($n)"
ret=0
$DIG $DIGOPTS +tcp foo.baddname.example.net @10.53.0.1 a > dig.out.ns1.test${n} || ret=1
grep "DNAME target foo.baddname.example.org denied for foo.baddname.example.net/IN" ns1/named.run >/dev/null || ret=1
grep "status: SERVFAIL" dig.out.ns1.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking DNAME target filtering (accept) ($n)"
ret=0
$DIG $DIGOPTS +tcp foo.gooddname.example.net @10.53.0.1 a > dig.out.ns1.test${n} || ret=1
grep "status: NOERROR" dig.out.ns1.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${RESOLVE} ] ; then
    n=`expr $n + 1`
    echo_i "checking DNAME target filtering using dns_client (accept) ($n)"
    ret=0
    $RESOLVE $RESOLVOPTS -t a -s 10.53.0.1 foo.gooddname.example.net > resolve.out.ns1.test${n} || ret=1
    grep "foo.gooddname.example.net..*.gooddname.example.org" resolve.out.ns1.test${n} > /dev/null || ret=1
    grep "foo.gooddname.example.org..*.192.0.2.1" resolve.out.ns1.test${n} > /dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
fi

n=`expr $n + 1`
echo_i "checking DNAME target filtering (accept due to subdomain) ($n)"
ret=0
$DIG $DIGOPTS +tcp www.dname.sub.example.org @10.53.0.1 a > dig.out.ns1.test${n} || ret=1
grep "status: NOERROR" dig.out.ns1.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x ${RESOLVE} ] ; then
    n=`expr $n + 1`
    echo_i "checking DNAME target filtering using dns_client (accept due to subdomain) ($n)"
    ret=0
    $RESOLVE $RESOLVOPTS -t a -s 10.53.0.1 www.dname.sub.example.org > resolve.out.ns1.test${n} || ret=1
    grep "www.dname.sub.example.org..*.ok.sub.example.org." resolve.out.ns1.test${n} > /dev/null || ret=1
    grep "www.ok.sub.example.org..*.192.0.2.1" resolve.out.ns1.test${n} > /dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
fi

n=`expr $n + 1`
echo_i "check that the resolver accepts a referral response with a non-empty ANSWER section ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.1 foo.glue-in-answer.example.org. A > dig.ns1.out.${n} || ret=1
grep "status: NOERROR" dig.ns1.out.${n} > /dev/null || ret=1
grep "foo.glue-in-answer.example.org.*192.0.2.1" dig.ns1.out.${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "RT21594 regression test check setup ($n)"
ret=0
# Check that "aa" is not being set by the authoritative server.
$DIG $DIGOPTS +tcp . @10.53.0.4 soa > dig.ns4.out.${n} || ret=1
grep 'flags: qr rd;' dig.ns4.out.${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "RT21594 regression test positive answers ($n)"
ret=0
# Check that resolver accepts the non-authoritative positive answers.
$DIG $DIGOPTS +tcp . @10.53.0.5 soa > dig.ns5.out.${n} || ret=1
grep "status: NOERROR" dig.ns5.out.${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "RT21594 regression test NODATA answers ($n)"
ret=0
# Check that resolver accepts the non-authoritative nodata answers.
$DIG $DIGOPTS +tcp . @10.53.0.5 txt > dig.ns5.out.${n} || ret=1
grep "status: NOERROR" dig.ns5.out.${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "RT21594 regression test NXDOMAIN answers ($n)"
ret=0
# Check that resolver accepts the non-authoritative positive answers.
$DIG $DIGOPTS +tcp noexistant @10.53.0.5 txt > dig.ns5.out.${n} || ret=1
grep "status: NXDOMAIN" dig.ns5.out.${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that replacement of additional data by a negative cache no data entry clears the additional RRSIGs ($n)"
ret=0
$DIG $DIGOPTS +tcp mx example.net @10.53.0.7 > dig.ns7.out.${n} || ret=1
grep "status: NOERROR" dig.ns7.out.${n} > /dev/null || ret=1
if [ $ret = 1 ]; then echo_i "mx priming failed"; fi
$NSUPDATE << EOF
server 10.53.0.6 ${PORT}
zone example.net
update delete mail.example.net A
update add mail.example.net 0 AAAA ::1
send
EOF
$DIG $DIGOPTS +tcp a mail.example.net @10.53.0.7 > dig.ns7.out.${n} || ret=2
grep "status: NOERROR" dig.ns7.out.${n} > /dev/null || ret=2
grep "ANSWER: 0" dig.ns7.out.${n} > /dev/null || ret=2
if [ $ret = 2 ]; then echo_i "ncache priming failed"; fi
$DIG $DIGOPTS +tcp mx example.net @10.53.0.7 > dig.ns7.out.${n} || ret=3
grep "status: NOERROR" dig.ns7.out.${n} > /dev/null || ret=3
$DIG $DIGOPTS +tcp rrsig mail.example.net +norec @10.53.0.7 > dig.ns7.out.${n}  || ret=4
grep "status: NOERROR" dig.ns7.out.${n} > /dev/null || ret=4
grep "ANSWER: 0" dig.ns7.out.${n} > /dev/null || ret=4
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that update a nameservers address has immediate effects ($n)"
ret=0
$DIG $DIGOPTS +tcp TXT foo.moves @10.53.0.7 > dig.ns7.foo.${n} || ret=1
grep "From NS 5" dig.ns7.foo.${n} > /dev/null || ret=1
$NSUPDATE << EOF
server 10.53.0.7 ${PORT}
zone server
update delete ns.server A
update add ns.server 300 A 10.53.0.4
send
EOF
sleep 1
$DIG $DIGOPTS +tcp TXT bar.moves @10.53.0.7 > dig.ns7.bar.${n} || ret=1
grep "From NS 4" dig.ns7.bar.${n} > /dev/null || ret=1

if [ $ret != 0 ]; then echo_i "failed"; status=1; fi

n=`expr $n + 1`
echo_i "checking that update a nameservers glue has immediate effects ($n)"
ret=0
$DIG $DIGOPTS +tcp TXT foo.child.server @10.53.0.7 > dig.ns7.foo.${n} || ret=1
grep "From NS 5" dig.ns7.foo.${n} > /dev/null || ret=1
$NSUPDATE << EOF
server 10.53.0.7 ${PORT}
zone server
update delete ns.child.server A
update add ns.child.server 300 A 10.53.0.4
send
EOF
sleep 1
$DIG $DIGOPTS +tcp TXT bar.child.server @10.53.0.7 > dig.ns7.bar.${n} || ret=1
grep "From NS 4" dig.ns7.bar.${n} > /dev/null || ret=1

if [ $ret != 0 ]; then echo_i "failed"; status=1; fi

n=`expr $n + 1`
echo_i "checking empty RFC 1918 reverse zones ($n)"
ret=0
# Check that "aa" is being set by the resolver for RFC 1918 zones
# except the one that has been deliberately disabled
$DIG $DIGOPTS @10.53.0.7 -x 10.1.1.1 > dig.ns4.out.1.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.1.${n} > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.7 -x 192.168.1.1 > dig.ns4.out.2.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.2.${n} > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.7 -x 172.16.1.1  > dig.ns4.out.3.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.3.${n} > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.7 -x 172.17.1.1 > dig.ns4.out.4.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.4.${n} > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.7 -x 172.18.1.1 > dig.ns4.out.5.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.5.${n} > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.7 -x 172.19.1.1 > dig.ns4.out.6.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.6.${n} > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.7 -x 172.21.1.1 > dig.ns4.out.7.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.7.${n} > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.7 -x 172.22.1.1 > dig.ns4.out.8.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.8.${n} > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.7 -x 172.23.1.1 > dig.ns4.out.9.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.9.${n} > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.7 -x 172.24.1.1 > dig.ns4.out.11.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.11.${n} > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.7 -x 172.25.1.1 > dig.ns4.out.12.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.12.${n} > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.7 -x 172.26.1.1 > dig.ns4.out.13.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.13.${n} > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.7 -x 172.27.1.1 > dig.ns4.out.14.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.14.${n} > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.7 -x 172.28.1.1 > dig.ns4.out.15.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.15.${n} > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.7 -x 172.29.1.1 > dig.ns4.out.16.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.16.${n} > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.7 -x 172.30.1.1 > dig.ns4.out.17.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.17.${n} > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.7 -x 172.31.1.1 > dig.ns4.out.18.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.18.${n} > /dev/null || ret=1
# but this one should NOT be authoritative
$DIG $DIGOPTS @10.53.0.7 -x 172.20.1.1 > dig.ns4.out.19.${n} || ret=1
grep 'flags: qr rd ra;' dig.ns4.out.19.${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; status=1; fi

n=`expr $n + 1`
echo_i "checking that removal of a delegation is honoured ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.5 www.to-be-removed.tld A > dig.ns5.prime.${n}
grep "status: NOERROR" dig.ns5.prime.${n} > /dev/null || { ret=1; echo_i "priming failed"; }
cp ns4/tld2.db ns4/tld.db
rndc_reload ns4 10.53.0.4 tld
old=
for i in 0 1 2 3 4 5 6 7 8 9
do
	foo=0
	$DIG $DIGOPTS @10.53.0.5 ns$i.to-be-removed.tld A > /dev/null
	$DIG $DIGOPTS @10.53.0.5 www.to-be-removed.tld A > dig.ns5.out.${n}
	grep "status: NXDOMAIN" dig.ns5.out.${n} > /dev/null || foo=1
	[ $foo = 0 ] && break
	$NSUPDATE << EOF
server 10.53.0.6 ${PORT}
zone to-be-removed.tld
update add to-be-removed.tld 100 NS ns${i}.to-be-removed.tld
update delete to-be-removed.tld NS ns${old}.to-be-removed.tld
send
EOF
	old=$i
	sleep 1
done
[ $ret = 0 ] && ret=$foo;
if [ $ret != 0 ]; then echo_i "failed"; status=1; fi

n=`expr $n + 1`
echo_i "check for improved error message with SOA mismatch ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.1 www.sub.broken aaaa > dig.out.ns1.test${n} || ret=1
grep "not subdomain of zone" ns1/named.run > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

copy_setports ns7/named2.conf.in ns7/named.conf
$RNDCCMD 10.53.0.7 reconfig 2>&1 | sed 's/^/ns7 /' | cat_i

n=`expr $n + 1`
echo_i "check resolution on the listening port ($n)"
ret=0
$DIG $DIGOPTS +tcp +tries=2 +time=5 mx example.net @10.53.0.7 > dig.ns7.out.${n} || ret=2
grep "status: NOERROR" dig.ns7.out.${n} > /dev/null || ret=1
grep "ANSWER: 1" dig.ns7.out.${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check prefetch (${n})"
ret=0
$DIG $DIGOPTS @10.53.0.5 fetch.tld txt > dig.out.1.${n} || ret=1
ttl1=`awk '/"A" "short" "ttl"/ { print $2 - 2 }' dig.out.1.${n}`
# sleep so we are in prefetch range
sleep ${ttl1:-0}
# trigger prefetch
$DIG $DIGOPTS @10.53.0.5 fetch.tld txt > dig.out.2.${n} || ret=1
ttl2=`awk '/"A" "short" "ttl"/ { print $2 }' dig.out.2.${n}`
sleep 1
# check that prefetch occured
$DIG $DIGOPTS @10.53.0.5 fetch.tld txt > dig.out.3.${n} || ret=1
ttl=`awk '/"A" "short" "ttl"/ { print $2 }' dig.out.3.${n}`
test ${ttl:-0} -gt ${ttl2:-1} || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check prefetch of validated DS's RRSIG TTL is updated (${n})"
ret=0
$DIG $DIGOPTS +dnssec @10.53.0.5 ds.example.net ds > dig.out.1.${n} || ret=1
ttl1=`awk '$4 == "DS" && $7 == "1" { print $2 - 2 }' dig.out.1.${n}`
# sleep so we are in prefetch range
sleep ${ttl1:-0}
# trigger prefetch
$DIG $DIGOPTS @10.53.0.5 ds.example.net ds > dig.out.2.${n} || ret=1
ttl1=`awk '$4 == "DS" && $7 == "1" { print $2 }' dig.out.2.${n}`
sleep 1
# check that prefetch occured
$DIG $DIGOPTS @10.53.0.5 ds.example.net ds +dnssec > dig.out.3.${n} || ret=1
dsttl=`awk '$4 == "DS" && $7 == "1" { print $2 }' dig.out.3.${n}`
sigttl=`awk '$4 == "RRSIG" && $5 == "DS" { print $2 }' dig.out.3.${n}`
test ${dsttl:-0} -gt ${ttl2:-1} || ret=1
test ${sigttl:-0} -gt ${ttl2:-1} || ret=1
test ${dsttl:-0} -eq ${sigttl:-1} || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check prefetch disabled (${n})"
ret=0
$DIG $DIGOPTS @10.53.0.7 fetch.example.net txt > dig.out.1.${n} || ret=1
ttl1=`awk '/"A" "short" "ttl"/ { print $2 - 2 }' dig.out.1.${n}`
# sleep so we are in expire range
sleep ${ttl1:-0}
# look for ttl = 1, allow for one miss at getting zero ttl
zerotonine="0 1 2 3 4 5 6 7 8 9"
for i in $zerotonine $zerotonine $zerotonine $zerotonine
do
	$DIG $DIGOPTS @10.53.0.7 fetch.example.net txt > dig.out.2.${n} || ret=1
	ttl2=`awk '/"A" "short" "ttl"/ { print $2 }' dig.out.2.${n}`
	test ${ttl2:-2} -eq 1 && break
	$PERL -e 'select(undef, undef, undef, 0.05);'
done
test ${ttl2:-2} -eq 1 || ret=1
# delay so that any prefetched record will have a lower ttl than expected
sleep 3
# check that prefetch has not occured
$DIG $DIGOPTS @10.53.0.7 fetch.example.net txt > dig.out.3.${n} || ret=1
ttl=`awk '/"A" "short" "ttl"/ { print $2 - 2 }' dig.out.3.${n}`
test ${ttl:-0} -eq ${ttl1:-1} || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check prefetch qtype * (${n})"
ret=0
$DIG $DIGOPTS @10.53.0.5 fetchall.tld any > dig.out.1.${n} || ret=1
ttl1=`awk '/"A" "short" "ttl"/ { print $2 - 2 }' dig.out.1.${n}`
# sleep so we are in prefetch range
sleep ${ttl1:-0}
# trigger prefetch
$DIG $DIGOPTS @10.53.0.5 fetchall.tld any > dig.out.2.${n} || ret=1
ttl2=`awk '/"A" "short" "ttl"/ { print $2 }' dig.out.2.${n}`
sleep 1
# check that the nameserver is still alive
$DIG $DIGOPTS @10.53.0.5 fetchall.tld any > dig.out.3.${n} || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that E was logged on EDNS queries in the query log (${n})"
ret=0
$DIG $DIGOPTS @10.53.0.5 +edns edns.fetchall.tld any > dig.out.2.${n} || ret=1
grep "query: edns.fetchall.tld IN ANY +E" ns5/named.run > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.5 +noedns noedns.fetchall.tld any > dig.out.2.${n} || ret=1
grep "query: noedns.fetchall.tld IN ANY" ns5/named.run > /dev/null || ret=1
grep "query: noedns.fetchall.tld IN ANY +E" ns5/named.run > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that '-t aaaa' in .digrc does not have unexpected side effects ($n)"
ret=0
echo "-t aaaa" > .digrc
env HOME=`pwd` $DIG $DIGOPTS @10.53.0.4 . > dig.out.1.${n} || ret=1
env HOME=`pwd` $DIG $DIGOPTS @10.53.0.4 . A > dig.out.2.${n} || ret=1
env HOME=`pwd` $DIG $DIGOPTS @10.53.0.4 -x 127.0.0.1 > dig.out.3.${n} || ret=1
grep ';\..*IN.*AAAA$' dig.out.1.${n} > /dev/null || ret=1
grep ';\..*IN.*A$' dig.out.2.${n} > /dev/null || ret=1
grep 'extra type option' dig.out.2.${n} > /dev/null && ret=1
grep ';1\.0\.0\.127\.in-addr\.arpa\..*IN.*PTR$' dig.out.3.${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

edns=`$FEATURETEST --edns-version`

n=`expr $n + 1`
echo_i "check that EDNS version is logged (${n})"
ret=0
$DIG $DIGOPTS @10.53.0.5 +edns edns0.fetchall.tld any > dig.out.2.${n} || ret=1
grep "query: edns0.fetchall.tld IN ANY +E(0)" ns5/named.run > /dev/null || ret=1
if test ${edns:-0} != 0; then
    $DIG $DIGOPTS @10.53.0.5 +edns=1 edns1.fetchall.tld any > dig.out.2.${n} || ret=1
    grep "query: edns1.fetchall.tld IN ANY +E(1)" ns5/named.run > /dev/null || ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if test ${edns:-0} != 0; then
    n=`expr $n + 1`
    echo_i "check that edns-version is honoured (${n})"
    ret=0
    $DIG $DIGOPTS @10.53.0.5 +edns no-edns-version.tld > dig.out.1.${n} || ret=1
    grep "query: no-edns-version.tld IN A -E(1)" ns6/named.run > /dev/null || ret=1
    $DIG $DIGOPTS @10.53.0.5 +edns edns-version.tld > dig.out.2.${n} || ret=1
    grep "query: edns-version.tld IN A -E(0)" ns7/named.run > /dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
fi

n=`expr $n + 1`
echo_i "check that CNAME nameserver is logged correctly (${n})"
ret=0
$DIG $DIGOPTS soa all-cnames @10.53.0.5 > dig.out.ns5.test${n} || ret=1
grep "status: SERVFAIL" dig.out.ns5.test${n} > /dev/null || ret=1
grep "skipping nameserver 'cname.tld' because it is a CNAME, while resolving 'all-cnames/SOA'" ns5/named.run > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that unexpected opcodes are handled correctly (${n})"
ret=0
$DIG $DIGOPTS soa all-cnames @10.53.0.5 +opcode=15 +cd +rec +ad +zflag > dig.out.ns5.test${n} || ret=1
grep "status: NOTIMP" dig.out.ns5.test${n} > /dev/null || ret=1
grep "flags:[^;]* qr[; ]" dig.out.ns5.test${n} > /dev/null || ret=1
grep "flags:[^;]* ra[; ]" dig.out.ns5.test${n} > /dev/null && ret=1
grep "flags:[^;]* rd[; ]" dig.out.ns5.test${n} > /dev/null && ret=1
grep "flags:[^;]* cd[; ]" dig.out.ns5.test${n} > /dev/null && ret=1
grep "flags:[^;]* ad[; ]" dig.out.ns5.test${n} > /dev/null && ret=1
grep "flags:[^;]*; MBZ: " dig.out.ns5.test${n} > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that EDNS client subnet with non-zeroed bits is handled correctly (${n})"
ret=0
# 0001 (IPv4) 1f (31 significant bits) 00 (0) ffffffff (255.255.255.255)
$DIG $DIGOPTS soa . @10.53.0.5 +ednsopt=8:00011f00ffffffff > dig.out.ns5.test${n} || ret=1
grep "status: FORMERR" dig.out.ns5.test${n} > /dev/null || ret=1
grep "; EDNS: version:" dig.out.ns5.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that dig +subnet zeros address bits correctly (${n})"
ret=0
$DIG $DIGOPTS soa . @10.53.0.5 +subnet=255.255.255.255/23 > dig.out.ns5.test${n} || ret=1
grep "status: NOERROR" dig.out.ns5.test${n} > /dev/null || ret=1
grep "CLIENT-SUBNET: 255.255.254.0/23/0" dig.out.ns5.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that SOA query returns data for delegation-only apex (${n})"
ret=0
$DIG $DIGOPTS soa delegation-only @10.53.0.5 > dig.out.ns5.test${n} || ret=1
grep "status: NOERROR" dig.out.ns5.test${n} > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns5.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

n=`expr $n + 1`
echo_i "check that NS query returns data for delegation-only apex (${n})"
ret=0
$DIG $DIGOPTS ns delegation-only @10.53.0.5 > dig.out.ns5.test${n} || ret=1
grep "status: NOERROR" dig.out.ns5.test${n} > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns5.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that A query returns data for delegation-only A apex (${n})"
ret=0
$DIG $DIGOPTS a delegation-only @10.53.0.5 > dig.out.ns5.test${n} || ret=1
grep "status: NOERROR" dig.out.ns5.test${n} > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns5.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that CDS query returns data for delegation-only apex (${n})"
ret=0
$DIG $DIGOPTS cds delegation-only @10.53.0.5 > dig.out.ns5.test${n} || ret=1
grep "status: NOERROR" dig.out.ns5.test${n} > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns5.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that AAAA query returns data for delegation-only AAAA apex (${n})"
ret=0
$DIG $DIGOPTS a delegation-only @10.53.0.5 > dig.out.ns5.test${n} || ret=1
grep "status: NOERROR" dig.out.ns5.test${n} > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns5.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "check that DNSKEY query returns data for delegation-only apex (${n})"
ret=0
$DIG $DIGOPTS dnskey delegation-only @10.53.0.5 > dig.out.ns5.test${n} || ret=1
grep "status: NOERROR" dig.out.ns5.test${n} > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns5.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that CDNSKEY query returns data for delegation-only apex (${n})"
ret=0
$DIG $DIGOPTS cdnskey delegation-only @10.53.0.5 > dig.out.ns5.test${n} || ret=1
grep "status: NOERROR" dig.out.ns5.test${n} > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns5.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that NXDOMAIN is returned for delegation-only non-apex A data (${n})"
ret=0
$DIG $DIGOPTS a a.delegation-only @10.53.0.5 > dig.out.ns5.test${n} || ret=1
grep "status: NXDOMAIN" dig.out.ns5.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that NXDOMAIN is returned for delegation-only non-apex CDS data (${n})"
ret=0
$DIG $DIGOPTS cds cds.delegation-only @10.53.0.5 > dig.out.ns5.test${n} || ret=1
grep "status: NXDOMAIN" dig.out.ns5.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that NXDOMAIN is returned for delegation-only non-apex AAAA data (${n})"
ret=0
$DIG $DIGOPTS aaaa aaaa.delegation-only @10.53.0.5 > dig.out.ns5.test${n} || ret=1
grep "status: NXDOMAIN" dig.out.ns5.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "check that NXDOMAIN is returned for delegation-only non-apex CDNSKEY data (${n})"
ret=0
$DIG $DIGOPTS cdnskey cdnskey.delegation-only @10.53.0.5 > dig.out.ns5.test${n} || ret=1
grep "status: NXDOMAIN" dig.out.ns5.test${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check zero ttl not returned for learnt non zero ttl records (${n})"
ret=0
# use prefetch disabled server
$DIG $DIGOPTS @10.53.0.7 non-zero.example.net txt > dig.out.1.${n} || ret=1
ttl1=`awk '/"A" "short" "ttl"/ { print $2 - 2 }' dig.out.1.${n}`
# sleep so we are in expire range
sleep ${ttl1:-0}
# look for ttl = 1, allow for one miss at getting zero ttl
zerotonine="0 1 2 3 4 5 6 7 8 9"
zerotonine="$zerotonine $zerotonine $zerotonine"
for i in $zerotonine $zerotonine $zerotonine $zerotonine
do
	$DIG $DIGOPTS @10.53.0.7 non-zero.example.net txt > dig.out.2.${n} || ret=1
	ttl2=`awk '/"A" "short" "ttl"/ { print $2 }' dig.out.2.${n}`
	test ${ttl2:-1} -eq 0 && break
	test ${ttl2:-1} -ge ${ttl1:-0} && break
	$PERL -e 'select(undef, undef, undef, 0.05);'
done
test ${ttl2:-1} -eq 0 && ret=1
test ${ttl2:-1} -ge ${ttl1:-0} || break
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check zero ttl is returned for learnt zero ttl records (${n})"
ret=0
$DIG $DIGOPTS @10.53.0.7 zero.example.net txt > dig.out.1.${n} || ret=1
ttl=`awk '/"A" "zero" "ttl"/ { print $2 }' dig.out.1.${n}`
test ${ttl:-1} -eq 0 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that 'ad' in not returned in truncated answer with empty answer and authority sections to request with +ad (${n})"
ret=0
$DIG $DIGOPTS @10.53.0.6 dnskey ds.example.net +bufsize=512 +ad +nodnssec +ignore +norec > dig.out.$n
grep "flags: qr aa tc; QUERY: 1, ANSWER: 0, AUTHORITY: 0" dig.out.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that 'ad' in not returned in truncated answer with empty answer and authority sections to request with +dnssec (${n})"
ret=0
$DIG $DIGOPTS @10.53.0.6 dnskey ds.example.net +bufsize=512 +noad +dnssec +ignore +norec > dig.out.$n
grep "flags: qr aa tc; QUERY: 1, ANSWER: 0, AUTHORITY: 0" dig.out.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that the resolver accepts a reply with empty question section with TC=1 and retries over TCP ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.5 truncated.no-questions. a > dig.ns5.out.${n} || ret=1
grep "status: NOERROR" dig.ns5.out.${n} > /dev/null || ret=1
grep "ANSWER: 1," dig.ns5.out.${n} > /dev/null || ret=1
grep "1.2.3.4" dig.ns5.out.${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that the resolver rejects a reply with empty question section with TC=0 ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.5 not-truncated.no-questions. a > dig.ns5.out.${n} || ret=1
grep "status: NOERROR" dig.ns5.out.${n} > /dev/null && ret=1
grep "ANSWER: 1," dig.ns5.out.${n} > /dev/null && ret=1
grep "1.2.3.4" dig.ns5.out.${n} > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking SERVFAIL is returned when all authoritative servers return FORMERR ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.5 ns.formerr-to-all. a > dig.ns5.out.${n} || ret=1
grep "status: SERVFAIL" dig.ns5.out.${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
