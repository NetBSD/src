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

DIGOPTS="+tcp +dnssec -p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

status=0
n=0

$RNDCCMD 10.53.0.3 signing -nsec3param 1 0 0 - nsec3 > /dev/null 2>&1

for i in 1 2 3 4 5 6 7 8 9 0
do
	nsec3param=`$DIG $DIGOPTS +nodnssec +short @10.53.0.3 nsec3param nsec3.`
	test "$nsec3param" = "1 0 0 -" && break
	sleep 1
done

n=`expr $n + 1`
echo_i "checking that an unsupported algorithm is not used for signing ($n)"
ret=0
grep -q "algorithm is unsupported" ns3/named.run || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that rrsigs are replaced with ksk only ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.3 axfr nsec3. |
	awk '/RRSIG NSEC3/ {a[$1]++} END { for (i in a) {if (a[i] != 1) exit (1)}}' || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that the zone is signed on initial transfer ($n)"
ret=0
for i in 1 2 3 4 5 6 7 8 9 10 1 2 3 4 5 6 7 8 9 10
do
	ret=0
	$RNDCCMD 10.53.0.3 signing -list bits > signing.out.test$n 2>&1
	keys=`grep '^Done signing' signing.out.test$n | wc -l`
	[ $keys = 2 ] || ret=1
	if [ $ret = 0 ]; then break; fi
	sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking expired signatures are updated on load ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.3 +noall +answer +dnssec expired SOA > dig.out.ns3.test$n
expiry=`awk '$4 == "RRSIG" { print $9 }' dig.out.ns3.test$n`
[ "$expiry" = "20110101000000" ] && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking removal of private type record via 'rndc signing -clear' ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -list bits > signing.out.test$n 2>&1
keys=`sed -n -e 's/Done signing with key \(.*\)$/\1/p' signing.out.test$n`
for key in $keys; do
	$RNDCCMD 10.53.0.3 signing -clear ${key} bits > /dev/null || ret=1
	break;	# We only want to remove 1 record for now.
done 2>&1 |sed 's/^/ns3 /' | cat_i

for i in 1 2 3 4 5 6 7 8 9 10
do
	ans=0
	$RNDCCMD 10.53.0.3 signing -list bits > signing.out.test$n 2>&1
        num=`grep "Done signing with" signing.out.test$n | wc -l`
	[ $num = 1 ] && break
	sleep 1
done
[ $ans = 0 ] || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking private type was properly signed ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.6 bits TYPE65534 > dig.out.ns6.test$n
grep "ANSWER: 2," dig.out.ns6.test$n > /dev/null || ret=1
grep "flags:.* ad[ ;]" dig.out.ns6.test$n > /dev/null || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking removal of remaining private type record via 'rndc signing -clear all' ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -clear all bits > /dev/null || ret=1

for i in 1 2 3 4 5 6 7 8 9 10
do
	ans=0
	$RNDCCMD 10.53.0.3 signing -list bits > signing.out.test$n 2>&1
	grep "No signing records found" signing.out.test$n > /dev/null || ans=1
	[ $ans = 1 ] || break
	sleep 1
done
[ $ans = 0 ] || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking negative private type response was properly signed ($n)"
ret=0
sleep 1
$DIG $DIGOPTS @10.53.0.6 bits TYPE65534 > dig.out.ns6.test$n
grep "status: NOERROR" dig.out.ns6.test$n > /dev/null || ret=1
grep "ANSWER: 0," dig.out.ns6.test$n > /dev/null || ret=1
grep "flags:.* ad[ ;]" dig.out.ns6.test$n > /dev/null || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

$NSUPDATE << EOF
zone bits
server 10.53.0.2 ${PORT}
update add added.bits 0 A 1.2.3.4
send
EOF

n=`expr $n + 1`
echo_i "checking that the record is added on the hidden master ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.2 added.bits A > dig.out.ns2.test$n
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that update has been transfered and has been signed ($n)"
ret=0
for i in 1 2 3 4 5 6 7 8 9 10
do
	ret=0
	$DIG $DIGOPTS @10.53.0.3 added.bits A > dig.out.ns3.test$n
	grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
	grep "ANSWER: 2," dig.out.ns3.test$n > /dev/null || ret=1
	if [ $ret = 0 ]; then break; fi
	sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

$NSUPDATE << EOF
zone bits
server 10.53.0.2 ${PORT}
update add bits 0 SOA ns2.bits. . 2011072400 20 20 1814400 3600
send
EOF

n=`expr $n + 1`
echo_i "checking YYYYMMDDVV (2011072400) serial on hidden master ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.2 bits SOA > dig.out.ns2.test$n
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n > /dev/null || ret=1
grep "2011072400" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking YYYYMMDDVV (2011072400) serial in signed zone ($n)"
for i in 1 2 3 4 5 6 7 8 9 10
do
	ret=0
	$DIG $DIGOPTS @10.53.0.3 bits SOA > dig.out.ns3.test$n
	grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
	grep "ANSWER: 2," dig.out.ns3.test$n > /dev/null || ret=1
	grep "2011072400" dig.out.ns3.test$n > /dev/null || ret=1
	if [ $ret = 0 ]; then break; fi
	sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that the zone is signed on initial transfer, noixfr ($n)"
ret=0
for i in 1 2 3 4 5 6 7 8 9 10 1 2 3 4 5 6 7 8 9 10 1 2 3 4 5 6 7 8 9 10
do
	ret=0
	$RNDCCMD 10.53.0.3 signing -list noixfr > signing.out.test$n 2>&1
	keys=`grep '^Done signing' signing.out.test$n | wc -l`
	[ $keys = 2 ] || ret=1
	if [ $ret = 0 ]; then break; fi
	sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

$NSUPDATE << EOF
zone noixfr
server 10.53.0.4 ${PORT}
update add added.noixfr 0 A 1.2.3.4
send
EOF

n=`expr $n + 1`
echo_i "checking that the record is added on the hidden master, noixfr ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.4 added.noixfr A > dig.out.ns4.test$n
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that update has been transfered and has been signed, noixfr ($n)"
ret=0
for i in 1 2 3 4 5 6 7 8 9 10 1 2 3 4 5 6 7 8 9 10 1 2 3 4 5 6 7 8 9 10
do
	ret=0
	$DIG $DIGOPTS @10.53.0.3 added.noixfr A > dig.out.ns3.test$n
	grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
	grep "ANSWER: 2," dig.out.ns3.test$n > /dev/null || ret=1
	if [ $ret = 0 ]; then break; fi
	sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

$NSUPDATE << EOF
zone noixfr
server 10.53.0.4 ${PORT}
update add noixfr 0 SOA ns4.noixfr. . 2011072400 20 20 1814400 3600
send
EOF

n=`expr $n + 1`
echo_i "checking YYYYMMDDVV (2011072400) serial on hidden master, noixfr ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.4 noixfr SOA > dig.out.ns4.test$n
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns4.test$n > /dev/null || ret=1
grep "2011072400" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking YYYYMMDDVV (2011072400) serial in signed zone, noixfr ($n)"
for i in 1 2 3 4 5 6 7 8 9 10
do
	ret=0
	$DIG $DIGOPTS @10.53.0.3 noixfr SOA > dig.out.ns3.test$n
	grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
	grep "ANSWER: 2," dig.out.ns3.test$n > /dev/null || ret=1
	grep "2011072400" dig.out.ns3.test$n > /dev/null || ret=1
	if [ $ret = 0 ]; then break; fi
	sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that the master zone signed on initial load ($n)"
ret=0
for i in 1 2 3 4 5 6 7 8 9 10
do
	ret=0
	$RNDCCMD 10.53.0.3 signing -list master  > signing.out.test$n 2>&1
	keys=`grep '^Done signing' signing.out.test$n | wc -l`
	[ $keys = 2 ] || ret=1
	if [ $ret = 0 ]; then break; fi
	sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking removal of private type record via 'rndc signing -clear' (master) ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -list master > signing.out.test$n 2>&1
keys=`sed -n -e 's/Done signing with key \(.*\)$/\1/p' signing.out.test$n`
for key in $keys; do
	$RNDCCMD 10.53.0.3 signing -clear ${key} master > /dev/null || ret=1
	break;	# We only want to remove 1 record for now.
done 2>&1 |sed 's/^/ns3 /' | cat_i

for i in 1 2 3 4 5 6 7 8 9
do
	ans=0
	$RNDCCMD 10.53.0.3 signing -list master > signing.out.test$n 2>&1
        num=`grep "Done signing with" signing.out.test$n | wc -l`
	[ $num = 1 ] && break
	sleep 1
done
[ $ans = 0 ] || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking private type was properly signed (master) ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.6 master TYPE65534 > dig.out.ns6.test$n
grep "ANSWER: 2," dig.out.ns6.test$n > /dev/null || ret=1
grep "flags:.* ad[ ;]" dig.out.ns6.test$n > /dev/null || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking removal of remaining private type record via 'rndc signing -clear' (master) ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -clear all master > /dev/null || ret=1
for i in 1 2 3 4 5 6 7 8 9 10
do
	ans=0
	$RNDCCMD 10.53.0.3 signing -list master > signing.out.test$n 2>&1
	grep "No signing records found" signing.out.test$n > /dev/null || ans=1
	[ $ans = 1 ] || break
	sleep 1
done
[ $ans = 0 ] || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check adding of record to unsigned master ($n)"
ret=0
cp ns3/master2.db.in ns3/master.db
rndc_reload ns3 10.53.0.3 master
for i in 1 2 3 4 5 6 7 8 9
do
	ans=0
	$DIG $DIGOPTS @10.53.0.3 e.master A > dig.out.ns3.test$n
	grep "10.0.0.5" dig.out.ns3.test$n > /dev/null || ans=1
	grep "ANSWER: 2," dig.out.ns3.test$n > /dev/null || ans=1
	[ $ans = 1 ] || break
	sleep 1
done
[ $ans = 0 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check adding record fails when SOA serial not changed ($n)"
ret=0
echo "c A 10.0.0.3" >> ns3/master.db
rndc_reload ns3 10.53.0.3
sleep 1
$DIG $DIGOPTS @10.53.0.3 c.master A > dig.out.ns3.test$n
grep "NXDOMAIN" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check adding record works after updating SOA serial ($n)"
ret=0
cp ns3/master3.db.in ns3/master.db
$RNDCCMD 10.53.0.3 reload master 2>&1 | sed 's/^/ns3 /' | cat_i
for i in 1 2 3 4 5 6 7 8 9
do
	ans=0
	$DIG $DIGOPTS @10.53.0.3 c.master A > dig.out.ns3.test$n
	grep "10.0.0.3" dig.out.ns3.test$n > /dev/null || ans=1
	grep "ANSWER: 2," dig.out.ns3.test$n > /dev/null || ans=1
	[ $ans = 1 ] || break
	sleep 1
done
[ $ans = 0 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check the added record was properly signed ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.3 e.master A > dig.out.ns6.test$n
grep "10.0.0.5" dig.out.ns6.test$n > /dev/null || ans=1
grep "ANSWER: 2," dig.out.ns6.test$n > /dev/null || ans=1
grep "flags:.* ad[ ;]" dig.out.ns6.test$n > /dev/null || ans=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that the dynamic master zone signed on initial load ($n)"
ret=0
for i in 1 2 3 4 5 6 7 8 9 10
do
	ret=0
	$RNDCCMD 10.53.0.3 signing -list dynamic > signing.out.test$n 2>&1
	keys=`grep '^Done signing' signing.out.test$n | wc -l`
	[ $keys = 2 ] || ret=1
	if [ $ret = 0 ]; then break; fi
	sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking master zone that was updated while offline is correct ($n)"
ret=0
serial=`$DIG $DIGOPTS +nodnssec +short @10.53.0.3 updated SOA | awk '{print $3}'`
# serial should have changed
[ "$serial" = "2000042407" ] && ret=1
# e.updated should exist and should be signed
$DIG $DIGOPTS @10.53.0.3 e.updated A > dig.out.ns3.test$n
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
grep "ANSWER: 2," dig.out.ns3.test$n > /dev/null || ret=1
# updated.db.signed.jnl should exist, should have the source serial
# of master2.db, and should show a minimal diff: no more than 8 added
# records (SOA/RRSIG, 2 x NSEC/RRSIG, A/RRSIG), and 4 removed records
# (SOA/RRSIG, NSEC/RRSIG).
serial=`$JOURNALPRINT ns3/updated.db.signed.jnl | head -1 | awk '{print $4}'`
[ "$serial" = "2000042408" ] || ret=1
diffsize=`$JOURNALPRINT ns3/updated.db.signed.jnl | wc -l`
[ "$diffsize" -le 13 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking adding of record to unsigned master using UPDATE ($n)"
ret=0

[ -f ns3/dynamic.db.jnl ] && { ret=1 ; echo_i "journal exists (pretest)" ; }

$NSUPDATE << EOF
zone dynamic
server 10.53.0.3 ${PORT}
update add e.dynamic 0 A 1.2.3.4
send
EOF

[ -f ns3/dynamic.db.jnl ] || { ret=1 ; echo_i "journal does not exist (posttest)" ; }

for i in 1 2 3 4 5 6 7 8 9 10
do
	ans=0
	$DIG $DIGOPTS @10.53.0.3 e.dynamic > dig.out.ns3.test$n
	grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ans=1
	grep "ANSWER: 2," dig.out.ns3.test$n > /dev/null || ans=1
	grep "1.2.3.4" dig.out.ns3.test$n > /dev/null || ans=1
	[ $ans = 0 ] && break
	sleep 1
done
[ $ans = 0 ] || { ret=1; echo_i "signed record not found"; cat dig.out.ns3.test$n ; }

if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "stop bump in the wire signer server ($n)"
ret=0
$PERL ../stop.pl inline ns3 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "restart bump in the wire signer server ($n)"
ret=0
$PERL ../start.pl --noclean --restart --port ${PORT} inline ns3 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

$NSUPDATE << EOF
zone bits
server 10.53.0.2 ${PORT}
update add bits 0 SOA ns2.bits. . 2011072450 20 20 1814400 3600
send
EOF

n=`expr $n + 1`
echo_i "checking YYYYMMDDVV (2011072450) serial on hidden master ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.2 bits SOA > dig.out.ns2.test$n
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n > /dev/null || ret=1
grep "2011072450" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking YYYYMMDDVV (2011072450) serial in signed zone ($n)"
for i in 1 2 3 4 5 6 7 8 9 10
do
	ret=0
	$DIG $DIGOPTS @10.53.0.3 bits SOA > dig.out.ns3.test$n
	grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
	grep "ANSWER: 2," dig.out.ns3.test$n > /dev/null || ret=1
	grep "2011072450" dig.out.ns3.test$n > /dev/null || ret=1
	if [ $ret = 0 ]; then break; fi
	sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

$NSUPDATE << EOF
zone noixfr
server 10.53.0.4 ${PORT}
update add noixfr 0 SOA ns4.noixfr. . 2011072450 20 20 1814400 3600
send
EOF

n=`expr $n + 1`
echo_i "checking YYYYMMDDVV (2011072450) serial on hidden master, noixfr ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.4 noixfr SOA > dig.out.ns4.test$n
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns4.test$n > /dev/null || ret=1
grep "2011072450" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking YYYYMMDDVV (2011072450) serial in signed zone, noixfr ($n)"
for i in 1 2 3 4 5 6 7 8 9 10
do
	ret=0
	$DIG $DIGOPTS @10.53.0.3 noixfr SOA > dig.out.ns3.test$n
	grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
	grep "ANSWER: 2," dig.out.ns3.test$n > /dev/null || ret=1
	grep "2011072450" dig.out.ns3.test$n > /dev/null || ret=1
	if [ $ret = 0 ]; then break; fi
	sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

$NSUPDATE << EOF
zone bits
server 10.53.0.3 ${PORT}
update add bits 0 SOA ns2.bits. . 2011072460 20 20 1814400 3600
send
EOF

n=`expr $n + 1`
echo_i "checking forwarded update on hidden master ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.2 bits SOA > dig.out.ns2.test$n
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n > /dev/null || ret=1
grep "2011072460" dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking forwarded update on signed zone ($n)"
for i in 1 2 3 4 5 6 7 8 9 10
do
	ret=0
	$DIG $DIGOPTS @10.53.0.3 bits SOA > dig.out.ns3.test$n
	grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
	grep "ANSWER: 2," dig.out.ns3.test$n > /dev/null || ret=1
	grep "2011072460" dig.out.ns3.test$n > /dev/null || ret=1
	if [ $ret = 0 ]; then break; fi
	sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

$NSUPDATE << EOF
zone noixfr
server 10.53.0.3 ${PORT}
update add noixfr 0 SOA ns4.noixfr. . 2011072460 20 20 1814400 3600
send
EOF

n=`expr $n + 1`
echo_i "checking forwarded update on hidden master, noixfr ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.4 noixfr SOA > dig.out.ns4.test$n
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns4.test$n > /dev/null || ret=1
grep "2011072460" dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking forwarded update on signed zone, noixfr ($n)"
for i in 1 2 3 4 5 6 7 8 9 10
do
	ret=0
	$DIG $DIGOPTS @10.53.0.3 noixfr SOA > dig.out.ns3.test$n
	grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
	grep "ANSWER: 2," dig.out.ns3.test$n > /dev/null || ret=1
	grep "2011072460" dig.out.ns3.test$n > /dev/null || ret=1
	if [ $ret = 0 ]; then break; fi
	sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

ret=0
n=`expr $n + 1`
echo_i "checking turning on of inline signing in a slave zone via reload ($n)"
$DIG $DIGOPTS @10.53.0.5 +dnssec bits SOA > dig.out.ns5.test$n
grep "status: NOERROR" dig.out.ns5.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns5.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "setup broken"; fi
status=`expr $status + $ret`
copy_setports ns5/named.conf.post ns5/named.conf
(cd ns5; $KEYGEN -q -a rsasha256 bits) > /dev/null 2>&1
(cd ns5; $KEYGEN -q -a rsasha256 -f KSK bits) > /dev/null 2>&1
rndc_reload ns5 10.53.0.5
for i in 1 2 3 4 5 6 7 8 9 10
do
	ret=0
	$DIG $DIGOPTS @10.53.0.5 bits SOA > dig.out.ns5.test$n
	grep "status: NOERROR" dig.out.ns5.test$n > /dev/null || ret=1
	grep "ANSWER: 2," dig.out.ns5.test$n > /dev/null || ret=1
	if [ $ret = 0 ]; then break; fi
	sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking rndc freeze/thaw of dynamic inline zone no change ($n)"
ret=0
$RNDCCMD 10.53.0.3 freeze dynamic > freeze.test$n 2>&1 || { echo_i "/' < freeze.test$n"; ret=1;  }
sleep 1
$RNDCCMD 10.53.0.3 thaw dynamic > thaw.test$n 2>&1 || { echo_i "rndc thaw dynamic failed" ; ret=1; }
sleep 1
grep "zone dynamic/IN (unsigned): ixfr-from-differences: unchanged" ns3/named.run > /dev/null ||  ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`


n=`expr $n + 1`
echo_i "checking rndc freeze/thaw of dynamic inline zone ($n)"
ret=0
$RNDCCMD 10.53.0.3 freeze dynamic > freeze.test$n 2>&1 || ret=1
sleep 1
awk '$2 == ";" && $3 ~ /serial/ { printf("%d %s %s\n", $1 + 1, $2, $3); next; }
     { print; }
     END { print "freeze1.dynamic. 0 TXT freeze1"; } ' ns3/dynamic.db > ns3/dynamic.db.new
mv ns3/dynamic.db.new ns3/dynamic.db
$RNDCCMD 10.53.0.3 thaw dynamic > thaw.test$n 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check added record freeze1.dynamic ($n)"
for i in 1 2 3 4 5 6 7 8 9
do
    ret=0
    $DIG $DIGOPTS @10.53.0.3 freeze1.dynamic TXT > dig.out.ns3.test$n
    grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
    grep "ANSWER: 2," dig.out.ns3.test$n > /dev/null || ret=1
    test $ret = 0 && break
    sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# allow 1 second so that file time stamps change
sleep 1

n=`expr $n + 1`
echo_i "checking rndc freeze/thaw of server ($n)"
ret=0
$RNDCCMD 10.53.0.3 freeze > freeze.test$n 2>&1 || ret=1
sleep 1
awk '$2 == ";" && $3 ~ /serial/ { printf("%d %s %s\n", $1 + 1, $2, $3); next; }
     { print; }
     END { print "freeze2.dynamic. 0 TXT freeze2"; } ' ns3/dynamic.db > ns3/dynamic.db.new
mv ns3/dynamic.db.new ns3/dynamic.db
$RNDCCMD 10.53.0.3 thaw > thaw.test$n 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check added record freeze2.dynamic ($n)"
for i in 1 2 3 4 5 6 7 8 9
do
    ret=0
    $DIG $DIGOPTS @10.53.0.3 freeze2.dynamic TXT > dig.out.ns3.test$n
    grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
    grep "ANSWER: 2," dig.out.ns3.test$n > /dev/null || ret=1
    test $ret = 0 && break
    sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check rndc reload allows reuse of inline-signing zones ($n)"
ret=0
{ $RNDCCMD 10.53.0.3 reload 2>&1 || ret=1 ; } | sed 's/^/ns3 /' | cat_i
grep "not reusable" ns3/named.run > /dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check rndc sync removes both signed and unsigned journals ($n)"
ret=0
[ -f ns3/dynamic.db.jnl ] || ret=1
[ -f ns3/dynamic.db.signed.jnl ] || ret=1
$RNDCCMD 10.53.0.3 sync -clean dynamic 2>&1 || ret=1
[ -f ns3/dynamic.db.jnl ] && ret=1
[ -f ns3/dynamic.db.signed.jnl ] && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

$NSUPDATE << EOF
zone retransfer
server 10.53.0.2 ${PORT}
update add added.retransfer 0 A 1.2.3.4
send

EOF

n=`expr $n + 1`
echo_i "checking that the retransfer record is added on the hidden master ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.2 added.retransfer A > dig.out.ns2.test$n
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that the change has not been transfered due to notify ($n)"
ret=0
for i in 0 1 2 3 4 5 6 7 8 9
do
	ans=0
	$DIG $DIGOPTS @10.53.0.3 added.retransfer A > dig.out.ns3.test$n
	grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ans=1
	[ $ans = 0 ] && break
	sleep 1
done
if [ $ans != 1 ]; then echo_i "failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check rndc retransfer of a inline slave zone works ($n)"
ret=0
$RNDCCMD 10.53.0.3 retransfer retransfer 2>&1 || ret=1
for i in 0 1 2 3 4 5 6 7 8 9
do
	ans=0
	$DIG $DIGOPTS @10.53.0.3 added.retransfer A > dig.out.ns3.test$n
	grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ans=1
	grep "ANSWER: 2," dig.out.ns3.test$n > /dev/null || ans=1
	[ $ans = 0 ] && break
	sleep 1
done
[ $ans = 1 ] && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check 'rndc signing -nsec3param' requests are queued for zones which are not loaded ($n)"
ret=0
# The "retransfer3" zone is configured with "allow-transfer { none; };" on ns2,
# which means it should not yet be available on ns3.
$DIG $DIGOPTS @10.53.0.3 retransfer3 SOA > dig.out.ns3.pre.test$n
grep "status: SERVFAIL" dig.out.ns3.pre.test$n > /dev/null || ret=1
# Switch the zone to NSEC3.  An "NSEC3 -> NSEC -> NSEC3" sequence is used purely
# to test that multiple queued "rndc signing -nsec3param" requests are handled
# properly.
$RNDCCMD 10.53.0.3 signing -nsec3param 1 0 0 - retransfer3 > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.3 signing -nsec3param none retransfer3 > /dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.3 signing -nsec3param 1 0 0 - retransfer3 > /dev/null 2>&1 || ret=1
# Reconfigure ns2 to allow outgoing transfers for the "retransfer3" zone.
sed "s|\(allow-transfer { none; };.*\)|// \1|;" ns2/named.conf > ns2/named.conf.new
mv ns2/named.conf.new ns2/named.conf
$RNDCCMD 10.53.0.2 reconfig || ret=1
# Request ns3 to retransfer the "retransfer3" zone.
$RNDCCMD 10.53.0.3 retransfer retransfer3 || ret=1
# Wait until ns3 finishes building the NSEC3 chain for "retransfer3".  There is
# no need to immediately set ret=1 if building the NSEC3 chain is not finished
# within the time limit because the query we will send shortly will detect any
# problems anyway.
for i in 0 1 2 3 4 5 6 7 8 9
do
	$RNDCCMD 10.53.0.3 signing -list retransfer3 > signing.out.test$n.$i 2>&1
	keys_done=`grep "Done signing" signing.out.test$n.$i | wc -l`
	nsec3_pending=`grep "NSEC3 chain" signing.out.test$n.$i | wc -l`
	test $keys_done -eq 2 -a $nsec3_pending -eq 0 && break
	sleep 1
done
# Check whether "retransfer3" uses NSEC3 as requested.
$DIG $DIGOPTS @10.53.0.3 nonexist.retransfer3 A > dig.out.ns3.post.test$n
grep "status: NXDOMAIN" dig.out.ns3.post.test$n > /dev/null || ret=1
grep "NSEC3" dig.out.ns3.post.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check rndc retransfer of a inline nsec3 slave retains nsec3 ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -nsec3param 1 0 0 - retransfer3 > /dev/null 2>&1 || ret=1
for i in 0 1 2 3 4 5 6 7 8 9
do
	ans=0
	$DIG $DIGOPTS @10.53.0.3 nonexist.retransfer3 A > dig.out.ns3.pre.test$n
	grep "status: NXDOMAIN" dig.out.ns3.pre.test$n > /dev/null || ans=1
	grep "NSEC3" dig.out.ns3.pre.test$n > /dev/null || ans=1
	[ $ans = 0 ] && break
	sleep 1
done
$RNDCCMD 10.53.0.3 retransfer retransfer3 2>&1 || ret=1
for i in 0 1 2 3 4 5 6 7 8 9
do
	ans=0
	$DIG $DIGOPTS @10.53.0.3 nonexist.retransfer3 A > dig.out.ns3.post.test$n
	grep "status: NXDOMAIN" dig.out.ns3.post.test$n > /dev/null || ans=1
	grep "NSEC3" dig.out.ns3.post.test$n > /dev/null || ans=1
	[ $ans = 0 ] && break
	sleep 1
done
[ $ans = 1 ] && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# NOTE: The test below should be considered fragile.  More details can be found
# in the comment inside ns7/named.conf.
n=`expr $n + 1`
echo_i "check rndc retransfer of a inline nsec3 slave does not trigger an infinite loop ($n)"
ret=0
zone=nsec3-loop
# Add slave zone using rndc
$RNDCCMD 10.53.0.7 addzone $zone \
	'{ type slave; masters { 10.53.0.2; }; file "'$zone'.db"; inline-signing yes; auto-dnssec maintain; };'
# Wait until slave zone is fully signed using NSEC
for i in 1 2 3 4 5 6 7 8 9 0
do
	ret=1
	$RNDCCMD 10.53.0.7 signing -list $zone > signing.out.test$n 2>&1
	keys=`grep '^Done signing' signing.out.test$n | wc -l`
	[ $keys -eq 3 ] && ret=0 && break
	sleep 1
done
# Switch slave zone to NSEC3
$RNDCCMD 10.53.0.7 signing -nsec3param 1 0 2 12345678 $zone > /dev/null 2>&1
# Wait until slave zone is fully signed using NSEC3
for i in 1 2 3 4 5 6 7 8 9 0
do
	ret=1
	nsec3param=`$DIG $DIGOPTS +nodnssec +short @10.53.0.7 nsec3param $zone`
	test "$nsec3param" = "1 0 2 12345678" && ret=0 && break
	sleep 1
done
# Attempt to retransfer the slave zone from master
$RNDCCMD 10.53.0.7 retransfer $zone
# Check whether the signer managed to fully sign the retransferred zone by
# waiting for a specific SOA serial number to appear in the logs; if this
# specific SOA serial number does not appear in the logs, it means the signer
# has either ran into an infinite loop or crashed; note that we check the logs
# instead of sending SOA queries to the signer as these may influence its
# behavior in a way which may prevent the desired scenario from being
# reproduced (see comment in ns7/named.conf)
for i in 1 2 3 4 5 6 7 8 9 0
do
	ret=1
	grep "ns2.$zone. . 10 20 20 1814400 3600" ns7/named.run > /dev/null 2>&1
	[ $? -eq 0 ] && ret=0 && break
	sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "stop bump in the wire signer server ($n)"
ret=0
$PERL ../stop.pl inline ns3 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "update SOA record while stopped"
cp ns3/master4.db.in ns3/master.db
rm ns3/master.db.jnl

n=`expr $n + 1`
echo_i "restart bump in the wire signer server ($n)"
ret=0
$PERL ../start.pl --noclean --restart --port ${PORT} inline ns3 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "updates to SOA parameters other than serial while stopped are reflected in signed zone ($n)"
ret=0
for i in 1 2 3 4 5 6 7 8 9
do
	ans=0
	$DIG $DIGOPTS @10.53.0.3 master SOA > dig.out.ns3.test$n
	grep "hostmaster" dig.out.ns3.test$n > /dev/null || ans=1
	grep "ANSWER: 2," dig.out.ns3.test$n > /dev/null || ans=1
	[ $ans = 1 ] || break
	sleep 1
done
[ $ans = 0 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that reloading all zones does not cause zone maintenance to cease for inline-signed zones ($n)"
ret=1
# Ensure "rndc reload" attempts to load ns3/master.db by waiting 1 second so
# that the master file modification time has no possibility of being equal to
# the one stored during server startup.
sleep 1
nextpart ns3/named.run > /dev/null
cp ns3/master5.db.in ns3/master.db
rndc_reload ns3 10.53.0.3
for i in 1 2 3 4 5 6 7 8 9 10
do
	if nextpart ns3/named.run | grep "zone master.*sending notifies" > /dev/null; then
		ret=0
		break
	fi
	sleep 1
done
# Sanity check: master file updates should be reflected in the signed zone,
# i.e. SOA RNAME should no longer be set to "hostmaster".
$DIG $DIGOPTS @10.53.0.3 master SOA > dig.out.ns3.test$n || ret=1
grep "hostmaster" dig.out.ns3.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that reloading errors prevent synchronization ($n)"
ret=0
$DIG $DIGOPTS +short @10.53.0.3 master SOA > dig.out.ns3.test$n.1 || ret=1
sleep 1
nextpart ns3/named.run > /dev/null
cp ns3/master5.db.in ns3/master.db
rndc_reload ns3 10.53.0.3
for i in 1 2 3 4 5 6 7 8 9 10
do
	if nextpart ns3/named.run |
           grep "not loaded due to errors" > /dev/null
        then
		break
	fi
	sleep 1
done
# Sanity check: the SOA record should be unchanged
$DIG $DIGOPTS +short @10.53.0.3 master SOA > dig.out.ns3.test$n.2 || ret=1
$DIFF dig.out.ns3.test$n.1  dig.out.ns3.test$n.2 > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "test add/del zone combinations ($n)"
ret=0
for zone in a b c d e f g h i j k l m n o p q r s t u v w x y z
do
$RNDCCMD 10.53.0.2 addzone test-$zone \
	'{ type master; file "bits.db.in"; allow-transfer { any; }; };'
$DIG $DIGOPTS @10.53.0.2 test-$zone SOA > dig.out.ns2.$zone.test$n
grep "status: NOERROR," dig.out.ns2.$zone.test$n  > /dev/null || { ret=1; cat dig.out.ns2.$zone.test$n; }
$RNDCCMD 10.53.0.3 addzone test-$zone \
	'{ type slave; masters { 10.53.0.2; }; file "'test-$zone.bk'"; inline-signing yes; auto-dnssec maintain; allow-transfer { any; }; };'
$RNDCCMD 10.53.0.3 delzone test-$zone > /dev/null 2>&1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing adding external keys to a inline zone ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.3 dnskey externalkey > dig.out.ns3.test$n
for alg in 7 13
do
   [ $alg = 13 -a ! -f checkecdsa ] && continue;

   case $alg in
   7) echo_i "checking NSEC3RSASHA1";;
   13) echo_i "checking ECDSAP256SHA256";;
   *) echo_i "checking $alg";;
   esac

   dnskeys=`grep "IN.DNSKEY.25[67] [0-9]* $alg " dig.out.ns3.test$n | wc -l`
   rrsigs=`grep "RRSIG.DNSKEY $alg " dig.out.ns3.test$n | wc -l`
   test ${dnskeys:-0} -eq 3 || { echo_i "failed $alg (dnskeys ${dnskeys:-0})"; ret=1; }
   test ${rrsigs:-0} -eq 2 || { echo_i "failed $alg (rrsigs ${rrsigs:-0})"; ret=1; }
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing imported key won't overwrite a private key ($n)"
ret=0
key=`$KEYGEN -q -a rsasha256 import.example`
cp ${key}.key import.key
# import should fail
$IMPORTKEY -f import.key import.example > /dev/null 2>&1 && ret=1
rm -f ${key}.private
# private key removed; import should now succeed
$IMPORTKEY -f import.key import.example > /dev/null 2>&1 || ret=1
# now that it's an external key, re-import should succeed
$IMPORTKEY -f import.key import.example > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing updating inline secure serial via 'rndc signing -serial' ($n)"
ret=0
$DIG $DIGOPTS nsec3. SOA @10.53.0.3 > dig.out.n3.pre.test$n
newserial=`$PERL -e 'while (<>) { chomp; my @field = split /\s+/; printf("%u\n", $field[6] + 10) if ($field[3] eq "SOA"); }' < dig.out.n3.pre.test$n`
$RNDCCMD 10.53.0.3 signing -serial ${newserial:-0} nsec3 > /dev/null 2>&1
sleep 1
$DIG $DIGOPTS nsec3. SOA @10.53.0.3 > dig.out.ns3.post.test$n
serial=`awk '$4 == "SOA" { print $7 }' dig.out.ns3.post.test$n`
[ ${newserial:-0} -eq ${serial:-1} ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing updating inline secure serial via 'rndc signing -serial' with negative change ($n)"
ret=0
$DIG $DIGOPTS nsec3. SOA @10.53.0.3 > dig.out.n3.pre.test$n
oldserial=`awk '$4 == "SOA" { print $7 }' dig.out.n3.pre.test$n`
newserial=`$PERL -e 'while (<>) { chomp; my @field = split /\s+/; printf("%u\n", $field[6] - 10) if ($field[3] eq "SOA"); }' < dig.out.n3.pre.test$n`
$RNDCCMD 10.53.0.3 signing -serial ${newserial:-0} nsec3 > /dev/null 2>&1
sleep 1
$DIG $DIGOPTS nsec3. SOA @10.53.0.3 > dig.out.ns3.post.test$n
serial=`awk '$4 == "SOA" { print $7 }' dig.out.ns3.post.test$n`
[ ${oldserial:-0} -eq ${serial:-1} ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

#
# Freezing only operates on the raw zone.
#
n=`expr $n + 1`
echo_i "testing updating inline secure serial via 'rndc signing -serial' when frozen ($n)"
ret=0
$DIG $DIGOPTS nsec3. SOA @10.53.0.3 > dig.out.n3.pre.test$n
oldserial=`awk '$4 == "SOA" { print $7 }' dig.out.n3.pre.test$n`
newserial=`$PERL -e 'while (<>) { chomp; my @field = split /\s+/; printf("%u\n", $field[6] + 10) if ($field[3] eq "SOA"); }' < dig.out.n3.pre.test$n`
$RNDCCMD 10.53.0.3 freeze nsec3 > /dev/null 2>&1
$RNDCCMD 10.53.0.3 signing -serial ${newserial:-0} nsec3 > /dev/null 2>&1
$RNDCCMD 10.53.0.3 thaw nsec3 > /dev/null 2>&1
sleep 1
$DIG $DIGOPTS nsec3. SOA @10.53.0.3 > dig.out.ns3.post.test$n
serial=`awk '$4 == "SOA" { print $7 }' dig.out.ns3.post.test$n`
[ ${newserial:-0} -eq ${serial:-1} ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing updating dynamic serial via 'rndc signing -serial' ($n)"
ret=0
$DIG $DIGOPTS bits. SOA @10.53.0.2 > dig.out.ns2.pre.test$n
newserial=`$PERL -e 'while (<>) { chomp; my @field = split /\s+/; printf("%u\n", $field[6] + 10) if ($field[3] eq "SOA"); }' < dig.out.ns2.pre.test$n`
$RNDCCMD 10.53.0.2 signing -serial ${newserial:-0} bits > /dev/null 2>&1
sleep 1
$DIG $DIGOPTS bits. SOA @10.53.0.2 > dig.out.ns2.post.test$n
serial=`awk '$4 == "SOA" { print $7 }' dig.out.ns2.post.test$n`
[ ${newserial:-0} -eq ${serial:-1} ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing updating dynamic serial via 'rndc signing -serial' with negative change ($n)"
ret=0
$DIG $DIGOPTS bits. SOA @10.53.0.2 > dig.out.ns2.pre.test$n
oldserial=`awk '$4 == "SOA" { print $7 }' dig.out.ns2.pre.test$n`
newserial=`$PERL -e 'while (<>) { chomp; my @field = split /\s+/; printf("%u\n", $field[6] - 10) if ($field[3] eq "SOA"); }' < dig.out.ns2.pre.test$n`
$RNDCCMD 10.53.0.2 signing -serial ${newserial:-0} bits > /dev/null 2>&1
sleep 1
$DIG $DIGOPTS bits. SOA @10.53.0.2 > dig.out.ns2.post.test$n
serial=`awk '$4 == "SOA" { print $7 }' dig.out.ns2.post.test$n`
[ ${oldserial:-0} -eq ${serial:-1} ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing updating dynamic serial via 'rndc signing -serial' when frozen ($n)"
ret=0
$DIG $DIGOPTS bits. SOA @10.53.0.2 > dig.out.ns2.pre.test$n
oldserial=`awk '$4 == "SOA" { print $7 }' dig.out.ns2.pre.test$n`
newserial=`$PERL -e 'while (<>) { chomp; my @field = split /\s+/; printf("%u\n", $field[6] + 10) if ($field[3] eq "SOA"); }' < dig.out.ns2.pre.test$n`
$RNDCCMD 10.53.0.2 freeze bits > /dev/null 2>&1
$RNDCCMD 10.53.0.2 signing -serial ${newserial:-0} bits > /dev/null 2>&1
$RNDCCMD 10.53.0.2 thaw bits > /dev/null 2>&1
sleep 1
$DIG $DIGOPTS bits. SOA @10.53.0.2 > dig.out.ns2.post.test$n
serial=`awk '$4 == "SOA" { print $7 }' dig.out.ns2.post.test$n`
[ ${oldserial:-0} -eq ${serial:-1} ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing that inline signing works with inactive ZSK and active KSK ($n)"
ret=0

$DIG $DIGOPTS @10.53.0.3 soa inactivezsk  > dig.out.ns3.pre.test$n || ret=1
soa1=`awk '$4 == "SOA" { print $7 }' dig.out.ns3.pre.test$n`

$NSUPDATE << EOF
server 10.53.0.2 ${PORT}
update add added.inactivezsk 0 IN TXT added record
send
EOF

for i in 1 2 3 4 5 6 7 8 9 10
do
    $DIG $DIGOPTS @10.53.0.3 soa inactivezsk  > dig.out.ns3.post.test$n || ret=1
    soa2=`awk '$4 == "SOA" { print $7 }' dig.out.ns3.post.test$n`
    test ${soa1:-0} -ne ${soa2:-0} && break
    sleep 1
done
test ${soa1:-0} -ne ${soa2:-0} || ret=1

$DIG $DIGOPTS @10.53.0.3 txt added.inactivezsk > dig.out.ns3.test$n || ret=1
grep "ANSWER: 3," dig.out.ns3.test$n > /dev/null || ret=1
grep "RRSIG" dig.out.ns3.test$n > /dev/null || ret=1
grep "TXT 7 2" dig.out.ns3.test$n > /dev/null || ret=1
grep "TXT 8 2" dig.out.ns3.test$n > /dev/null || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing that inline signing works with inactive KSK and active ZSK ($n)"
ret=0

$DIG $DIGOPTS @10.53.0.3 axfr inactiveksk > dig.out.ns3.test$n

#
#  check that DNSKEY is signed with ZSK for algorithm 7
#
awk='$4 == "DNSKEY" && $5 == 256 && $7 == 7 { print }'
zskid=`awk "${awk}" dig.out.ns3.test$n |
       $DSFROMKEY -A -2 -f - inactiveksk | awk '{ print $4}' `
grep "DNSKEY 7 1 [0-9]* [0-9]* [0-9]* ${zskid} " dig.out.ns3.test$n > /dev/null || ret=1
awk='$4 == "DNSKEY" && $5 == 257 && $7 == 7 { print }'
kskid=`awk "${awk}" dig.out.ns3.test$n |
       $DSFROMKEY -2 -f - inactiveksk | awk '{ print $4}' `
grep "DNSKEY 7 1 [0-9]* [0-9]* [0-9]* ${kskid} " dig.out.ns3.test$n > /dev/null && ret=1

#
#  check that DNSKEY is signed with KSK for algorithm 8
#
awk='$4 == "DNSKEY" && $5 == 256 && $7 == 8 { print }'
zskid=`awk "${awk}" dig.out.ns3.test$n |
       $DSFROMKEY -A -2 -f - inactiveksk | awk '{ print $4}' `
grep "DNSKEY 8 1 [0-9]* [0-9]* [0-9]* ${zskid} " dig.out.ns3.test$n > /dev/null && ret=1
awk='$4 == "DNSKEY" && $5 == 257 && $7 == 8 { print }'
kskid=`awk "${awk}" dig.out.ns3.test$n |
       $DSFROMKEY -2 -f - inactiveksk | awk '{ print $4}' `
grep "DNSKEY 8 1 [0-9]* [0-9]* [0-9]* ${kskid} " dig.out.ns3.test$n > /dev/null || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Wait until an update to the raw part of a given inline signed zone is fully
# processed.  As waiting for a fixed amount of time is suboptimal and there is
# no single message that would signify both a successful modification and an
# error in a race-free manner, instead wait until either notifies are sent
# (which means the secure zone was modified) or a receive_secure_serial() error
# is logged (which means the zone was not modified and will not be modified any
# further in response to the relevant raw zone update).
wait_until_raw_zone_update_is_processed() {
	zone="$1"
	for i in 1 2 3 4 5 6 7 8 9 10
	do
		if nextpart ns3/named.run | egrep "zone ${zone}.*(sending notifies|receive_secure_serial)" > /dev/null; then
			return
		fi
		sleep 1
	done
}

n=`expr $n + 1`
echo_i "checking that changes to raw zone are applied to a previously unsigned secure zone ($n)"
ret=0
# Query for bar.nokeys/A and ensure the response is negative.  As this zone
# does not have any signing keys set up, the response must be unsigned.
$DIG $DIGOPTS @10.53.0.3 bar.nokeys. A > dig.out.ns3.pre.test$n 2>&1 || ret=1
grep "status: NOERROR" dig.out.ns3.pre.test$n > /dev/null && ret=1
grep "RRSIG" dig.out.ns3.pre.test$n > /dev/null && ret=1
# Ensure the wait_until_raw_zone_update_is_processed() call below will ignore
# log messages generated before the raw zone is updated.
nextpart ns3/named.run > /dev/null
# Add a record to the raw zone on the master.
$NSUPDATE << EOF || ret=1
zone nokeys.
server 10.53.0.2 ${PORT}
update add bar.nokeys. 0 A 127.0.0.1
send
EOF
wait_until_raw_zone_update_is_processed "nokeys"
# Query for bar.nokeys/A again and ensure the signer now returns a positive,
# yet still unsigned response.
$DIG $DIGOPTS @10.53.0.3 bar.nokeys. A > dig.out.ns3.post.test$n 2>&1
grep "status: NOERROR" dig.out.ns3.post.test$n > /dev/null || ret=1
grep "RRSIG" dig.out.ns3.pre.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that changes to raw zone are not applied to a previously signed secure zone with no keys available (primary) ($n)"
ret=0
# Query for bar.removedkeys-primary/A and ensure the response is negative.  As
# this zone has signing keys set up, the response must be signed.
$DIG $DIGOPTS @10.53.0.3 bar.removedkeys-primary. A > dig.out.ns3.pre.test$n 2>&1 || ret=1
grep "status: NOERROR" dig.out.ns3.pre.test$n > /dev/null && ret=1
grep "RRSIG" dig.out.ns3.pre.test$n > /dev/null || ret=1
# Remove the signing keys for this zone.
mv -f ns3/Kremovedkeys-primary* ns3/removedkeys
# Ensure the wait_until_raw_zone_update_is_processed() call below will ignore
# log messages generated before the raw zone is updated.
nextpart ns3/named.run > /dev/null
# Add a record to the raw zone on the master.
$NSUPDATE << EOF || ret=1
zone removedkeys-primary.
server 10.53.0.3 ${PORT}
update add bar.removedkeys-primary. 0 A 127.0.0.1
send
EOF
wait_until_raw_zone_update_is_processed "removedkeys-primary"
# Query for bar.removedkeys-primary/A again and ensure the signer still returns
# a negative, signed response.
$DIG $DIGOPTS @10.53.0.3 bar.removedkeys-primary. A > dig.out.ns3.post.test$n 2>&1
grep "status: NOERROR" dig.out.ns3.post.test$n > /dev/null && ret=1
grep "RRSIG" dig.out.ns3.pre.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that backlogged changes to raw zone are applied after keys become available (primary) ($n)"
ret=0
# Restore the signing keys for this zone.
mv ns3/removedkeys/Kremovedkeys-primary* ns3
$RNDCCMD 10.53.0.3 loadkeys removedkeys-primary > /dev/null 2>&1
# Determine what a SOA record with a bumped serial number should look like.
BUMPED_SOA=`sed -n 's/.*\(add removedkeys-primary.*IN.*SOA\)/\1/p;' ns3/named.run | tail -1 | awk '{$8 += 1; print $0}'`
# Ensure the wait_until_raw_zone_update_is_processed() call below will ignore
# log messages generated before the raw zone is updated.
nextpart ns3/named.run > /dev/null
# Bump the SOA serial number of the raw zone.
$NSUPDATE << EOF || ret=1
zone removedkeys-primary.
server 10.53.0.3 ${PORT}
update del removedkeys-primary. SOA
update ${BUMPED_SOA}
send
EOF
wait_until_raw_zone_update_is_processed "removedkeys-primary"
# Query for bar.removedkeys-primary/A again and ensure the signer now returns a
# positive, signed response.
$DIG $DIGOPTS @10.53.0.3 bar.removedkeys-primary. A > dig.out.ns3.test$n 2>&1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
grep "RRSIG" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that changes to raw zone are not applied to a previously signed secure zone with no keys available (secondary) ($n)"
ret=0
# Query for bar.removedkeys-secondary/A and ensure the response is negative.  As this
# zone does have signing keys set up, the response must be signed.
$DIG $DIGOPTS @10.53.0.3 bar.removedkeys-secondary. A > dig.out.ns3.pre.test$n 2>&1 || ret=1
grep "status: NOERROR" dig.out.ns3.pre.test$n > /dev/null && ret=1
grep "RRSIG" dig.out.ns3.pre.test$n > /dev/null || ret=1
# Remove the signing keys for this zone.
mv -f ns3/Kremovedkeys-secondary* ns3/removedkeys
# Ensure the wait_until_raw_zone_update_is_processed() call below will ignore
# log messages generated before the raw zone is updated.
nextpart ns3/named.run > /dev/null
# Add a record to the raw zone on the master.
$NSUPDATE << EOF || ret=1
zone removedkeys-secondary.
server 10.53.0.2 ${PORT}
update add bar.removedkeys-secondary. 0 A 127.0.0.1
send
EOF
wait_until_raw_zone_update_is_processed "removedkeys-secondary"
# Query for bar.removedkeys-secondary/A again and ensure the signer still returns a
# negative, signed response.
$DIG $DIGOPTS @10.53.0.3 bar.removedkeys-secondary. A > dig.out.ns3.post.test$n 2>&1
grep "status: NOERROR" dig.out.ns3.post.test$n > /dev/null && ret=1
grep "RRSIG" dig.out.ns3.pre.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that backlogged changes to raw zone are applied after keys become available (secondary) ($n)"
ret=0
# Restore the signing keys for this zone.
mv ns3/removedkeys/Kremovedkeys-secondary* ns3
$RNDCCMD 10.53.0.3 loadkeys removedkeys-secondary > /dev/null 2>&1
# Determine what a SOA record with a bumped serial number should look like.
BUMPED_SOA=`sed -n 's/.*\(add removedkeys-secondary.*IN.*SOA\)/\1/p;' ns2/named.run | tail -1 | awk '{$8 += 1; print $0}'`
# Ensure the wait_until_raw_zone_update_is_processed() call below will ignore
# log messages generated before the raw zone is updated.
nextpart ns3/named.run > /dev/null
# Bump the SOA serial number of the raw zone on the master.
$NSUPDATE << EOF || ret=1
zone removedkeys-secondary.
server 10.53.0.2 ${PORT}
update del removedkeys-secondary. SOA
update ${BUMPED_SOA}
send
EOF
wait_until_raw_zone_update_is_processed "removedkeys-secondary"
# Query for bar.removedkeys-secondary/A again and ensure the signer now returns
# a positive, signed response.
$DIG $DIGOPTS @10.53.0.3 bar.removedkeys-secondary. A > dig.out.ns3.test$n 2>&1
grep "status: NOERROR" dig.out.ns3.test$n > /dev/null || ret=1
grep "RRSIG" dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Check that the master file $2 for zone $1 does not contain RRSIG records
# while the journal file for that zone does contain them.
ensure_sigs_only_in_journal() {
	origin="$1"
	masterfile="$2"
	$CHECKZONE -i none -f raw -D -o - "$origin" "$masterfile" 2>&1 | grep -w RRSIG > /dev/null && ret=1
	$CHECKZONE -j -i none -f raw -D -o - "$origin" "$masterfile" 2>&1 | grep -w RRSIG > /dev/null || ret=1
}

n=`expr $n + 1`
echo_i "checking that records added from a journal are scheduled to be resigned ($n)"
ret=0
# Signing keys for the "delayedkeys" zone are not yet accessible.  Thus, the
# zone file for the signed version of the zone will contain no DNSSEC records.
# Move keys into place now and load them, which will cause DNSSEC records to
# only be present in the journal for the signed version of the zone.
mv Kdelayedkeys* ns3/
$RNDCCMD 10.53.0.3 loadkeys delayedkeys > rndc.out.ns3.pre.test$n 2>&1 || ret=1
# Wait until the zone is signed.
ans=1
for i in 1 2 3 4 5 6 7 8 9 10
do
	$RNDCCMD 10.53.0.3 signing -list delayedkeys > signing.out.test$n 2>&1
	num=`grep "Done signing with" signing.out.test$n | wc -l`
	if [ $num -eq 2 ]; then
		ans=0
		break
	fi
	sleep 1
done
if [ $ans != 0 ]; then ret=1; fi
# Halt rather than stopping the server to prevent the master file from being
# flushed upon shutdown since we specifically want to avoid it.
$PERL $SYSTEMTESTTOP/stop.pl --use-rndc --halt --port ${CONTROLPORT} inline ns3
ensure_sigs_only_in_journal delayedkeys ns3/delayedkeys.db.signed
$PERL $SYSTEMTESTTOP/start.pl --noclean --restart --port ${PORT} inline ns3
# At this point, the raw zone journal will not have a source serial set.  Upon
# server startup, receive_secure_serial() will rectify that, update SOA, resign
# it, and schedule its future resign.  This will cause "rndc zonestatus" to
# return delayedkeys/SOA as the next node to resign, so we restart the server
# once again; with the raw zone journal now having a source serial set,
# receive_secure_serial() should refrain from introducing any zone changes.
$PERL $SYSTEMTESTTOP/stop.pl --use-rndc --halt --port ${CONTROLPORT} inline ns3
ensure_sigs_only_in_journal delayedkeys ns3/delayedkeys.db.signed
$PERL $SYSTEMTESTTOP/start.pl --noclean --restart --port ${PORT} inline ns3
# We can now test whether the secure zone journal was correctly processed:
# unless the records contained in it were scheduled for resigning, no resigning
# event will be scheduled at all since the secure zone master file contains no
# DNSSEC records.
$RNDCCMD 10.53.0.3 zonestatus delayedkeys > rndc.out.ns3.post.test$n 2>&1 || ret=1
grep "next resign node:" rndc.out.ns3.post.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "check that zonestatus reports 'type: master' for a inline master zone ($n)"
ret=0
$RNDCCMD 10.53.0.3 zonestatus master > rndc.out.ns3.test$n
grep "type: master" rndc.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check that zonestatus reports 'type: slave' for a inline slave zone ($n)"
ret=0
$RNDCCMD 10.53.0.3 zonestatus bits > rndc.out.ns3.test$n
grep "type: slave" rndc.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
