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

DIGOPTS="+tcp +noadd +nosea +nostat +noquest +nocomm +nocmd -p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

status=0
n=0

#
# Wait up to 10 seconds for the servers to finish starting before testing.
#
for i in 1 2 3 4 5 6 7 8 9 10
do
	ret=0
	$DIG +tcp -p ${PORT} example @10.53.0.2 soa > dig.out.ns2.test$n || ret=1
	grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
	grep "flags:.* aa[ ;]" dig.out.ns2.test$n > /dev/null || ret=1
	$DIG +tcp -p ${PORT} example @10.53.0.3 soa > dig.out.ns3.test$n || ret=1
	grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
	grep "flags:.* aa[ ;]" dig.out.ns3.test$n > /dev/null || ret=1
        nr=`grep 'x[0-9].*sending notify to' ns2/named.run | wc -l`
        [ $nr -eq 20 ] || ret=1
	[ $ret = 0 ] && break
	sleep 1
done

n=`expr $n + 1`
echo_i "checking initial status ($n)"
ret=0
$DIG $DIGOPTS a.example. @10.53.0.2 a > dig.out.ns2.test$n || ret=1
grep "10.0.0.1" dig.out.ns2.test$n > /dev/null || ret=1

$DIG $DIGOPTS a.example. @10.53.0.3 a > dig.out.ns3.test$n || ret=1
grep "10.0.0.1" dig.out.ns3.test$n > /dev/null || ret=1

digcomp dig.out.ns2.test$n dig.out.ns3.test$n || ret=1

[ $ret = 0 ] || echo_i "failed"
status=`expr $ret + $status`

n=`expr $n + 1`
echo_i "checking startup notify rate limit ($n)"
ret=0
grep 'x[0-9].*sending notify to' ns2/named.run |
    sed 's/.*:\([0-9][0-9]\)\..*/\1/' | uniq -c | awk '{print $1}' > log.out
# the notifies should span at least 4 seconds
wc -l log.out | awk '$1 < 4 { exit(1) }' || ret=1
# ... with no more than 5 in any one second
awk '$1 > 5 { exit(1) }' log.out || ret=1
[ $ret = 0 ] || echo_i "failed"
status=`expr $ret + $status`

nextpart ns3/named.run > /dev/null

sleep 1 # make sure filesystem time stamp is newer for reload.
rm -f ns2/example.db
cp -f ns2/example2.db ns2/example.db
if [ ! "$CYGWIN" ]; then
    echo_i "reloading with example2 using HUP and waiting up to 45 seconds"
    $KILL -HUP `cat ns2/named.pid`
else
    echo_i "reloading with example2 using rndc and waiting up to 45 seconds"
    $RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/I:ns2 /'
fi

try=0
while test $try -lt 45
do
    nextpart ns3/named.run > tmp
    grep "transfer of 'example/IN' from 10.53.0.2#.*success" tmp > /dev/null && break
    sleep 1
    try=`expr $try + 1`
done

n=`expr $n + 1`
echo_i "checking notify message was logged ($n)"
ret=0
grep 'notify from 10.53.0.2#[0-9][0-9]*: serial 2$' ns3/named.run > /dev/null || ret=1
[ $ret = 0 ] || echo_i "failed"
status=`expr $ret + $status`

n=`expr $n + 1`
echo_i "checking example2 loaded ($n)"
ret=0
$DIG $DIGOPTS a.example. @10.53.0.2 a > dig.out.ns2.test$n || ret=1
grep "10.0.0.2" dig.out.ns2.test$n > /dev/null || ret=1

[ $ret = 0 ] || echo_i "failed"
status=`expr $ret + $status`

n=`expr $n + 1`
echo_i "checking example2 contents have been transferred after HUP reload ($n)"
ret=0
$DIG $DIGOPTS a.example. @10.53.0.2 a > dig.out.ns2.test$n || ret=1
grep "10.0.0.2" dig.out.ns2.test$n > /dev/null || ret=1

$DIG $DIGOPTS a.example. @10.53.0.3 a > dig.out.ns3.test$n || ret=1
grep "10.0.0.2" dig.out.ns3.test$n > /dev/null || ret=1

digcomp dig.out.ns2.test$n dig.out.ns3.test$n || ret=1

[ $ret = 0 ] || echo_i "failed"
status=`expr $ret + $status`

echo_i "stopping master and restarting with example4 then waiting up to 45 seconds"
$PERL $SYSTEMTESTTOP/stop.pl . ns2

rm -f ns2/example.db
cp -f ns2/example4.db ns2/example.db

$PERL $SYSTEMTESTTOP/start.pl --noclean --restart --port ${PORT} . ns2

try=0
while test $try -lt 45
do
    nextpart ns3/named.run > tmp
    grep "transfer of 'example/IN' from 10.53.0.2#.*success" tmp > /dev/null && break
    sleep 1
    try=`expr $try + 1`
done

n=`expr $n + 1`
echo_i "checking notify message was logged ($n)"
ret=0
grep 'notify from 10.53.0.2#[0-9][0-9]*: serial 4$' ns3/named.run > /dev/null || ret=1
[ $ret = 0 ] || echo_i "failed"
status=`expr $ret + $status`

n=`expr $n + 1`
echo_i "checking example4 loaded ($n)"
ret=0
$DIG $DIGOPTS a.example. @10.53.0.2 a > dig.out.ns2.test$n || ret=1
grep "10.0.0.4" dig.out.ns2.test$n > /dev/null || ret=1

[ $ret = 0 ] || echo_i "failed"
status=`expr $ret + $status`

n=`expr $n + 1`
echo_i "checking example4 contents have been transfered after restart ($n)"
ret=0
$DIG $DIGOPTS a.example. @10.53.0.2 a > dig.out.ns2.test$n || ret=1
grep "10.0.0.4" dig.out.ns2.test$n > /dev/null || ret=1

$DIG $DIGOPTS a.example. @10.53.0.3 a > dig.out.ns3.test$n || ret=1
grep "10.0.0.4" dig.out.ns3.test$n > /dev/null || ret=1

digcomp dig.out.ns2.test$n dig.out.ns3.test$n || ret=1

[ $ret = 0 ] || echo_i "failed"
status=`expr $ret + $status`

n=`expr $n + 1`
echo_i "checking notify to alternate port with master inheritance ($n)"
$NSUPDATE << EOF
server 10.53.0.2 ${PORT}
zone x21
update add added.x21 0 in txt "test string"
send
EOF
for i in 1 2 3 4 5 6 7 8 9
do
	$DIG $DIGOPTS added.x21. @10.53.0.4 txt -p $EXTRAPORT1 > dig.out.ns4.test$n || ret=1
	grep "test string" dig.out.ns4.test$n > /dev/null && break
	sleep 1
done
grep "test string" dig.out.ns4.test$n > /dev/null || ret=1

[ $ret = 0 ] || echo_i "failed"
status=`expr $ret + $status`

n=`expr $n + 1`
echo_i "checking notify to multiple views using tsig ($n)"
ret=0
$NSUPDATE << EOF
server 10.53.0.5 ${PORT}
zone x21
key a aaaaaaaaaaaaaaaaaaaa
update add added.x21 0 in txt "test string"
send
EOF

for i in 1 2 3 4 5 6 7 8 9
do
	$DIG $DIGOPTS added.x21. -y b:bbbbbbbbbbbbbbbbbbbb @10.53.0.5 \
		txt > dig.out.b.ns5.test$n || ret=1
	$DIG $DIGOPTS added.x21. -y c:cccccccccccccccccccc @10.53.0.5 \
		txt > dig.out.c.ns5.test$n || ret=1
	grep "test string" dig.out.b.ns5.test$n > /dev/null &&
	grep "test string" dig.out.c.ns5.test$n > /dev/null &&
        break
	sleep 1
done
grep "test string" dig.out.b.ns5.test$n > /dev/null || ret=1
grep "test string" dig.out.c.ns5.test$n > /dev/null || ret=1

[ $ret = 0 ] || echo_i "failed"
status=`expr $ret + $status`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
