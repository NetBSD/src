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

set -e

. ../conf.sh

DIGOPTS="+tcp +noadd +nosea +nostat +noquest +nocomm +nocmd -p ${PORT}"
RNDCCMD="$RNDC -p ${CONTROLPORT} -c ../_common/rndc.conf"

nextpart_thrice() {
  nextpart ns1/named.run >/dev/null
  nextpart ns2/named.run >/dev/null
  nextpart ns3/named.run >/dev/null
}

wait_for_log_thrice() {
  echo_i "waiting for servers to incorporate changes"
  wait_for_log 10 "committing update transaction" ns1/named.run
  wait_for_log 10 "zone transfer finished" ns2/named.run
  wait_for_log 10 "zone transfer finished" ns3/named.run
}

status=0
n=1
capture_dnstap() {
  retry_quiet 20 test -f ns3/dnstap.out && mv ns3/dnstap.out dnstap.out.$n
  $RNDCCMD -s 10.53.0.3 dnstap -reopen
}

uq_equals_ur() {
  zonename="$1"
  "$DNSTAPREAD" dnstap.out.$n \
    | awk '$9 ~ /^'$zonename'\// { print }' \
    | awk '$3 == "UQ" { UQ+=1 } $3 == "UR" { UR += 1 } END { print UQ+0, UR+0 }' >dnstapread.out$n
  read UQ UR <dnstapread.out$n
  echo_i "UQ=$UQ UR=$UR"
  test $UQ -eq $UR || return 1
}

echo_i "waiting for servers to be ready for testing ($n)"
for i in 1 2 3 4 5 6 7 8 9 10; do
  ret=0
  $DIG +tcp -p ${PORT} example. @10.53.0.1 soa >dig.out.ns1.$n || ret=1
  grep "status: NOERROR" dig.out.ns1.$n >/dev/null || ret=1
  $DIG +tcp -p ${PORT} example. @10.53.0.2 soa >dig.out.ns2.$n || ret=1
  grep "status: NOERROR" dig.out.ns2.$n >/dev/null || ret=1
  $DIG +tcp -p ${PORT} example. @10.53.0.3 soa >dig.out.ns3.$n || ret=1
  grep "status: NOERROR" dig.out.ns3.$n >/dev/null || ret=1
  test $ret = 0 && break
  sleep 1
done
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))

echo_i "fetching primary copy of zone before update ($n)"
ret=0
$DIG $DIGOPTS example. @10.53.0.1 axfr >dig.out.ns1.example.before || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))

echo_i "fetching secondary 1 copy of zone before update ($n)"
$DIG $DIGOPTS example. @10.53.0.2 axfr >dig.out.ns2.example.before || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))

echo_i "fetching secondary 2 copy of zone before update ($n)"
ret=0
$DIG $DIGOPTS example. @10.53.0.3 axfr >dig.out.ns3.example.before || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))

echo_i "comparing pre-update copies to known good data ($n)"
ret=0
digcomp knowngood.before dig.out.ns1.example.before || ret=1
digcomp knowngood.before dig.out.ns2.example.before || ret=1
digcomp knowngood.before dig.out.ns3.example.before || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi

echo_i "checking update forwarding of a zone (signed) (Do53 -> DoT) ($n)"
nextpart_thrice
ret=0
$NSUPDATE -y "${DEFAULT_HMAC}:update.example:c3Ryb25nIGVub3VnaCBmb3IgYSBtYW4gYnV0IG1hZGUgZm9yIGEgd29tYW4K" -- - <<EOF || ret=1
local 10.53.0.1
server 10.53.0.3 ${PORT}
update add updated.example. 600 A 10.10.10.1
update add updated.example. 600 TXT Foo
send
EOF
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))
wait_for_log_thrice

echo_i "fetching primary copy of zone after update ($n)"
ret=0
$DIG $DIGOPTS example. @10.53.0.1 axfr >dig.out.ns1.example.after1 || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))

echo_i "fetching secondary 1 copy of zone after update ($n)"
ret=0
$DIG $DIGOPTS example. @10.53.0.2 axfr >dig.out.ns2.example.after1 || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi

echo_i "fetching secondary 2 copy of zone after update ($n)"
ret=0
$DIG $DIGOPTS example. @10.53.0.3 axfr >dig.out.ns3.example.after1 || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))

echo_i "comparing post-update copies to known good data ($n)"
ret=0
digcomp knowngood.after1 dig.out.ns1.example.after1 || ret=1
digcomp knowngood.after1 dig.out.ns2.example.after1 || ret=1
digcomp knowngood.after1 dig.out.ns3.example.after1 || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi

echo_i "checking update forwarding of a zone (signed) (DoT -> DoT) ($n)"
nextpart_thrice
ret=0
$NSUPDATE -y "${DEFAULT_HMAC}:update.example:c3Ryb25nIGVub3VnaCBmb3IgYSBtYW4gYnV0IG1hZGUgZm9yIGEgd29tYW4K" -S -O -- - <<EOF || ret=1
local 10.53.0.1
server 10.53.0.3 ${TLSPORT}
update add updated-dot.example. 600 A 10.10.10.1
update add updated-dot.example. 600 TXT Foo
send
EOF
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))
wait_for_log_thrice

echo_i "fetching primary copy of zone after update ($n)"
ret=0
$DIG $DIGOPTS example. @10.53.0.1 axfr >dig.out.ns1.example.after2 || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))

echo_i "fetching secondary 1 copy of zone after update ($n)"
ret=0
$DIG $DIGOPTS example. @10.53.0.2 axfr >dig.out.ns2.example.after2 || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi

echo_i "fetching secondary 2 copy of zone after update ($n)"
ret=0
$DIG $DIGOPTS example. @10.53.0.3 axfr >dig.out.ns3.example.after2 || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))

echo_i "comparing post-update copies to known good data ($n)"
ret=0
digcomp knowngood.after2 dig.out.ns1.example.after2 || ret=1
digcomp knowngood.after2 dig.out.ns2.example.after2 || ret=1
digcomp knowngood.after2 dig.out.ns3.example.after2 || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi

echo_i "checking 'forwarding update for zone' is logged twice ($n)"
ret=0
cnt=$(grep -F "forwarding update for zone 'example/IN'" ns3/named.run | wc -l || ret=1)
test "${cnt}" -eq 2 || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))

if $FEATURETEST --enable-dnstap; then
  echo_i "checking DNSTAP logging of UPDATE forwarded update replies ($n)"
  ret=0
  capture_dnstap
  uq_equals_ur example || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
  n=$((n + 1))
fi

echo_i "updating zone (unsigned) ($n)"
nextpart_thrice
ret=0
$NSUPDATE -- - <<EOF || ret=1
local 10.53.0.1
server 10.53.0.3 ${PORT}
update add unsigned.example. 600 A 10.10.10.1
update add unsigned.example. 600 TXT Foo
send
EOF
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))
wait_for_log_thrice

echo_i "fetching primary copy of zone after update ($n)"
ret=0
$DIG $DIGOPTS example. @10.53.0.1 axfr >dig.out.ns1.example.after3 || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi

echo_i "fetching secondary 1 copy of zone after update ($n)"
ret=0
$DIG $DIGOPTS example. @10.53.0.2 axfr >dig.out.ns2.example.after3 || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))

echo_i "fetching secondary 2 copy of zone after update ($n)"
ret=0
$DIG $DIGOPTS example. @10.53.0.3 axfr >dig.out.ns3.example.after3 || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi

echo_i "comparing post-update copies to known good data ($n)"
ret=0
digcomp knowngood.after3 dig.out.ns1.example.after3 || ret=1
digcomp knowngood.after3 dig.out.ns2.example.after3 || ret=1
digcomp knowngood.after3 dig.out.ns3.example.after3 || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi

echo_i "fetching primary copy of zone before update, first primary fails ($n)"
ret=0
$DIG $DIGOPTS example3. @10.53.0.1 axfr >dig.out.ns1.example3.before || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))

echo_i "fetching secondary 1 copy of zone before update, first primary fails ($n)"
$DIG $DIGOPTS example3. @10.53.0.2 axfr >dig.out.ns2.example3.before || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))

echo_i "fetching secondary 2 copy of zone before update, first primary fails ($n)"
ret=0
$DIG $DIGOPTS example3. @10.53.0.3 axfr >dig.out.ns3.example3.before || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))

echo_i "comparing pre-update copies to known good data, first primary fails ($n)"
ret=0
digcomp knowngood.before.example3 dig.out.ns1.example3.before || ret=1
digcomp knowngood.before.example3 dig.out.ns2.example3.before || ret=1
digcomp knowngood.before.example3 dig.out.ns3.example3.before || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi

echo_i "checking update forwarding of a zone (signed) (Do53 -> DoT) ($n)"
nextpart_thrice
ret=0
$NSUPDATE -y "${DEFAULT_HMAC}:update.example:c3Ryb25nIGVub3VnaCBmb3IgYSBtYW4gYnV0IG1hZGUgZm9yIGEgd29tYW4K" -- - <<EOF || ret=1
local 10.53.0.1
server 10.53.0.3 ${PORT}
update add updated.example3. 600 A 10.10.10.1
update add updated.example3. 600 TXT Foo
send
EOF
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))
wait_for_log_thrice

echo_i "fetching primary copy of zone after update, first primary fails ($n)"
ret=0
$DIG $DIGOPTS example3. @10.53.0.1 axfr >dig.out.ns1.example3.after1 || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))

echo_i "fetching secondary 1 copy of zone after update, first primary fails ($n)"
ret=0
$DIG $DIGOPTS example3. @10.53.0.2 axfr >dig.out.ns2.example3.after1 || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi

echo_i "fetching secondary 2 copy of zone after update, first primary fails ($n)"
ret=0
$DIG $DIGOPTS example3. @10.53.0.3 axfr >dig.out.ns3.example3.after1 || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))

echo_i "comparing post-update copies to known good data, first primary fails ($n)"
ret=0
digcomp knowngood.after1.example3 dig.out.ns1.example3.after1 || ret=1
digcomp knowngood.after1.example3 dig.out.ns2.example3.after1 || ret=1
digcomp knowngood.after1.example3 dig.out.ns3.example3.after1 || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi

if $FEATURETEST --enable-dnstap; then
  echo_i "checking DNSTAP logging of UPDATE forwarded update replies ($n)"
  ret=0
  capture_dnstap
  uq_equals_ur example3 || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
  n=$((n + 1))
fi
n=$((n + 1))

if test -f keyname; then
  echo_i "checking update forwarding with sig0 (Do53 -> Do53) ($n)"
  nextpart_thrice
  ret=0
  keyname=$(cat keyname)
  $NSUPDATE -k $keyname.private -- - <<EOF >nsupdate.out.test$n 2>&1 || ret=1
	local 10.53.0.1
	server 10.53.0.3 ${PORT}
	zone example2
	update add unsigned.example2. 600 A 10.10.10.1
	update add unsigned.example2. 600 TXT Foo
	send
EOF
  if [ $ret != 0 ]; then
    echo_i "failed"
    status=$((status + ret))
  fi
  n=$((n + 1))
  wait_for_log_thrice

  $DIG -p ${PORT} unsigned.example2 A @10.53.0.1 >dig.out.ns1.test$n || ret=1
  grep "status: NOERROR" dig.out.ns1.test$n >/dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
  n=$((n + 1))

  if $FEATURETEST --enable-dnstap; then
    echo_i "checking DNSTAP logging of UPDATE forwarded update replies ($n)"
    ret=0
    capture_dnstap
    uq_equals_ur example2 || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
    n=$((n + 1))
  fi

  echo_i "checking update forwarding with sig0 (DoT -> Do53) ($n)"
  nextpart_thrice
  ret=0
  keyname=$(cat keyname)
  $NSUPDATE -k $keyname.private -S -O -- - <<EOF >nsupdate.out.test$n 2>&1 || ret=1
        local 10.53.0.1
	server 10.53.0.3 ${TLSPORT}
	zone example2
	update add unsigned-dot.example2. 600 A 10.10.10.1
	update add unsigned-dot.example2. 600 TXT Foo
	send
EOF
  if [ $ret != 0 ]; then
    echo_i "failed"
    status=$((status + ret))
  fi
  n=$((n + 1))
  wait_for_log_thrice

  $DIG -p ${PORT} unsigned-dot.example2 A @10.53.0.1 >dig.out.ns1.test$n || ret=1
  grep "status: NOERROR" dig.out.ns1.test$n >/dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
  n=$((n + 1))

  if $FEATURETEST --enable-dnstap; then
    echo_i "checking DNSTAP logging of UPDATE forwarded update replies ($n)"
    ret=0
    capture_dnstap
    uq_equals_ur example2 || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
    n=$((n + 1))
  fi

  echo_i "checking update forwarding with sig0 with too many keys ($n)"
  nextpart_thrice
  ret=0
  good=0
  bad=0
  for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17; do
    keyname=$(cat keyname$i)
    $NSUPDATE -d -D -k $keyname.private -- - <<EOF >nsupdate.out.test$n.$i 2>&1 && good=$((good + 1)) || bad=$((bad + 1))
	local 10.53.0.1
	server 10.53.0.3 ${PORT}
	zone example2-toomanykeys
	update add toomanykeys$i.example2-toomanykeys. 600 A 10.10.10.1
	send
EOF
  done
  # There are 17 keys in the zone but by default named checks maximum 16 keys
  # to find a matching key, so one of these updates should have been failed.
  [ $good = 16 ] && [ $bad = 1 ] || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
  n=$((n + 1))
fi

echo_i "attempting an update that should be rejected by ACL ($n)"
ret=0
{
  $NSUPDATE -- - <<EOF
        local 10.53.0.2
        server 10.53.0.3 ${PORT}
        update add another.unsigned.example. 600 A 10.10.10.2
        update add another.unsigned.example. 600 TXT Bar
        send
EOF
} >nsupdate.out.$n 2>&1 && ret=1
grep REFUSED nsupdate.out.$n >/dev/null || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))

echo_i "checking update forwarding to dead primary ($n)"
count=0
ret=0
while [ $count -lt 5 -a $ret -eq 0 ]; do
  (
    $NSUPDATE -- - <<EOF
local 10.53.0.1
server 10.53.0.3 ${PORT}
zone noprimary
update add unsigned.noprimary. 600 A 10.10.10.1
update add unsigned.noprimary. 600 TXT Foo
send
EOF
  ) >/dev/null 2>&1 &
  $DIG -p ${PORT} +noadd +notcp +noauth noprimary. @10.53.0.3 soa >dig.out.ns3.test$n.$count || ret=1
  grep "status: NOERROR" dig.out.ns3.test$n.$count >/dev/null || ret=1
  count=$((count + 1))
done
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
fi
n=$((n + 1))

echo_i "waiting for nsupdate to finish ($n)"
wait
n=$((n + 1))

if $FEATURETEST --enable-dnstap; then
  echo_i "checking DNSTAP logging of UPDATE forwarded update replies ($n)"
  ret=0
  capture_dnstap
  uq_equals_ur noprimary && ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
  n=$((n + 1))
fi

n=$((n + 1))
ret=0
echo_i "attempting updates that should exceed quota ($n)"
# lower the update quota to 1.
cp ns3/named2.conf ns3/named.conf
rndc_reconfig ns3 10.53.0.3
nextpart ns3/named.run >/dev/null
for loop in 1 2 3 4 5 6 7 8 9 10; do
  {
    $NSUPDATE -- - >/dev/null 2>&1 <<END
  local 10.53.0.1
  server 10.53.0.3 ${PORT}
  update add txt-$loop.unsigned.example 300 IN TXT Whatever
  send
END
  } &
done
wait_for_log 10 "too many DNS UPDATEs queued" ns3/named.run || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
