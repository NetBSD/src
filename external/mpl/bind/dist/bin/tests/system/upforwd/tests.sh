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

# ns1 = stealth primary
# ns2 = secondary with update forwarding disabled; not currently used
# ns3 = secondary with update forwarding enabled

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

DIGOPTS="+tcp +noadd +nosea +nostat +noquest +nocomm +nocmd -p ${PORT}"
RNDCCMD="$RNDC -p ${CONTROLPORT} -c ../common/rndc.conf"

status=0
n=1
capture_dnstap() {
	retry_quiet 20 test -f ns3/dnstap.out && mv ns3/dnstap.out dnstap.out.$n
	$RNDCCMD -s 10.53.0.3 dnstap -reopen
}

uq_equals_ur() {
	"$DNSTAPREAD" dnstap.out.$n |
        awk '$3 == "UQ" { UQ+=1 } $3 == "UR" { UR += 1 } END { print UQ+0, UR+0 }' > dnstapread.out$n
        read UQ UR < dnstapread.out$n
	echo_i "UQ=$UQ UR=$UR"
        test $UQ -eq $UR || return 1
}

echo_i "waiting for servers to be ready for testing ($n)"
for i in 1 2 3 4 5 6 7 8 9 10
do
	ret=0
	$DIG +tcp -p ${PORT} example. @10.53.0.1 soa > dig.out.ns1 || ret=1
	grep "status: NOERROR" dig.out.ns1 > /dev/null ||  ret=1
	$DIG +tcp -p ${PORT} example. @10.53.0.2 soa > dig.out.ns2 || ret=1
	grep "status: NOERROR" dig.out.ns2 > /dev/null ||  ret=1
	$DIG +tcp -p ${PORT} example. @10.53.0.3 soa > dig.out.ns3 || ret=1
	grep "status: NOERROR" dig.out.ns3 > /dev/null ||  ret=1
	test $ret = 0 && break
	sleep 1
done
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi
n=`expr $n + 1`

echo_i "fetching primary copy of zone before update ($n)"
ret=0
$DIG $DIGOPTS example.\
	@10.53.0.1 axfr > dig.out.ns1 || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi
n=`expr $n + 1`

echo_i "fetching secondary 1 copy of zone before update ($n)"
$DIG $DIGOPTS example.\
	@10.53.0.2 axfr > dig.out.ns2 || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi
n=`expr $n + 1`

echo_i "fetching secondary 2 copy of zone before update ($n)"
ret=0
$DIG $DIGOPTS example.\
	@10.53.0.3 axfr > dig.out.ns3 || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi
n=`expr $n + 1`

echo_i "comparing pre-update copies to known good data ($n)"
ret=0
digcomp knowngood.before dig.out.ns1 || ret=1
digcomp knowngood.before dig.out.ns2 || ret=1
digcomp knowngood.before dig.out.ns3 || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi

echo_i "updating zone (signed) ($n)"
ret=0
$NSUPDATE -y update.example:c3Ryb25nIGVub3VnaCBmb3IgYSBtYW4gYnV0IG1hZGUgZm9yIGEgd29tYW4K -- - <<EOF || ret=1
local 10.53.0.1
server 10.53.0.3 ${PORT}
update add updated.example. 600 A 10.10.10.1
update add updated.example. 600 TXT Foo
send
EOF
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi
n=`expr $n + 1`

echo_i "sleeping 15 seconds for server to incorporate changes"
sleep 15

echo_i "fetching primary copy of zone after update ($n)"
ret=0
$DIG $DIGOPTS example.\
	@10.53.0.1 axfr > dig.out.ns1 || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi
n=`expr $n + 1`

echo_i "fetching secondary 1 copy of zone after update ($n)"
ret=0
$DIG $DIGOPTS example.\
	@10.53.0.2 axfr > dig.out.ns2 || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi

echo_i "fetching secondary 2 copy of zone after update ($n)"
ret=0
$DIG $DIGOPTS example.\
	@10.53.0.3 axfr > dig.out.ns3 || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi
n=`expr $n + 1`

echo_i "comparing post-update copies to known good data ($n)"
ret=0
digcomp knowngood.after1 dig.out.ns1 || ret=1
digcomp knowngood.after1 dig.out.ns2 || ret=1
digcomp knowngood.after1 dig.out.ns3 || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi

echo_i "checking 'forwarding update for zone' is logged ($n)"
ret=0
grep "forwarding update for zone 'example/IN'" ns3/named.run > /dev/null || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi
n=`expr $n + 1`

if $FEATURETEST --enable-dnstap
then
	echo_i "checking DNSTAP logging of UPDATE forwarded update replies ($n)"
	ret=0
	capture_dnstap
	uq_equals_ur || ret=1
	if [ $ret != 0 ] ; then echo_i "failed"; fi
	status=`expr $status + $ret`
	n=`expr $n + 1`
fi

echo_i "updating zone (unsigned) ($n)"
ret=0
$NSUPDATE -- - <<EOF || ret=1
local 10.53.0.1
server 10.53.0.3 ${PORT}
update add unsigned.example. 600 A 10.10.10.1
update add unsigned.example. 600 TXT Foo
send
EOF
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi
n=`expr $n + 1`

echo_i "sleeping 15 seconds for server to incorporate changes"
sleep 15

echo_i "fetching primary copy of zone after update ($n)"
ret=0
$DIG $DIGOPTS example.\
	@10.53.0.1 axfr > dig.out.ns1 || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi

echo_i "fetching secondary 1 copy of zone after update ($n)"
ret=0
$DIG $DIGOPTS example.\
	@10.53.0.2 axfr > dig.out.ns2 || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi
n=`expr $n + 1`

echo_i "fetching secondary 2 copy of zone after update ($n)"
ret=0
$DIG $DIGOPTS example.\
	@10.53.0.3 axfr > dig.out.ns3 || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi

echo_i "comparing post-update copies to known good data ($n)"
ret=0
digcomp knowngood.after2 dig.out.ns1 || ret=1
digcomp knowngood.after2 dig.out.ns2 || ret=1
digcomp knowngood.after2 dig.out.ns3 || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi

if $FEATURETEST --enable-dnstap
then
	echo_i "checking DNSTAP logging of UPDATE forwarded update replies ($n)"
	ret=0
	capture_dnstap
	uq_equals_ur || ret=1
	if [ $ret != 0 ] ; then echo_i "failed"; fi
	status=`expr $status + $ret`
	n=`expr $n + 1`
fi
n=`expr $n + 1`

echo_i "checking update forwarding to dead primary ($n)"
count=0
ret=0
while [ $count -lt 5 -a $ret -eq 0 ]
do
(
$NSUPDATE -- - <<EOF 
local 10.53.0.1
server 10.53.0.3 ${PORT}
zone nomaster
update add unsigned.nomaster. 600 A 10.10.10.1
update add unsigned.nomaster. 600 TXT Foo
send
EOF
) > /dev/null 2>&1 &
	$DIG -p ${PORT} +noadd +notcp +noauth nomaster. @10.53.0.3 soa > dig.out.ns3 || ret=1
	grep "status: NOERROR" dig.out.ns3 > /dev/null || ret=1
	count=`expr $count + 1`
done
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi
n=`expr $n + 1`

if $FEATURETEST --enable-dnstap
then
	echo_i "checking DNSTAP logging of UPDATE forwarded update replies ($n)"
	ret=0
	capture_dnstap
	uq_equals_ur && ret=1
	if [ $ret != 0 ] ; then echo_i "failed"; fi
	status=`expr $status + $ret`
	n=`expr $n + 1`
fi

if test -f keyname
then
	echo_i "checking update forwarding to with sig0 ($n)"
	ret=0
	keyname=`cat keyname`
	$NSUPDATE -k $keyname.private -- - <<EOF
	local 10.53.0.1
	server 10.53.0.3 ${PORT}
	zone example2
	update add unsigned.example2. 600 A 10.10.10.1
	update add unsigned.example2. 600 TXT Foo
	send
EOF
	$DIG -p ${PORT} unsigned.example2 A @10.53.0.1 > dig.out.ns1.test$n
	grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
	if [ $ret != 0 ] ; then echo_i "failed"; fi
	status=`expr $status + $ret`
	n=`expr $n + 1`

	if $FEATURETEST --enable-dnstap
	then
		echo_i "checking DNSTAP logging of UPDATE forwarded update replies ($n)"
		ret=0
		capture_dnstap
		uq_equals_ur || ret=1
		if [ $ret != 0 ] ; then echo_i "failed"; fi
		status=`expr $status + $ret`
		n=`expr $n + 1`
	fi
fi

echo_i "attempting an update that should be rejected by ACL ($n)"
ret=0
{
        $NSUPDATE -- - << EOF
        local 10.53.0.2
        server 10.53.0.3 ${PORT}
        update add another.unsigned.example. 600 A 10.10.10.2
        update add another.unsigned.example. 600 TXT Bar
        send
EOF
} > nsupdate.out.$n 2>&1
grep REFUSED nsupdate.out.$n > /dev/null || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $status + $ret`; fi
n=`expr $n + 1`

n=$((n + 1))
ret=0
echo_i "attempting updates that should exceed quota ($n)"
# lower the update quota to 1.
copy_setports ns3/named2.conf.in ns3/named.conf
rndc_reconfig ns3 10.53.0.3
nextpart ns3/named.run > /dev/null
for loop in 1 2 3 4 5 6 7 8 9 10; do
{
  $NSUPDATE -- - > /dev/null 2>&1 <<END
  local 10.53.0.1
  server 10.53.0.3 ${PORT}
  update add txt-$loop.unsigned.example 300 IN TXT Whatever
  send
END
} &
done
wait_for_log 10 "too many DNS UPDATEs queued" ns3/named.run || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
