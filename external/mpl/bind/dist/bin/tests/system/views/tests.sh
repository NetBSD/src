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

DIGOPTS="+tcp +noadd +nosea +nostat +noquest +nocomm +nocmd +noauth -p ${PORT}"
SHORTOPTS="+tcp +short -p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

status=0

echo_i "fetching a.example from ns2's initial configuration"
$DIG $DIGOPTS a.example. @10.53.0.2 any > dig.out.ns2.1 || status=1

echo_i "fetching a.example from ns3's initial configuration"
$DIG $DIGOPTS a.example. @10.53.0.3 any > dig.out.ns3.1 || status=1

echo_i "copying in new configurations for ns2 and ns3"
rm -f ns2/named.conf ns3/named.conf ns2/example.db
cp -f ns2/example2.db ns2/example.db
copy_setports ns2/named2.conf.in ns2/named.conf
copy_setports ns3/named2.conf.in ns3/named.conf

echo_i "reloading ns2 and ns3 with rndc"
nextpart ns2/named.run > /dev/null
nextpart ns3/named.run > /dev/null
rndc_reload ns2 10.53.0.2
rndc_reload ns3 10.53.0.3

echo_i "wait for reload"
a=0 b=0
for i in 1 2 3 4 5 6 7 8 9 0; do
        nextpart ns2/named.run | grep "all zones loaded" > /dev/null && a=1
        nextpart ns3/named.run | grep "all zones loaded" > /dev/null && b=1
        [ $a -eq 1 -a $b -eq 1 ] && break
        sleep 1
done

echo_i "fetching a.example from ns2's 10.53.0.4, source address 10.53.0.4"
$DIG $DIGOPTS -b 10.53.0.4 a.example. @10.53.0.4 any > dig.out.ns4.2 || status=1

echo_i "fetching a.example from ns2's 10.53.0.2, source address 10.53.0.2"
$DIG $DIGOPTS -b 10.53.0.2 a.example. @10.53.0.2 any > dig.out.ns2.2 || status=1

echo_i "fetching a.example from ns3's 10.53.0.3, source address defaulted"
$DIG $DIGOPTS @10.53.0.3 a.example. any > dig.out.ns3.2 || status=1

echo_i "comparing ns3's initial a.example to one from reconfigured 10.53.0.2"
digcomp dig.out.ns3.1 dig.out.ns2.2 || status=1

echo_i "comparing ns3's initial a.example to one from reconfigured 10.53.0.3"
digcomp dig.out.ns3.1 dig.out.ns3.2 || status=1

echo_i "comparing ns2's initial a.example to one from reconfigured 10.53.0.4"
digcomp dig.out.ns2.1 dig.out.ns4.2 || status=1

echo_i "comparing ns2's initial a.example to one from reconfigured 10.53.0.3"
echo_i "(should be different)"
if $PERL ../digcomp.pl dig.out.ns2.1 dig.out.ns3.2 >/dev/null
then
	echo_i "no differences found.  something's wrong."
	status=1
fi

echo_i "updating cloned zone in internal view"
$NSUPDATE << EOF
server 10.53.0.2 ${PORT}
zone clone
update add b.clone. 300 in a 10.1.0.3
send
EOF
echo_i "sleeping to allow update to take effect"
sleep 5

echo_i "verifying update affected both views"
ret=0
one=`$DIG $SHORTOPTS -b 10.53.0.2 @10.53.0.2 b.clone a`
two=`$DIG $SHORTOPTS -b 10.53.0.4 @10.53.0.2 b.clone a`
if [ "$one" != "$two" ]; then
        echo "'$one' does not match '$two'"
        ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "verifying forwarder in cloned zone works"
ret=0
one=`$DIG $SHORTOPTS -b 10.53.0.2 @10.53.0.2 child.clone txt`
two=`$DIG $SHORTOPTS -b 10.53.0.4 @10.53.0.2 child.clone txt`
three=`$DIG $SHORTOPTS @10.53.0.3 child.clone txt`
four=`$DIG $SHORTOPTS @10.53.0.5 child.clone txt`
echo "$three" | grep NS3 > /dev/null || { ret=1; echo "expected response from NS3 got '$three'"; }
echo "$four" | grep NS5 > /dev/null || { ret=1; echo "expected response from NS5 got '$four'"; }
if [ "$one" = "$two" ]; then
        echo "'$one' matches '$two'"
        ret=1
fi
if [ "$one" != "$three" ]; then
        echo "'$one' does not match '$three'"
        ret=1
fi
if [ "$two" != "$four" ]; then
        echo "'$two' does not match '$four'"
        ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "verifying inline zones work with views"
ret=0
$DIG -p ${PORT} @10.53.0.2 -b 10.53.0.2 +dnssec DNSKEY inline > dig.out.internal
$DIG -p ${PORT} @10.53.0.2 -b 10.53.0.5 +dnssec DNSKEY inline > dig.out.external
grep "ANSWER: 4," dig.out.internal > /dev/null || ret=1
grep "ANSWER: 4," dig.out.external > /dev/null || ret=1
int=`awk '$4 == "DNSKEY" { print $8 }' dig.out.internal | sort`
ext=`awk '$4 == "DNSKEY" { print $8 }' dig.out.external | sort`
test "$int" != "$ext" || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
