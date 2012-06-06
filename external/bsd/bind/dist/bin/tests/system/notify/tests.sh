#!/bin/sh
#
# Copyright (C) 2004, 2007, 2011  Internet Systems Consortium, Inc. ("ISC")
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

# Id: tests.sh,v 1.36 2011/10/17 01:33:27 marka Exp 

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

status=0
n=0

#
# Wait up to 10 seconds for the servers to finish starting before testing.
#
for i in 1 2 3 4 5 6 7 8 9 10
do
	ret=0
	$DIG +tcp example @10.53.0.2 soa -p 5300 > dig.out.ns2.test$n || ret=1
	grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
	grep "flags:.* aa[ ;]" dig.out.ns2.test$n > /dev/null || ret=1
	$DIG +tcp example @10.53.0.3 soa -p 5300 > dig.out.ns3.test$n || ret=1
	grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
	grep "flags:.* aa[ ;]" dig.out.ns3.test$n > /dev/null || ret=1
	[ $ret = 0 ] && break
	sleep 1
done

n=`expr $n + 1`
echo "I:checking initial status ($n)"
ret=0
$DIG +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd a.example.\
	@10.53.0.2 a -p 5300 > dig.out.ns2.test$n || ret=1
grep "10.0.0.1" dig.out.ns2.test$n > /dev/null || ret=1

$DIG +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd a.example.\
	@10.53.0.3 a -p 5300 > dig.out.ns3.test$n || ret=1
grep "10.0.0.1" dig.out.ns3.test$n > /dev/null || ret=1

$PERL ../digcomp.pl dig.out.ns2.test$n dig.out.ns3.test$n || ret=1

[ $ret = 0 ] || echo "I:failed"
status=`expr $ret + $status`

echo "I:reloading with example2 using HUP and waiting 45 seconds"
sleep 1 # make sure filesystem time stamp is newer for reload.
rm -f ns2/example.db
cp -f ns2/example2.db ns2/example.db
kill -HUP `cat ns2/named.pid`
sleep 45

n=`expr $n + 1`
echo "I:checking example2 loaded ($n)"
ret=0
$DIG +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd a.example.\
	@10.53.0.2 a -p 5300 > dig.out.ns2.test$n || ret=1
grep "10.0.0.2" dig.out.ns2.test$n > /dev/null || ret=1

[ $ret = 0 ] || echo "I:failed"
status=`expr $ret + $status`

n=`expr $n + 1`
echo "I:checking example2 contents have been transferred after HUP reload ($n)"
ret=0
$DIG +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd a.example.\
	@10.53.0.2 a -p 5300 > dig.out.ns2.test$n || ret=1
grep "10.0.0.2" dig.out.ns2.test$n > /dev/null || ret=1

$DIG +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd a.example.\
	@10.53.0.3 a -p 5300 > dig.out.ns3.test$n || ret=1
grep "10.0.0.2" dig.out.ns3.test$n > /dev/null || ret=1

$PERL ../digcomp.pl dig.out.ns2.test$n dig.out.ns3.test$n || ret=1

[ $ret = 0 ] || echo "I:failed"
status=`expr $ret + $status`

echo "I:stopping master and restarting with example4 then waiting 45 seconds"
$PERL $SYSTEMTESTTOP/stop.pl . ns2

rm -f ns2/example.db
cp -f ns2/example4.db ns2/example.db

$PERL $SYSTEMTESTTOP/start.pl --noclean --restart . ns2

sleep 45

n=`expr $n + 1`
echo "I:checking example4 loaded ($n)"
ret=0
$DIG +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd a.example.\
	@10.53.0.2 a -p 5300 > dig.out.ns2.test$n || ret=1
grep "10.0.0.4" dig.out.ns2.test$n > /dev/null || ret=1

[ $ret = 0 ] || echo "I:failed"
status=`expr $ret + $status`

n=`expr $n + 1`
echo "I:checking example4 contents have been transfered after restart ($n)"
ret=0
$DIG +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd a.example.\
	@10.53.0.2 a -p 5300 > dig.out.ns2.test$n || ret=1
grep "10.0.0.4" dig.out.ns2.test$n > /dev/null || ret=1

$DIG +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd a.example.\
	@10.53.0.3 a -p 5300 > dig.out.ns3.test$n || ret=1
grep "10.0.0.4" dig.out.ns3.test$n > /dev/null || ret=1

$PERL ../digcomp.pl dig.out.ns2.test$n dig.out.ns3.test$n || ret=1

[ $ret = 0 ] || echo "I:failed"
status=`expr $ret + $status`

echo "I:exit status: $status"
exit $status
