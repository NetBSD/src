#!/bin/sh
#
# Copyright (C) 2004, 2007, 2011-2013  Internet Systems Consortium, Inc. ("ISC")
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

# Id: tests.sh,v 1.13 2011/10/13 22:18:05 marka Exp  

# ns1 = stealth master
# ns2 = slave with update forwarding disabled; not currently used
# ns3 = slave with update forwarding enabled

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

status=0


echo "I:waiting for servers to be ready for testing"
for i in 1 2 3 4 5 6 7 8 9 10
do
	ret=0
	$DIG +tcp example. @10.53.0.1 soa -p 5300 > dig.out.ns1 || ret=1
	grep "status: NOERROR" dig.out.ns1 > /dev/null ||  ret=1
	$DIG +tcp example. @10.53.0.2 soa -p 5300 > dig.out.ns2 || ret=1
	grep "status: NOERROR" dig.out.ns2 > /dev/null ||  ret=1
	$DIG +tcp example. @10.53.0.3 soa -p 5300 > dig.out.ns3 || ret=1
	grep "status: NOERROR" dig.out.ns3 > /dev/null ||  ret=1
	test $ret = 0 && break
	sleep 1
done
if [ $ret != 0 ] ; then echo "I:failed"; status=`expr $status + $ret`; fi

echo "I:fetching master copy of zone before update"
ret=0
$DIG +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.\
	@10.53.0.1 axfr -p 5300 > dig.out.ns1 || ret=1
if [ $ret != 0 ] ; then echo "I:failed"; status=`expr $status + $ret`; fi

echo "I:fetching slave 1 copy of zone before update"
$DIG +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.\
	@10.53.0.2 axfr -p 5300 > dig.out.ns2 || ret=1
if [ $ret != 0 ] ; then echo "I:failed"; status=`expr $status + $ret`; fi

echo "I:fetching slave 2 copy of zone before update"
ret=0
$DIG +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.\
	@10.53.0.3 axfr -p 5300 > dig.out.ns3 || ret=1
if [ $ret != 0 ] ; then echo "I:failed"; status=`expr $status + $ret`; fi

echo "I:comparing pre-update copies to known good data"
ret=0
$PERL ../digcomp.pl knowngood.before dig.out.ns1 || ret=1
$PERL ../digcomp.pl knowngood.before dig.out.ns2 || ret=1
$PERL ../digcomp.pl knowngood.before dig.out.ns3 || ret=1
if [ $ret != 0 ] ; then echo "I:failed"; status=`expr $status + $ret`; fi

echo "I:updating zone (signed)"
ret=0
$NSUPDATE -y update.example:c3Ryb25nIGVub3VnaCBmb3IgYSBtYW4gYnV0IG1hZGUgZm9yIGEgd29tYW4K -- - <<EOF || ret=1
server 10.53.0.3 5300
update add updated.example. 600 A 10.10.10.1
update add updated.example. 600 TXT Foo
send
EOF
if [ $ret != 0 ] ; then echo "I:failed"; status=`expr $status + $ret`; fi

echo "I:sleeping 15 seconds for server to incorporate changes"
sleep 15

echo "I:fetching master copy of zone after update"
ret=0
$DIG +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.\
	@10.53.0.1 axfr -p 5300 > dig.out.ns1 || ret=1
if [ $ret != 0 ] ; then echo "I:failed"; status=`expr $status + $ret`; fi

echo "I:fetching slave 1 copy of zone after update"
ret=0
$DIG +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.\
	@10.53.0.2 axfr -p 5300 > dig.out.ns2 || ret=1
if [ $ret != 0 ] ; then echo "I:failed"; status=`expr $status + $ret`; fi

echo "I:fetching slave 2 copy of zone after update"
ret=0
$DIG +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.\
	@10.53.0.3 axfr -p 5300 > dig.out.ns3 || ret=1
if [ $ret != 0 ] ; then echo "I:failed"; status=`expr $status + $ret`; fi

echo "I:comparing post-update copies to known good data"
ret=0
$PERL ../digcomp.pl knowngood.after1 dig.out.ns1 || ret=1
$PERL ../digcomp.pl knowngood.after1 dig.out.ns2 || ret=1
$PERL ../digcomp.pl knowngood.after1 dig.out.ns3 || ret=1
if [ $ret != 0 ] ; then echo "I:failed"; status=`expr $status + $ret`; fi

echo "I:checking 'forwarding update for zone' is logged"
ret=0
grep "forwarding update for zone 'example/IN'" ns3/named.run > /dev/null || ret=1
if [ $ret != 0 ] ; then echo "I:failed"; status=`expr $status + $ret`; fi

echo "I:updating zone (unsigned)"
ret=0
$NSUPDATE -- - <<EOF || ret=1
server 10.53.0.3 5300
update add unsigned.example. 600 A 10.10.10.1
update add unsigned.example. 600 TXT Foo
send
EOF
if [ $ret != 0 ] ; then echo "I:failed"; status=`expr $status + $ret`; fi

echo "I:sleeping 15 seconds for server to incorporate changes"
sleep 15

echo "I:fetching master copy of zone after update"
ret=0
$DIG +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.\
	@10.53.0.1 axfr -p 5300 > dig.out.ns1 || ret=1
if [ $ret != 0 ] ; then echo "I:failed"; status=`expr $status + $ret`; fi

echo "I:fetching slave 1 copy of zone after update"
ret=0
$DIG +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.\
	@10.53.0.2 axfr -p 5300 > dig.out.ns2 || ret=1
if [ $ret != 0 ] ; then echo "I:failed"; status=`expr $status + $ret`; fi

echo "I:fetching slave 2 copy of zone after update"
ret=0
$DIG +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.\
	@10.53.0.3 axfr -p 5300 > dig.out.ns3 || ret=1
if [ $ret != 0 ] ; then echo "I:failed"; status=`expr $status + $ret`; fi

echo "I:comparing post-update copies to known good data"
ret=0
$PERL ../digcomp.pl knowngood.after2 dig.out.ns1 || ret=1
$PERL ../digcomp.pl knowngood.after2 dig.out.ns2 || ret=1
$PERL ../digcomp.pl knowngood.after2 dig.out.ns3 || ret=1
if [ $ret != 0 ] ; then echo "I:failed"; status=`expr $status + $ret`; fi

echo "I:checking update forwarding to dead master"
count=0
ret=0
while [ $count -lt 5 -a $ret -eq 0 ]
do
(
$NSUPDATE -- - <<EOF 
server 10.53.0.3 5300
zone nomaster
update add unsigned.nomaster. 600 A 10.10.10.1
update add unsigned.nomaster. 600 TXT Foo
send
EOF
) > /dev/null 2>&1 &
	$DIG +notcp +noadd +noauth nomaster.\
		@10.53.0.3 soa -p 5300 > dig.out.ns3 || ret=1
	grep "status: NOERROR" dig.out.ns3 > /dev/null || ret=1
	count=`expr $count + 1`
done
if [ $ret != 0 ] ; then echo "I:failed"; status=`expr $status + $ret`; fi

echo "I:exit status: $status"
exit $status
