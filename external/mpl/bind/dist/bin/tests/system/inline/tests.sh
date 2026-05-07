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

set -e

. ../conf.sh

DIGOPTS="+tcp +dnssec -p ${PORT}"
RNDCCMD="$RNDC -c ../_common/rndc.conf -p ${CONTROLPORT} -s"

dig_with_opts() {
  $DIG $DIGOPTS "$@"
}

rndccmd() {
  $RNDCCMD "$@"
}

wait_for_serial() (
  $DIG $DIGOPTS "@$1" "$2" SOA >"$4"
  serial=$(awk '$4 == "SOA" { print $7 }' "$4")
  [ "$3" -eq "${serial:--1}" ]
)

status=0
n=0
ret=0

# Make sure nsec3 zone is NSEC3 signed.
for i in 1 2 3 4 5 6 7 8 9 0; do
  nsec3param=$($DIG $DIGOPTS +nodnssec +short @10.53.0.3 nsec3param nsec3.) || ret=1
  test "$nsec3param" = "1 0 0 -" && break
  sleep 1
done

if [ $ret != 0 ]; then
  echo_i "pre-condition failed, test aborted"
  exit 1
fi

n=$((n + 1))
echo_i "checking that an unsupported algorithm is not used for signing ($n)"
ret=0
grep -q "algorithm is unsupported" ns3/named.run || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that rrsigs are replaced with ksk only ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.3 axfr nsec3. \
  | awk '/RRSIG NSEC3/ {a[$1]++} END { for (i in a) {if (a[i] != 1) exit (1)}}' || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that the zone is signed on initial transfer ($n)"
ret=0
zone_is_signed() {
  $DIG $DIGOPTS @10.53.0.3 bits. AXFR >dig.out.ns3.test$n || return 1
  $VERIFY -z -o bits. dig.out.ns3.test$n >verify.out.bits.test$n || return 1
  return 0
}
retry_quiet 10 zone_is_signed || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking expired signatures are updated on load ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.3 +noall +answer +dnssec expired SOA >dig.out.ns3.test$n || ret=1
expiry=$(awk '$4 == "RRSIG" { print $9 }' dig.out.ns3.test$n)
[ "$expiry" = "20110101000000" ] && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking removal of private type record via 'rndc signing -clear' ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -list bits >signing.out.test$n 2>&1 || ret=1
keys=$(sed -n -e 's/Done signing with key \(.*\)$/\1/p' signing.out.test$n)
for key in $keys; do
  $RNDCCMD 10.53.0.3 signing -clear ${key} bits >/dev/null || ret=1
  break # We only want to remove 1 record for now.
done 2>&1 | sed 's/^/ns3 /' | cat_i

for i in 1 2 3 4 5 6 7 8 9 10; do
  ans=0
  $RNDCCMD 10.53.0.3 signing -list bits >signing.out.test$n 2>&1 || ret=1
  num=$(grep "Done signing with" signing.out.test$n | wc -l)
  [ $num = 1 ] && break
  sleep 1
done
[ $ans = 0 ] || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking private type was properly signed ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.6 bits TYPE65534 >dig.out.ns6.test$n || ret=1
# One private type record, one signature
grep "ANSWER: 2," dig.out.ns6.test$n >/dev/null || ret=1
grep "flags:.* ad[ ;]" dig.out.ns6.test$n >/dev/null || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking removal of remaining private type record via 'rndc signing -clear all' ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -clear all bits >signing.out.test$n.clear || ret=1

for i in 1 2 3 4 5 6 7 8 9 10; do
  ans=0
  $RNDCCMD 10.53.0.3 signing -list bits >signing.out.test$n 2>&1 || ret=1
  grep "No signing records found" signing.out.test$n >/dev/null || ans=1
  [ $ans = 1 ] || break
  sleep 1
done
[ $ans = 0 ] || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking negative private type response was properly signed ($n)"
ret=0
sleep 1
$DIG $DIGOPTS @10.53.0.6 bits TYPE65534 >dig.out.ns6.test$n || ret=1
grep "status: NOERROR" dig.out.ns6.test$n >/dev/null || ret=1
grep "ANSWER: 0," dig.out.ns6.test$n >/dev/null || ret=1
grep "flags:.* ad[ ;]" dig.out.ns6.test$n >/dev/null || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that the record is added on the hidden primary ($n)"
ret=0

$NSUPDATE <<EOF || ret=1
zone bits
server 10.53.0.2 ${PORT}
update add added.bits 0 A 1.2.3.4
send
EOF

$DIG $DIGOPTS @10.53.0.2 added.bits A >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that update has been transferred and has been signed ($n)"
ret=0
for i in 1 2 3 4 5 6 7 8 9 10; do
  ret=0
  $DIG $DIGOPTS @10.53.0.3 added.bits A >dig.out.ns3.test$n || ret=1
  grep "status: NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
  grep "ANSWER: 2," dig.out.ns3.test$n >/dev/null || ret=1
  if [ $ret = 0 ]; then break; fi
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking YYYYMMDDVV (2011072400) serial on hidden primary ($n)"
ret=0

$NSUPDATE <<EOF || ret=1
zone bits
server 10.53.0.2 ${PORT}
update add bits 0 SOA ns2.bits. . 2011072400 20 20 1814400 3600
send
EOF

$DIG $DIGOPTS @10.53.0.2 bits SOA >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "2011072400" dig.out.ns2.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking YYYYMMDDVV (2011072400) serial in signed zone ($n)"
for i in 1 2 3 4 5 6 7 8 9 10; do
  ret=0
  $DIG $DIGOPTS @10.53.0.3 bits SOA >dig.out.ns3.test$n || ret=1
  grep "status: NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
  grep "ANSWER: 2," dig.out.ns3.test$n >/dev/null || ret=1
  grep "2011072400" dig.out.ns3.test$n >/dev/null || ret=1
  if [ $ret = 0 ]; then break; fi
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that the zone is signed on initial transfer, noixfr ($n)"
ret=0
for i in 1 2 3 4 5 6 7 8 9 10 1 2 3 4 5 6 7 8 9 10 1 2 3 4 5 6 7 8 9 10; do
  ret=0
  $RNDCCMD 10.53.0.3 signing -list noixfr >signing.out.test$n 2>&1 || ret=1
  keys=$(grep '^Done signing' signing.out.test$n | wc -l)
  [ $keys = 2 ] || ret=1
  if [ $ret = 0 ]; then break; fi
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that the record is added on the hidden primary, noixfr ($n)"
ret=0

$NSUPDATE <<EOF || ret=1
zone noixfr
server 10.53.0.4 ${PORT}
update add added.noixfr 0 A 1.2.3.4
send
EOF

$DIG $DIGOPTS @10.53.0.4 added.noixfr A >dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns4.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that update has been transferred and has been signed, noixfr ($n)"
ret=0
for i in 1 2 3 4 5 6 7 8 9 10 1 2 3 4 5 6 7 8 9 10 1 2 3 4 5 6 7 8 9 10; do
  ret=0
  $DIG $DIGOPTS @10.53.0.3 added.noixfr A >dig.out.ns3.test$n || ret=1
  grep "status: NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
  grep "ANSWER: 2," dig.out.ns3.test$n >/dev/null || ret=1
  if [ $ret = 0 ]; then break; fi
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking YYYYMMDDVV (2011072400) serial on hidden primary, noixfr ($n)"
ret=0

$NSUPDATE <<EOF || ret=1
zone noixfr
server 10.53.0.4 ${PORT}
update add noixfr 0 SOA ns4.noixfr. . 2011072400 20 20 1814400 3600
send
EOF

$DIG $DIGOPTS @10.53.0.4 noixfr SOA >dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns4.test$n >/dev/null || ret=1
grep "2011072400" dig.out.ns4.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking YYYYMMDDVV (2011072400) serial in signed zone, noixfr ($n)"
for i in 1 2 3 4 5 6 7 8 9 10; do
  ret=0
  $DIG $DIGOPTS @10.53.0.3 noixfr SOA >dig.out.ns3.test$n || ret=1
  grep "status: NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
  grep "ANSWER: 2," dig.out.ns3.test$n >/dev/null || ret=1
  grep "2011072400" dig.out.ns3.test$n >/dev/null || ret=1
  if [ $ret = 0 ]; then break; fi
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that the primary zone signed on initial load ($n)"
ret=0
for i in 1 2 3 4 5 6 7 8 9 10; do
  ret=0
  $RNDCCMD 10.53.0.3 signing -list primary >signing.out.test$n 2>&1 || ret=1
  keys=$(grep '^Done signing' signing.out.test$n | wc -l)
  [ $keys = 2 ] || ret=1
  if [ $ret = 0 ]; then break; fi
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking removal of private type record via 'rndc signing -clear' (primary) ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -list primary >signing.out.test$n 2>&1 || ret=1
keys=$(sed -n -e 's/Done signing with key \(.*\)$/\1/p' signing.out.test$n)
for key in $keys; do
  $RNDCCMD 10.53.0.3 signing -clear ${key} primary >/dev/null || ret=1
  break # We only want to remove 1 record for now.
done 2>&1 | sed 's/^/ns3 /' | cat_i

for i in 1 2 3 4 5 6 7 8 9; do
  ans=0
  $RNDCCMD 10.53.0.3 signing -list primary >signing.out.test$n 2>&1 || ret=1
  num=$(grep "Done signing with" signing.out.test$n | wc -l)
  [ $num = 1 ] && break
  sleep 1
done
[ $ans = 0 ] || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking private type was properly signed (primary) ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.6 primary TYPE65534 >dig.out.ns6.test$n || ret=1
grep "ANSWER: 2," dig.out.ns6.test$n >/dev/null || ret=1
grep "flags:.* ad[ ;]" dig.out.ns6.test$n >/dev/null || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking removal of remaining private type record via 'rndc signing -clear' (primary) ($n)"
ret=0
$RNDCCMD 10.53.0.3 signing -clear all primary >/dev/null || ret=1
for i in 1 2 3 4 5 6 7 8 9 10; do
  ans=0
  $RNDCCMD 10.53.0.3 signing -list primary >signing.out.test$n 2>&1 || ret=1
  grep "No signing records found" signing.out.test$n >/dev/null || ans=1
  [ $ans = 1 ] || break
  sleep 1
done
[ $ans = 0 ] || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check adding of record to unsigned primary ($n)"
ret=0
cp ns3/primary2.db.in ns3/primary.db
rndc_reload ns3 10.53.0.3 primary
for i in 1 2 3 4 5 6 7 8 9; do
  ans=0
  $DIG $DIGOPTS @10.53.0.3 e.primary A >dig.out.ns3.test$n || ret=1
  grep "10.0.0.5" dig.out.ns3.test$n >/dev/null || ans=1
  grep "ANSWER: 2," dig.out.ns3.test$n >/dev/null || ans=1
  [ $ans = 1 ] || break
  sleep 1
done
[ $ans = 0 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check adding record fails when SOA serial not changed ($n)"
ret=0
echo "c A 10.0.0.3" >>ns3/primary.db
rndc_reload ns3 10.53.0.3
sleep 1
$DIG $DIGOPTS @10.53.0.3 c.primary A >dig.out.ns3.test$n || ret=1
grep "NXDOMAIN" dig.out.ns3.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check adding record works after updating SOA serial ($n)"
ret=0
cp ns3/primary3.db.in ns3/primary.db
$RNDCCMD 10.53.0.3 reload primary 2>&1 | sed 's/^/ns3 /' | cat_i
for i in 1 2 3 4 5 6 7 8 9; do
  ans=0
  $DIG $DIGOPTS @10.53.0.3 c.primary A >dig.out.ns3.test$n || ret=1
  grep "10.0.0.3" dig.out.ns3.test$n >/dev/null || ans=1
  grep "ANSWER: 2," dig.out.ns3.test$n >/dev/null || ans=1
  [ $ans = 1 ] || break
  sleep 1
done
[ $ans = 0 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check the added record was properly signed ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.3 e.primary A >dig.out.ns6.test$n || ret=1
grep "10.0.0.5" dig.out.ns6.test$n >/dev/null || ans=1
grep "ANSWER: 2," dig.out.ns6.test$n >/dev/null || ans=1
grep "flags:.* ad[ ;]" dig.out.ns6.test$n >/dev/null || ans=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that the dynamic primary zone signed on initial load ($n)"
ret=0
for i in 1 2 3 4 5 6 7 8 9 10; do
  ret=0
  $RNDCCMD 10.53.0.3 signing -list dynamic >signing.out.test$n 2>&1 || ret=1
  keys=$(grep '^Done signing' signing.out.test$n | wc -l)
  [ $keys = 2 ] || ret=1
  if [ $ret = 0 ]; then break; fi
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking primary zone that was updated while offline is correct ($n)"
ret=0
$DIG $DIGOPTS +nodnssec +short @10.53.0.3 updated SOA >dig.out.ns3.soa.test$n || ret=1
serial=$(awk '{print $3}' dig.out.ns3.soa.test$n)
# serial should have changed
[ "$serial" = "2000042407" ] && ret=1
# e.updated should exist and should be signed
$DIG $DIGOPTS @10.53.0.3 e.updated A >dig.out.ns3.test$n || ret=1
grep "status: NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
grep "ANSWER: 2," dig.out.ns3.test$n >/dev/null || ret=1
# updated.db.signed.jnl should exist, should have the source serial
# of primary2.db, and should show a minimal diff: no more than 8 added
# records (SOA/RRSIG, 2 x NSEC/RRSIG, A/RRSIG), and 4 removed records
# (SOA/RRSIG, NSEC/RRSIG).
$JOURNALPRINT ns3/updated.db.signed.jnl >journalprint.out.test$n || ret=1
serial=$(awk '/Source serial =/ {print $4}' journalprint.out.test$n)
[ "$serial" = "2000042408" ] || ret=1
diffsize=$(wc -l <journalprint.out.test$n)
[ "$diffsize" -le 13 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking adding of record to unsigned primary using UPDATE ($n)"
ret=0

[ -f ns3/dynamic.db.jnl ] && {
  ret=1
  echo_i "journal exists (pretest)"
}

$NSUPDATE <<EOF || ret=1
zone dynamic
server 10.53.0.3 ${PORT}
update add e.dynamic 0 A 1.2.3.4
send
EOF

[ -f ns3/dynamic.db.jnl ] || {
  ret=1
  echo_i "journal does not exist (posttest)"
}

for i in 1 2 3 4 5 6 7 8 9 10; do
  ans=0
  $DIG $DIGOPTS @10.53.0.3 e.dynamic >dig.out.ns3.test$n || ret=1
  grep "status: NOERROR" dig.out.ns3.test$n >/dev/null || ans=1
  grep "ANSWER: 2," dig.out.ns3.test$n >/dev/null || ans=1
  grep "1.2.3.4" dig.out.ns3.test$n >/dev/null || ans=1
  [ $ans = 0 ] && break
  sleep 1
done
[ $ans = 0 ] || {
  ret=1
  echo_i "signed record not found"
  cat dig.out.ns3.test$n
}

if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "stop bump in the wire signer server ($n)"
ret=0
stop_server ns3 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "restart bump in the wire signer server ($n)"
ret=0
start_server --noclean --restart --port ${PORT} ns3 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking YYYYMMDDVV (2011072450) serial on hidden primary ($n)"
ret=0

$NSUPDATE <<EOF || ret=1
zone bits
server 10.53.0.2 ${PORT}
update add bits 0 SOA ns2.bits. . 2011072450 20 20 1814400 3600
send
EOF

$DIG $DIGOPTS @10.53.0.2 bits SOA >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "2011072450" dig.out.ns2.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking YYYYMMDDVV (2011072450) serial in signed zone ($n)"
for i in 1 2 3 4 5 6 7 8 9 10; do
  ret=0
  $DIG $DIGOPTS @10.53.0.3 bits SOA >dig.out.ns3.test$n || ret=1
  grep "status: NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
  grep "ANSWER: 2," dig.out.ns3.test$n >/dev/null || ret=1
  grep "2011072450" dig.out.ns3.test$n >/dev/null || ret=1
  if [ $ret = 0 ]; then break; fi
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking YYYYMMDDVV (2011072450) serial on hidden primary, noixfr ($n)"
ret=0

$NSUPDATE <<EOF || ret=1
zone noixfr
server 10.53.0.4 ${PORT}
update add noixfr 0 SOA ns4.noixfr. . 2011072450 20 20 1814400 3600
send
EOF

$DIG $DIGOPTS @10.53.0.4 noixfr SOA >dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns4.test$n >/dev/null || ret=1
grep "2011072450" dig.out.ns4.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking YYYYMMDDVV (2011072450) serial in signed zone, noixfr ($n)"
for i in 1 2 3 4 5 6 7 8 9 10; do
  ret=0
  $DIG $DIGOPTS @10.53.0.3 noixfr SOA >dig.out.ns3.test$n || ret=1
  grep "status: NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
  grep "ANSWER: 2," dig.out.ns3.test$n >/dev/null || ret=1
  grep "2011072450" dig.out.ns3.test$n >/dev/null || ret=1
  if [ $ret = 0 ]; then break; fi
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking forwarded update on hidden primary ($n)"
ret=0

$NSUPDATE <<EOF || ret=1
zone bits
server 10.53.0.3 ${PORT}
update add bits 0 SOA ns2.bits. . 2011072460 20 20 1814400 3600
send
EOF

$DIG $DIGOPTS @10.53.0.2 bits SOA >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
grep "2011072460" dig.out.ns2.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking forwarded update on signed zone ($n)"
for i in 1 2 3 4 5 6 7 8 9 10; do
  ret=0
  $DIG $DIGOPTS @10.53.0.3 bits SOA >dig.out.ns3.test$n || ret=1
  grep "status: NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
  grep "ANSWER: 2," dig.out.ns3.test$n >/dev/null || ret=1
  grep "2011072460" dig.out.ns3.test$n >/dev/null || ret=1
  if [ $ret = 0 ]; then break; fi
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking forwarded update on hidden primary, noixfr ($n)"
ret=0

$NSUPDATE <<EOF || ret=1
zone noixfr
server 10.53.0.3 ${PORT}
update add noixfr 0 SOA ns4.noixfr. . 2011072460 20 20 1814400 3600
send
EOF

$DIG $DIGOPTS @10.53.0.4 noixfr SOA >dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns4.test$n >/dev/null || ret=1
grep "2011072460" dig.out.ns4.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking forwarded update on signed zone, noixfr ($n)"
for i in 1 2 3 4 5 6 7 8 9 10; do
  ret=0
  $DIG $DIGOPTS @10.53.0.3 noixfr SOA >dig.out.ns3.test$n || ret=1
  grep "status: NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
  grep "ANSWER: 2," dig.out.ns3.test$n >/dev/null || ret=1
  grep "2011072460" dig.out.ns3.test$n >/dev/null || ret=1
  if [ $ret = 0 ]; then break; fi
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

ret=0
n=$((n + 1))
echo_i "checking turning on of inline signing in a secondary zone via reload ($n)"
$DIG $DIGOPTS @10.53.0.5 +dnssec bits SOA >dig.out.ns5.test$n || ret=1
grep "status: NOERROR" dig.out.ns5.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns5.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "setup broken"; fi
status=$((status + ret))
cp ns5/named2.conf ns5/named.conf
(
  cd ns5
  $KEYGEN -q -a ${DEFAULT_ALGORITHM} bits
) >/dev/null 2>&1
(
  cd ns5
  $KEYGEN -q -a ${DEFAULT_ALGORITHM} -f KSK bits
) >/dev/null 2>&1
rndc_reload ns5 10.53.0.5
for i in 1 2 3 4 5 6 7 8 9 10; do
  ret=0
  $DIG $DIGOPTS @10.53.0.5 bits SOA >dig.out.ns5.test$n || ret=1
  grep "status: NOERROR" dig.out.ns5.test$n >/dev/null || ret=1
  grep "ANSWER: 2," dig.out.ns5.test$n >/dev/null || ret=1
  if [ $ret = 0 ]; then break; fi
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking rndc freeze/thaw of dynamic inline zone no change ($n)"
ret=0
$RNDCCMD 10.53.0.3 freeze dynamic >freeze.test$n 2>&1 || {
  echo_i "/' < freeze.test$n"
  ret=1
}
sleep 1
$RNDCCMD 10.53.0.3 thaw dynamic >thaw.test$n 2>&1 || {
  echo_i "rndc thaw dynamic failed"
  ret=1
}
sleep 1
grep "zone dynamic/IN (unsigned): ixfr-from-differences: unchanged" ns3/named.run >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking rndc freeze/thaw of dynamic inline zone ($n)"
ret=0
$RNDCCMD 10.53.0.3 freeze dynamic >freeze.test$n 2>&1 || ret=1
sleep 1
awk '$2 == ";" && $3 ~ /serial/ { printf("%d %s %s\n", $1 + 1, $2, $3); next; }
     { print; }
     END { print "freeze1.dynamic. 0 TXT freeze1"; } ' ns3/dynamic.db >ns3/dynamic.db.new
mv ns3/dynamic.db.new ns3/dynamic.db
$RNDCCMD 10.53.0.3 thaw dynamic >thaw.test$n 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check added record freeze1.dynamic ($n)"
for i in 1 2 3 4 5 6 7 8 9; do
  ret=0
  $DIG $DIGOPTS @10.53.0.3 freeze1.dynamic TXT >dig.out.ns3.test$n || ret=1
  grep "status: NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
  grep "ANSWER: 2," dig.out.ns3.test$n >/dev/null || ret=1
  test $ret = 0 && break
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# allow 1 second so that file time stamps change
sleep 1

n=$((n + 1))
echo_i "checking rndc freeze/thaw of server ($n)"
ret=0
$RNDCCMD 10.53.0.3 freeze >freeze.test$n 2>&1 || ret=1
sleep 1
awk '$2 == ";" && $3 ~ /serial/ { printf("%d %s %s\n", $1 + 1, $2, $3); next; }
     { print; }
     END { print "freeze2.dynamic. 0 TXT freeze2"; } ' ns3/dynamic.db >ns3/dynamic.db.new
mv ns3/dynamic.db.new ns3/dynamic.db
$RNDCCMD 10.53.0.3 thaw >thaw.test$n 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check added record freeze2.dynamic ($n)"
for i in 1 2 3 4 5 6 7 8 9; do
  ret=0
  $DIG $DIGOPTS @10.53.0.3 freeze2.dynamic TXT >dig.out.ns3.test$n || ret=1
  grep "status: NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
  grep "ANSWER: 2," dig.out.ns3.test$n >/dev/null || ret=1
  test $ret = 0 && break
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check rndc reload allows reuse of inline-signing zones ($n)"
ret=0
{ $RNDCCMD 10.53.0.3 reload 2>&1 || ret=1; } | sed 's/^/ns3 /' | cat_i
grep "not reusable" ns3/named.run >/dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check rndc sync removes both signed and unsigned journals ($n)"
ret=0
[ -f ns3/dynamic.db.jnl ] || ret=1
[ -f ns3/dynamic.db.signed.jnl ] || ret=1
$RNDCCMD 10.53.0.3 sync -clean dynamic 2>&1 || ret=1
[ -f ns3/dynamic.db.jnl ] && ret=1
[ -f ns3/dynamic.db.signed.jnl ] && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that the retransfer record is added on the hidden primary ($n)"
ret=0

$NSUPDATE <<EOF || ret=1
zone retransfer
server 10.53.0.2 ${PORT}
update add added.retransfer 0 A 1.2.3.4
send

EOF

$DIG $DIGOPTS @10.53.0.2 added.retransfer A >dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns2.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that the change has not been transferred due to notify ($n)"
ret=0
for i in 0 1 2 3 4 5 6 7 8 9; do
  ans=0
  $DIG $DIGOPTS @10.53.0.3 added.retransfer A >dig.out.ns3.test$n || ret=1
  grep "status: NOERROR" dig.out.ns3.test$n >/dev/null || ans=1
  [ $ans = 0 ] && break
  sleep 1
done
if [ $ans != 1 ]; then
  echo_i "failed"
  ret=1
fi
status=$((status + ret))

n=$((n + 1))
echo_i "check rndc retransfer of a inline secondary zone works ($n)"
ret=0
$RNDCCMD 10.53.0.3 retransfer retransfer 2>&1 || ret=1
for i in 0 1 2 3 4 5 6 7 8 9; do
  ans=0
  $DIG $DIGOPTS @10.53.0.3 added.retransfer A >dig.out.ns3.test$n || ret=1
  grep "status: NOERROR" dig.out.ns3.test$n >/dev/null || ans=1
  grep "ANSWER: 2," dig.out.ns3.test$n >/dev/null || ans=1
  [ $ans = 0 ] && break
  sleep 1
done
[ $ans = 1 ] && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# NOTE: The test below should be considered fragile.  More details can be found
# in the comment inside ns7/named.conf.
n=$((n + 1))
echo_i "check rndc retransfer of a inline nsec3 secondary does not trigger an infinite loop ($n)"
ret=0
zone=nsec3-loop
# Add secondary zone using rndc
$RNDCCMD 10.53.0.7 addzone $zone \
  '{ type secondary; primaries { 10.53.0.2; }; file "'$zone'.db"; inline-signing yes; dnssec-policy default; };' || ret=1
# Wait until secondary zone is fully signed using NSEC
for i in 1 2 3 4 5 6 7 8 9 0; do
  ret=1
  $RNDCCMD 10.53.0.7 signing -list $zone >signing.out.test$n 2>&1 || ret=1
  keys=$(grep '^Done signing' signing.out.test$n | wc -l)
  [ $keys -eq 3 ] && ret=0 && break
  sleep 1
done
# Switch secondary zone to NSEC3
$RNDCCMD 10.53.0.7 modzone $zone \
  '{ type secondary; primaries { 10.53.0.2; }; file "'$zone'.db"; inline-signing yes; dnssec-policy nsec3; };' || ret=1
# Wait until secondary zone is fully signed using NSEC3
for i in 1 2 3 4 5 6 7 8 9 0; do
  ret=1
  $DIG $DIGOPTS +nodnssec +short @10.53.0.7 nsec3param $zone >dig.out.ns7.test$n
  nsec3param=$(cat dig.out.ns7.test$n)
  test "$nsec3param" = "1 0 0 -" && ret=0 && break
  sleep 1
done

# Attempt to retransfer the secondary zone from primary
$RNDCCMD 10.53.0.7 retransfer $zone || ret=1
# Check whether the signer managed to fully sign the retransferred zone by
# waiting for a specific SOA serial number to appear in the logs; if this
# specific SOA serial number does not appear in the logs, it means the signer
# has either ran into an infinite loop or crashed; note that we check the logs
# instead of sending SOA queries to the signer as these may influence its
# behavior in a way which may prevent the desired scenario from being
# reproduced (see comment in ns7/named.conf)
for i in 1 2 3 4 5 6 7 8 9 0; do
  ret=1
  {
    grep "ns2.$zone. . 10 20 20 1814400 3600" ns7/named.run >/dev/null 2>&1
    rc=$?
  } || true
  [ $rc -eq 0 ] && ret=0 && break
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "stop bump in the wire signer server ($n)"
ret=0
stop_server ns3 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "update SOA record while stopped"
cp ns3/primary4.db.in ns3/primary.db
rm -f ns3/primary.db.jnl

n=$((n + 1))
echo_i "restart bump in the wire signer server ($n)"
ret=0
start_server --noclean --restart --port ${PORT} ns3 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "updates to SOA parameters other than serial while stopped are reflected in signed zone ($n)"
ret=0
for i in 1 2 3 4 5 6 7 8 9; do
  ans=0
  $DIG $DIGOPTS @10.53.0.3 primary SOA >dig.out.ns3.test$n || ret=1
  grep "hostmaster" dig.out.ns3.test$n >/dev/null || ans=1
  grep "ANSWER: 2," dig.out.ns3.test$n >/dev/null || ans=1
  [ $ans = 1 ] || break
  sleep 1
done
[ $ans = 0 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that reloading all zones does not cause zone maintenance to cease for inline-signed zones ($n)"
ret=1
# Ensure "rndc reload" attempts to load ns3/primary.db by waiting 1 second so
# that the file modification time has no possibility of being equal to
# the one stored during server startup.
sleep 1
nextpart ns3/named.run >/dev/null
cp ns3/primary5.db.in ns3/primary.db
rndc_reload ns3 10.53.0.3
for i in 1 2 3 4 5 6 7 8 9 10; do
  if nextpart ns3/named.run | grep "zone primary.*sending notifies" >/dev/null; then
    ret=0
    break
  fi
  sleep 1
done
# Sanity check: file updates should be reflected in the signed zone,
# i.e. SOA RNAME should no longer be set to "hostmaster".
$DIG $DIGOPTS @10.53.0.3 primary SOA >dig.out.ns3.test$n || ret=1
grep "hostmaster" dig.out.ns3.test$n >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that reloading errors prevent synchronization ($n)"
ret=1
$DIG $DIGOPTS +short @10.53.0.3 primary SOA >dig.out.ns3.test$n.1 || ret=1
sleep 1
nextpart ns3/named.run >/dev/null
cp ns3/primary6.db.in ns3/primary.db
rndc_reload ns3 10.53.0.3
for i in 1 2 3 4 5 6 7 8 9 10; do
  if nextpart ns3/named.run | grep "not loaded due to errors" >/dev/null; then
    ret=0
    break
  fi
  sleep 1
done
# Sanity check: the SOA record should be unchanged
$DIG $DIGOPTS +short @10.53.0.3 primary SOA | grep -v '^;' >dig.out.ns3.test$n.2
diff dig.out.ns3.test$n.1 dig.out.ns3.test$n.2 >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check inline-signing with an include file ($n)"
ret=0
$DIG $DIGOPTS +short @10.53.0.3 primary SOA >dig.out.ns3.test$n.1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
sleep 1
nextpart ns3/named.run >/dev/null
cp ns3/primary7.db.in ns3/primary.db
rndc_reload ns3 10.53.0.3
_includefile_loaded() {
  $DIG $DIGOPTS @10.53.0.3 f.primary A >dig.out.ns3.test$n || return 1
  grep "status: NOERROR" dig.out.ns3.test$n >/dev/null || return 1
  grep "ANSWER: 2," dig.out.ns3.test$n >/dev/null || return 1
  grep "10\.0\.0\.7" dig.out.ns3.test$n >/dev/null || return 1
  return 0
}
retry_quiet 10 _includefile_loaded
# Sanity check: the SOA record should be changed
$DIG $DIGOPTS +short @10.53.0.3 primary SOA | grep -v '^;' >dig.out.ns3.test$n.2
diff dig.out.ns3.test$n.1 dig.out.ns3.test$n.2 >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "test add/del zone combinations ($n)"
ret=0
for zone in a b c d e f g h i j k l m n o p q r s t u v w x y z; do
  $RNDCCMD 10.53.0.2 addzone test-$zone \
    '{ type primary; file "bits.db.in"; allow-transfer { any; }; };' || ret=1
  $DIG $DIGOPTS @10.53.0.2 test-$zone SOA >dig.out.ns2.$zone.test$n || ret=1
  grep "status: NOERROR," dig.out.ns2.$zone.test$n >/dev/null || {
    ret=1
    cat dig.out.ns2.$zone.test$n
  }
  $RNDCCMD 10.53.0.3 addzone test-$zone \
    '{ type secondary; primaries { 10.53.0.2; }; file "'test-$zone.bk'"; inline-signing yes; dnssec-policy default; allow-transfer { any; }; };' || ret=1
  $RNDCCMD 10.53.0.3 delzone test-$zone >/dev/null 2>&1 || ret=1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing adding external keys to a inline zone ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.3 dnskey externalkey >dig.out.ns3.test$n || ret=1
for alg in ${DEFAULT_ALGORITHM_NUMBER} ${ALTERNATIVE_ALGORITHM_NUMBER}; do
  [ $alg = 13 -a ! -f checkecdsa ] && continue

  case $alg in
    7) echo_i "checking NSEC3RSASHA1" ;;
    8) echo_i "checking RSASHA256" ;;
    13) echo_i "checking ECDSAP256SHA256" ;;
    *) echo_i "checking $alg" ;;
  esac

  dnskeys=$(grep "IN.DNSKEY.25[67] [0-9]* $alg " dig.out.ns3.test$n | wc -l)
  rrsigs=$(grep "RRSIG.DNSKEY $alg " dig.out.ns3.test$n | wc -l)
  test ${dnskeys:-0} -eq 4 || {
    echo_i "failed $alg (dnskeys ${dnskeys:-0})"
    ret=1
  }
  test ${rrsigs:-0} -eq 1 || {
    echo_i "failed $alg (rrsigs ${rrsigs:-0})"
    ret=1
  }
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing imported key won't overwrite a private key ($n)"
ret=0
key=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} import.example)
cp ${key}.key import.key
# import should fail
$IMPORTKEY -f import.key import.example >/dev/null 2>&1 && ret=1
rm -f ${key}.private
# private key removed; import should now succeed
$IMPORTKEY -f import.key import.example >/dev/null 2>&1 || ret=1
# now that it's an external key, re-import should succeed
$IMPORTKEY -f import.key import.example >/dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing updating inline secure serial via 'rndc signing -serial' ($n)"
ret=0
$DIG $DIGOPTS nsec3. SOA @10.53.0.3 >dig.out.n3.pre.test$n || ret=1
newserial=$($PERL -e 'while (<>) { chomp; my @field = split /\s+/; printf("%u\n", $field[6] + 10) if ($field[3] eq "SOA"); }' <dig.out.n3.pre.test$n)
$RNDCCMD 10.53.0.3 signing -serial ${newserial:-0} nsec3 >/dev/null 2>&1 || ret=1
retry_quiet 5 wait_for_serial 10.53.0.3 nsec3. "${newserial:-0}" dig.out.ns3.post.test$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing updating inline secure serial via 'rndc signing -serial' with negative change ($n)"
ret=0
$DIG $DIGOPTS nsec3. SOA @10.53.0.3 >dig.out.n3.pre.test$n || ret=1
oldserial=$(awk '$4 == "SOA" { print $7 }' dig.out.n3.pre.test$n)
newserial=$($PERL -e 'while (<>) { chomp; my @field = split /\s+/; printf("%u\n", $field[6] - 10) if ($field[3] eq "SOA"); }' <dig.out.n3.pre.test$n)
$RNDCCMD 10.53.0.3 signing -serial ${newserial:-0} nsec3 >/dev/null 2>&1 || ret=1
sleep 1
$DIG $DIGOPTS nsec3. SOA @10.53.0.3 >dig.out.ns3.post.test$n || ret=1
serial=$(awk '$4 == "SOA" { print $7 }' dig.out.ns3.post.test$n)
[ ${oldserial:-0} -eq ${serial:-1} ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

#
# Freezing only operates on the raw zone.
#
n=$((n + 1))
echo_i "testing updating inline secure serial via 'rndc signing -serial' when frozen ($n)"
ret=0
$DIG $DIGOPTS nsec3. SOA @10.53.0.3 >dig.out.n3.pre.test$n || ret=1
oldserial=$(awk '$4 == "SOA" { print $7 }' dig.out.n3.pre.test$n)
newserial=$($PERL -e 'while (<>) { chomp; my @field = split /\s+/; printf("%u\n", $field[6] + 10) if ($field[3] eq "SOA"); }' <dig.out.n3.pre.test$n)
$RNDCCMD 10.53.0.3 freeze nsec3 >/dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.3 signing -serial ${newserial:-0} nsec3 >/dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.3 thaw nsec3 >/dev/null 2>&1 || ret=1
retry_quiet 5 wait_for_serial 10.53.0.3 nsec3. "${newserial:-0}" dig.out.ns3.post1.test$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing updating dynamic serial via 'rndc signing -serial' ($n)"
ret=0
$DIG $DIGOPTS bits. SOA @10.53.0.2 >dig.out.ns2.pre.test$n || ret=1
newserial=$($PERL -e 'while (<>) { chomp; my @field = split /\s+/; printf("%u\n", $field[6] + 10) if ($field[3] eq "SOA"); }' <dig.out.ns2.pre.test$n)
$RNDCCMD 10.53.0.2 signing -serial ${newserial:-0} bits >/dev/null 2>&1 || ret=1
retry_quiet 5 wait_for_serial 10.53.0.2 bits. "${newserial:-0}" dig.out.ns2.post.test$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing updating dynamic serial via 'rndc signing -serial' with negative change ($n)"
ret=0
$DIG $DIGOPTS bits. SOA @10.53.0.2 >dig.out.ns2.pre.test$n || ret=1
oldserial=$(awk '$4 == "SOA" { print $7 }' dig.out.ns2.pre.test$n)
newserial=$($PERL -e 'while (<>) { chomp; my @field = split /\s+/; printf("%u\n", $field[6] - 10) if ($field[3] eq "SOA"); }' <dig.out.ns2.pre.test$n)
$RNDCCMD 10.53.0.2 signing -serial ${newserial:-0} bits >/dev/null 2>&1 || ret=1
retry_quiet 5 wait_for_serial 10.53.0.2 bits. "${newserial:-1}" dig.out.ns2.post1.test$n && ret=1
retry_quiet 5 wait_for_serial 10.53.0.2 bits. "${oldserial:-1}" dig.out.ns2.post2.test$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing updating dynamic serial via 'rndc signing -serial' when frozen ($n)"
ret=0
$DIG $DIGOPTS bits. SOA @10.53.0.2 >dig.out.ns2.pre.test$n || ret=1
oldserial=$(awk '$4 == "SOA" { print $7 }' dig.out.ns2.pre.test$n)
newserial=$($PERL -e 'while (<>) { chomp; my @field = split /\s+/; printf("%u\n", $field[6] + 10) if ($field[3] eq "SOA"); }' <dig.out.ns2.pre.test$n)
$RNDCCMD 10.53.0.2 freeze bits >/dev/null 2>&1 || ret=1
$RNDCCMD 10.53.0.2 signing -serial ${newserial:-0} bits >/dev/null 2>&1 && ret=1
$RNDCCMD 10.53.0.2 thaw bits >/dev/null 2>&1 || ret=1
retry_quiet 5 wait_for_serial 10.53.0.2 bits. "${newserial:-1}" dig.out.ns2.post1.test$n && ret=1
retry_quiet 5 wait_for_serial 10.53.0.2 bits. "${oldserial:-1}" dig.out.ns2.post2.test$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Wait until an update to the raw part of a given inline signed zone is fully
# processed.  As waiting for a fixed amount of time is suboptimal and there is
# no single message that would signify both a successful modification and an
# error in a race-free manner, instead wait until either notifies are sent
# (which means the secure zone was modified) or a receive_secure_serial() error
# is logged (which means the zone was not modified and will not be modified any
# further in response to the relevant raw zone update).
wait_until_raw_zone_update_is_processed() {
  zone="$1"
  for i in 1 2 3 4 5 6 7 8 9 10; do
    if nextpart ns3/named.run | grep -E "zone ${zone}.*(sending notifies|receive_secure_serial)" >/dev/null; then
      return
    fi
    sleep 1
  done
}

n=$((n + 1))
echo_i "checking that changes to raw zone are applied to a previously unsigned secure zone ($n)"
ret=0
# Query for bar.nokeys/A and ensure the response is negative.  As this zone
# does not have any signing keys set up, the response must be unsigned.
$DIG $DIGOPTS @10.53.0.3 bar.nokeys. A >dig.out.ns3.pre.test$n 2>&1 || ret=1
grep "status: NOERROR" dig.out.ns3.pre.test$n >/dev/null && ret=1
grep "RRSIG" dig.out.ns3.pre.test$n >/dev/null && ret=1
# Ensure the wait_until_raw_zone_update_is_processed() call below will ignore
# log messages generated before the raw zone is updated.
nextpart ns3/named.run >/dev/null
# Add a record to the raw zone on the primary.
$NSUPDATE <<EOF || ret=1
zone nokeys.
server 10.53.0.2 ${PORT}
update add bar.nokeys. 0 A 127.0.0.1
send
EOF
wait_until_raw_zone_update_is_processed "nokeys"
# Query for bar.nokeys/A again and ensure the signer now returns a positive,
# yet still unsigned response.
$DIG $DIGOPTS @10.53.0.3 bar.nokeys. A >dig.out.ns3.post.test$n 2>&1 || ret=1
grep "status: NOERROR" dig.out.ns3.post.test$n >/dev/null || ret=1
grep "RRSIG" dig.out.ns3.pre.test$n >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that changes to raw zone are not applied to a previously signed secure zone with no keys available (primary) ($n)"
ret=0
# Query for bar.removedkeys-primary/A and ensure the response is negative.  As
# this zone has signing keys set up, the response must be signed.
$DIG $DIGOPTS @10.53.0.3 bar.removedkeys-primary. A >dig.out.ns3.pre.test$n 2>&1 || ret=1
grep "status: NOERROR" dig.out.ns3.pre.test$n >/dev/null && ret=1
grep "RRSIG" dig.out.ns3.pre.test$n >/dev/null || ret=1
# Remove the signing keys for this zone.
mv -f ns3/Kremovedkeys-primary* ns3/removedkeys
# Ensure the wait_until_raw_zone_update_is_processed() call below will ignore
# log messages generated before the raw zone is updated.
nextpart ns3/named.run >/dev/null
# Add a record to the raw zone on the primary.
$NSUPDATE <<EOF || ret=1
zone removedkeys-primary.
server 10.53.0.3 ${PORT}
update add bar.removedkeys-primary. 0 A 127.0.0.1
send
EOF
wait_until_raw_zone_update_is_processed "removedkeys-primary"
# Query for bar.removedkeys-primary/A again and ensure the signer still returns
# a negative, signed response.
$DIG $DIGOPTS @10.53.0.3 bar.removedkeys-primary. A >dig.out.ns3.post.test$n 2>&1 || ret=1
grep "status: NOERROR" dig.out.ns3.post.test$n >/dev/null && ret=1
grep "RRSIG" dig.out.ns3.pre.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that backlogged changes to raw zone are applied after keys become available (primary) ($n)"
ret=0
# Restore the signing keys for this zone.
mv ns3/removedkeys/Kremovedkeys-primary* ns3
$RNDCCMD 10.53.0.3 loadkeys removedkeys-primary >/dev/null 2>&1 || ret=1
# Determine what a SOA record with a bumped serial number should look like.
BUMPED_SOA=$(sed -n 's/.*\(add removedkeys-primary.*IN.*SOA\)/\1/p;' ns3/named.run | tail -1 | awk '{$8 += 1; print $0}')
# Ensure the wait_until_raw_zone_update_is_processed() call below will ignore
# log messages generated before the raw zone is updated.
nextpart ns3/named.run >/dev/null
# Bump the SOA serial number of the raw zone.
$NSUPDATE <<EOF || ret=1
zone removedkeys-primary.
server 10.53.0.3 ${PORT}
update del removedkeys-primary. SOA
update ${BUMPED_SOA}
send
EOF
wait_until_raw_zone_update_is_processed "removedkeys-primary"
# Query for bar.removedkeys-primary/A again and ensure the signer now returns a
# positive, signed response.
$DIG $DIGOPTS @10.53.0.3 bar.removedkeys-primary. A >dig.out.ns3.test$n 2>&1 || ret=1
grep "status: NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
grep "RRSIG" dig.out.ns3.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that changes to raw zone are not applied to a previously signed secure zone with no keys available (secondary) ($n)"
ret=0
# Query for bar.removedkeys-secondary/A and ensure the response is negative.  As this
# zone does have signing keys set up, the response must be signed.
$DIG $DIGOPTS @10.53.0.3 bar.removedkeys-secondary. A >dig.out.ns3.pre.test$n 2>&1 || ret=1
grep "status: NOERROR" dig.out.ns3.pre.test$n >/dev/null && ret=1
grep "RRSIG" dig.out.ns3.pre.test$n >/dev/null || ret=1
# Remove the signing keys for this zone.
mv -f ns3/Kremovedkeys-secondary* ns3/removedkeys
# Ensure the wait_until_raw_zone_update_is_processed() call below will ignore
# log messages generated before the raw zone is updated.
nextpart ns3/named.run >/dev/null
# Add a record to the raw zone on the primary.
$NSUPDATE <<EOF || ret=1
zone removedkeys-secondary.
server 10.53.0.2 ${PORT}
update add bar.removedkeys-secondary. 0 A 127.0.0.1
send
EOF
wait_until_raw_zone_update_is_processed "removedkeys-secondary"
# Query for bar.removedkeys-secondary/A again and ensure the signer still returns a
# negative, signed response.
$DIG $DIGOPTS @10.53.0.3 bar.removedkeys-secondary. A >dig.out.ns3.post.test$n 2>&1 || ret=1
grep "status: NOERROR" dig.out.ns3.post.test$n >/dev/null && ret=1
grep "RRSIG" dig.out.ns3.pre.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that backlogged changes to raw zone are applied after keys become available (secondary) ($n)"
ret=0
# Restore the signing keys for this zone.
mv ns3/removedkeys/Kremovedkeys-secondary* ns3
$RNDCCMD 10.53.0.3 loadkeys removedkeys-secondary >/dev/null 2>&1 || ret=1
# Determine what a SOA record with a bumped serial number should look like.
BUMPED_SOA=$(sed -n 's/.*\(add removedkeys-secondary.*IN.*SOA\)/\1/p;' ns2/named.run | tail -1 | awk '{$8 += 1; print $0}')
# Ensure the wait_until_raw_zone_update_is_processed() call below will ignore
# log messages generated before the raw zone is updated.
nextpart ns3/named.run >/dev/null
# Bump the SOA serial number of the raw zone on the primary.
$NSUPDATE <<EOF || ret=1
zone removedkeys-secondary.
server 10.53.0.2 ${PORT}
update del removedkeys-secondary. SOA
update ${BUMPED_SOA}
send
EOF
wait_until_raw_zone_update_is_processed "removedkeys-secondary"
# Query for bar.removedkeys-secondary/A again and ensure the signer now returns
# a positive, signed response.
$DIG $DIGOPTS @10.53.0.3 bar.removedkeys-secondary. A >dig.out.ns3.test$n 2>&1 || ret=1
grep "status: NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
grep "RRSIG" dig.out.ns3.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Check that the file $2 for zone $1 does not contain RRSIG records
# while the journal file for that zone does contain them.
ensure_sigs_only_in_journal() {
  origin="$1"
  masterfile="$2"
  $CHECKZONE -i none -f raw -D -o - "$origin" "$masterfile" 2>&1 | grep -w RRSIG >/dev/null && ret=1
  $CHECKZONE -j -i none -f raw -D -o - "$origin" "$masterfile" 2>&1 | grep -w RRSIG >/dev/null || ret=1
}

n=$((n + 1))
echo_i "checking that records added from a journal are scheduled to be resigned ($n)"
ret=0
zone="delayedkeys"
# Signing keys for the "delayedkeys" zone are not yet accessible.  Thus, the
# zone file for the signed version of the zone will contain no DNSSEC records.
# Move keys into place now and load them, which will cause DNSSEC records to
# only be present in the journal for the signed version of the zone.
mv Kdelayedkeys* ns3/
cp ns3/delayedkeys.conf.2 ns3/delayedkeys.conf
$RNDCCMD 10.53.0.3 reconfig >/dev/null 2>&1 || ret=1

#$RNDCCMD 10.53.0.3 loadkeys delayedkeys > rndc.out.ns3.pre.test$n 2>&1 || ret=1
# Wait until the zone is signed.
check_done_signing() (
  $RNDCCMD 10.53.0.3 signing -list delayedkeys >signing.out.test$n 2>&1 || true
  num=$(grep "Done signing with" signing.out.test$n | wc -l)
  [ $num -eq 2 ]
)
retry_quiet 10 check_done_signing || ret=1
# Halt rather than stopping the server to prevent the file from being
# flushed upon shutdown since we specifically want to avoid it.
stop_server --use-rndc --halt --port ${CONTROLPORT} ns3 || ret=1
ensure_sigs_only_in_journal delayedkeys ns3/delayedkeys.db.signed
start_server --noclean --restart --port ${PORT} ns3 || ret=1
# At this point, the raw zone journal will not have a source serial set.  Upon
# server startup, receive_secure_serial() will rectify that, update SOA, resign
# it, and schedule its future resign.  This will cause "rndc zonestatus" to
# return delayedkeys/SOA as the next node to resign, so we restart the server
# once again; with the raw zone journal now having a source serial set,
# receive_secure_serial() should refrain from introducing any zone changes.
stop_server --use-rndc --halt --port ${CONTROLPORT} ns3 || ret=1
ensure_sigs_only_in_journal delayedkeys ns3/delayedkeys.db.signed
nextpart ns3/named.run >/dev/null
start_server --noclean --restart --port ${PORT} ns3 || ret=1
# We can now test whether the secure zone journal was correctly processed:
# unless the records contained in it were scheduled for resigning, no resigning
# event will be scheduled at all since the secure zone file contains no
# DNSSEC records.
wait_for_log 20 "all zones loaded" ns3/named.run || ret=1
$RNDCCMD 10.53.0.3 zonestatus delayedkeys >rndc.out.ns3.post.test$n 2>&1 || ret=1
grep "next resign node:" rndc.out.ns3.post.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that zonestatus reports 'type: primary' for an inline primary zone ($n)"
ret=0
$RNDCCMD 10.53.0.3 zonestatus primary >rndc.out.ns3.test$n || ret=1
grep "type: primary" rndc.out.ns3.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that zonestatus reports 'type: secondary' for an inline secondary zone ($n)"
ret=0
$RNDCCMD 10.53.0.3 zonestatus bits >rndc.out.ns3.test$n || ret=1
grep "type: secondary" rndc.out.ns3.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking reload of touched inline zones ($n)"
ret=0
echo_ic "pre-reload 'next key event'"
nextpart ns8/named.run >nextpart.pre$n.out
count=$(grep "zone example[0-9][0-9].com/IN (signed): next key event:" nextpart.pre$n.out | wc -l)
echo_ic "found: $count/16"
[ $count -eq 16 ] || ret=1
echo_ic "touch and reload"
touch ns8/example??.com.db
$RNDCCMD 10.53.0.8 reload 2>&1 | sed 's/^/ns3 /' | cat_i
sleep 5
echo_ic "post-reload 'next key event'"
nextpart ns8/named.run >nextpart.post$n.out
count=$(grep "zone example[0-9][0-9].com/IN (signed): next key event:" nextpart.post$n.out | wc -l)
echo_ic "found: $count/16"
[ $count -eq 16 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking second reload of touched inline zones ($n)"
ret=0
nextpart ns8/named.run >nextpart.pre$n.out
$RNDCCMD 10.53.0.8 reload 2>&1 | sed 's/^/ns3 /' | cat_i
sleep 5
nextpart ns8/named.run >nextpart.post$n.out
grep "ixfr-from-differences: unchanged" nextpart.post$n.out && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "Check that 'rndc reload' of just the serial updates the signed instance ($n)"
ret=0
dig_with_opts @10.53.0.8 example SOA >dig.out.ns8.test$n.soa1 || ret=1
cp ns8/example2.db.in ns8/example.db || ret=1
nextpart ns8/named.run >/dev/null
rndccmd 10.53.0.8 reload || ret=1
wait_for_log 3 "all zones loaded" ns8/named.run
sleep 1
dig_with_opts @10.53.0.8 example SOA >dig.out.ns8.test$n.soa2 || ret=1
soa1=$(awk '$4 == "SOA" { print $7 }' dig.out.ns8.test$n.soa1)
soa2=$(awk '$4 == "SOA" { print $7 }' dig.out.ns8.test$n.soa2)
ttl1=$(awk '$4 == "SOA" { print $2 }' dig.out.ns8.test$n.soa1)
ttl2=$(awk '$4 == "SOA" { print $2 }' dig.out.ns8.test$n.soa2)
test ${soa1:-1000} -lt ${soa2:-0} || ret=1
test ${ttl1:-0} -eq 300 || ret=1
test ${ttl2:-0} -eq 300 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "Check that restart with zone changes and deleted journal works ($n)"
TSIG=
ret=0
dig_with_opts @10.53.0.8 example SOA >dig.out.ns8.test$n.soa1 || ret=1
stop_server --use-rndc --port ${CONTROLPORT} ns8 || ret=1
# TTL of all records change from 300 to 400
cp ns8/example3.db.in ns8/example.db || ret=1
rm -f ns8/example.db.jnl
nextpart ns8/named.run >/dev/null
start_server --noclean --restart --port ${PORT} ns8 || ret=1
wait_for_log 3 "all zones loaded" ns8/named.run
sleep 1
dig_with_opts @10.53.0.8 example SOA >dig.out.ns8.test$n.soa2 || ret=1
soa1=$(awk '$4 == "SOA" { print $7 }' dig.out.ns8.test$n.soa1)
soa2=$(awk '$4 == "SOA" { print $7 }' dig.out.ns8.test$n.soa2)
ttl1=$(awk '$4 == "SOA" { print $2 }' dig.out.ns8.test$n.soa1)
ttl2=$(awk '$4 == "SOA" { print $2 }' dig.out.ns8.test$n.soa2)
test ${soa1:-1000} -lt ${soa2:-0} || ret=1
test ${ttl1:-0} -eq 300 || ret=1
test ${ttl2:-0} -eq 400 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
