#!/bin/sh
#
# Copyright (C) 2004, 2007, 2009, 2010  Internet Systems Consortium, Inc. ("ISC")
# Copyright (C) 2000, 2001  Internet Software Consortium.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

# Id: tests.sh,v 1.11.142.2.18.4 2010/11/16 07:04:08 marka Exp

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

status=0
n=0

echo "I:checking non-cachable NXDOMAIN response handling"
ret=0
$DIG +tcp nxdomain.example.net @10.53.0.1 a -p 5300 > dig.out || ret=1
grep "status: NXDOMAIN" dig.out > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking non-cachable NODATA response handling"
ret=0
$DIG +tcp nodata.example.net @10.53.0.1 a -p 5300 > dig.out || ret=1
grep "status: NOERROR" dig.out > /dev/null || ret=1

if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`
echo "I:checking handling of bogus referrals"
# If the server has the "INSIST(!external)" bug, this query will kill it.
$DIG +tcp www.example.com. a @10.53.0.1 -p 5300 >/dev/null || status=1

echo "I:check handling of cname + other data / 1"
$DIG +tcp cname1.example.com. a @10.53.0.1 -p 5300 >/dev/null || status=1

echo "I:check handling of cname + other data / 2"
$DIG +tcp cname2.example.com. a @10.53.0.1 -p 5300 >/dev/null || status=1

echo "I:check that server is still running"
$DIG +tcp www.example.com. a @10.53.0.1 -p 5300 >/dev/null || status=1

echo "I:checking answer IPv4 address filtering (deny)"
ret=0
$DIG +tcp www.example.net @10.53.0.1 a -p 5300 > dig.out || ret=1
grep "status: SERVFAIL" dig.out > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking answer IPv6 address filtering (deny)"
ret=0
$DIG +tcp www.example.net @10.53.0.1 aaaa -p 5300 > dig.out || ret=1
grep "status: SERVFAIL" dig.out > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking answer IPv4 address filtering (accept)"
ret=0
$DIG +tcp www.example.org @10.53.0.1 a -p 5300 > dig.out || ret=1
grep "status: NOERROR" dig.out > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking answer IPv6 address filtering (accept)"
ret=0
$DIG +tcp www.example.org @10.53.0.1 aaaa -p 5300 > dig.out || ret=1
grep "status: NOERROR" dig.out > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking CNAME target filtering (deny)"
ret=0
$DIG +tcp badcname.example.net @10.53.0.1 a -p 5300 > dig.out || ret=1
grep "status: SERVFAIL" dig.out > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking CNAME target filtering (accept)"
ret=0
$DIG +tcp goodcname.example.net @10.53.0.1 a -p 5300 > dig.out || ret=1
grep "status: NOERROR" dig.out > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking CNAME target filtering (accept due to subdomain)"
ret=0
$DIG +tcp cname.sub.example.org @10.53.0.1 a -p 5300 > dig.out || ret=1
grep "status: NOERROR" dig.out > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking DNAME target filtering (deny)"
ret=0
$DIG +tcp foo.baddname.example.net @10.53.0.1 a -p 5300 > dig.out || ret=1
grep "status: SERVFAIL" dig.out > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking DNAME target filtering (accept)"
ret=0
$DIG +tcp foo.gooddname.example.net @10.53.0.1 a -p 5300 > dig.out || ret=1
grep "status: NOERROR" dig.out > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:checking DNAME target filtering (accept due to subdomain)"
ret=0
$DIG +tcp www.dname.sub.example.org @10.53.0.1 a -p 5300 > dig.out || ret=1
grep "status: NOERROR" dig.out > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: RT21594 regression test check setup ($n)"
ret=0
# Check that "aa" is not being set by the authoritative server.
$DIG +tcp . @10.53.0.4 soa -p 5300 > dig.ns4.out.${n} || ret=1
grep 'flags: qr rd;' dig.ns4.out.${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: RT21594 regression test positive answers ($n)"
ret=0
# Check that resolver accepts the non-authoritative positive answers.
$DIG +tcp . @10.53.0.5 soa -p 5300 > dig.ns5.out.${n} || ret=1
grep "status: NOERROR" dig.ns5.out.${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: RT21594 regression test NODATA answers ($n)"
ret=0
# Check that resolver accepts the non-authoritative nodata answers.
$DIG +tcp . @10.53.0.5 txt -p 5300 > dig.ns5.out.${n} || ret=1
grep "status: NOERROR" dig.ns5.out.${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: RT21594 regression test NXDOMAIN answers ($n)"
ret=0
# Check that resolver accepts the non-authoritative positive answers.
$DIG +tcp noexistant @10.53.0.5 txt -p 5300 > dig.ns5.out.${n} || ret=1
grep "status: NXDOMAIN" dig.ns5.out.${n} > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I:check that replacement of additional data by a negative cache no data entry clears the additional RRSIGs ($n)"
ret=0
$DIG +tcp mx example.net @10.53.0.7 -p 5300 > dig.ns7.out.${n} || ret=1
grep "status: NOERROR" dig.ns7.out.${n} > /dev/null || ret=1
if [ $ret = 1 ]; then echo "I:mx priming failed"; fi
$NSUPDATE << EOF
server 10.53.0.6 5300
zone example.net
update delete mail.example.net A
update add mail.example.net 0 AAAA ::1
send
EOF
$DIG +tcp a mail.example.net @10.53.0.7 -p 5300 > dig.ns7.out.${n} || ret=2
grep "status: NOERROR" dig.ns7.out.${n} > /dev/null || ret=2
grep "ANSWER: 0" dig.ns7.out.${n} > /dev/null || ret=2
if [ $ret = 2 ]; then echo "I:ncache priming failed"; fi
$DIG +tcp mx example.net @10.53.0.7 -p 5300 > dig.ns7.out.${n} || ret=3
grep "status: NOERROR" dig.ns7.out.${n} > /dev/null || ret=3
$DIG +tcp rrsig mail.example.net +norec @10.53.0.7 -p 5300 > dig.ns7.out.${n}  || ret=4
grep "status: NOERROR" dig.ns7.out.${n} > /dev/null || ret=4
grep "ANSWER: 0" dig.ns7.out.${n} > /dev/null || ret=4
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

echo "I:exit status: $status"
exit $status
