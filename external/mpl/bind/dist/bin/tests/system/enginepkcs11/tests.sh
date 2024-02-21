#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

set -e

# shellcheck source=conf.sh
. ../conf.sh

PWD=$(pwd)

status=0
ret=0
n=0

dig_with_opts() (
  $DIG +tcp +noadd +nosea +nostat +nocmd +dnssec -p "$PORT" "$@"
)

# Perform tests inside ns1 dir
cd ns1

for algtypebits in rsasha256:rsa:2048 rsasha512:rsa:2048 \
  ecdsap256sha256:EC:prime256v1 ecdsap384sha384:EC:prime384v1; do # Edwards curves are not yet supported by OpenSC
  # ed25519:EC:edwards25519 ed448:EC:edwards448
  alg=$(echo "$algtypebits" | cut -f 1 -d :)
  type=$(echo "$algtypebits" | cut -f 2 -d :)
  bits=$(echo "$algtypebits" | cut -f 3 -d :)
  zone="${alg}.example"
  zonefile="zone.${zone}.db.signed"

  if [ ! -f $zonefile ]; then
    echo_i "skipping test for ${alg}:${type}:${bits}, no signed zone file ${zonefile}"
    continue
  fi

  # Basic checks if setup was successful.
  n=$((n + 1))
  ret=0
  echo_i "Test key generation was successful for $zone ($n)"
  count=$(ls K*.key | grep "K${zone}" | wc -l)
  test "$count" -eq 4 || ret=1
  test "$ret" -eq 0 || echo_i "failed (expected 4 keys, got $count)"
  status=$((status + ret))

  n=$((n + 1))
  ret=0
  echo_i "Test zone signing was successful for $zone ($n)"
  $VERIFY -z -o $zone "${zonefile}" >verify.out.$zone.$n 2>&1 || ret=1
  test "$ret" -eq 0 || echo_i "failed (dnssec-verify failed)"
  status=$((status + ret))

  # Test inline signing with keys stored in engine.
  zskid1=$(cat "${zone}.zskid1")
  zskid2=$(cat "${zone}.zskid2")

  n=$((n + 1))
  ret=0
  echo_i "Test inline signing for $zone ($n)"
  dig_with_opts "$zone" @10.53.0.1 SOA >dig.out.soa.$zone.$n || ret=1
  awk '$4 == "RRSIG" { print $11 }' dig.out.soa.$zone.$n >dig.out.keyids.$zone.$n || return 1
  numsigs=$(cat dig.out.keyids.$zone.$n | wc -l)
  test $numsigs -eq 1 || return 1
  grep -w "$zskid1" dig.out.keyids.$zone.$n >/dev/null || return 1
  test "$ret" -eq 0 || echo_i "failed (SOA RRset not signed with key $zskid1)"
  status=$((status + ret))

  n=$((n + 1))
  ret=0
  echo_i "Dynamically update $zone, add new zsk ($n)"
  zsk2=$(grep -v ';' K${zone}.*.zsk2)
  cat >"update.cmd.zsk.$zone.$n" <<EOF
server 10.53.0.1 $PORT
ttl 300
zone $zone
update add $zsk2
send
EOF

  $NSUPDATE -v >"update.log.zsk.$zone.$n" <"update.cmd.zsk.$zone.$n" || ret=1
  test "$ret" -eq 0 || echo_i "failed (update failed)"
  status=$((status + ret))

  n=$((n + 1))
  ret=0
  echo_i "Test DNSKEY response for $zone after inline signing ($n)"
  _dig_dnskey() (
    dig_with_opts "$zone" @10.53.0.1 DNSKEY >dig.out.dnskey.$zone.$n || return 1
    count=$(awk 'BEGIN { count = 0 } $4 == "DNSKEY" { count++ } END {print count}' dig.out.dnskey.$zone.$n)
    test $count -eq 3
  )
  retry_quiet 10 _dig_dnskey || ret=1
  test "$ret" -eq 0 || echo_i "failed (expected 3 DNSKEY records)"
  status=$((status + ret))

  n=$((n + 1))
  ret=0
  echo_i "Test SOA response for $zone after inline signing ($n)"
  _dig_soa() (
    dig_with_opts "$zone" @10.53.0.1 SOA >dig.out.soa.$zone.$n || return 1
    awk '$4 == "RRSIG" { print $11 }' dig.out.soa.$zone.$n >dig.out.keyids.$zone.$n || return 1
    numsigs=$(cat dig.out.keyids.$zone.$n | wc -l)
    test $numsigs -eq 2 || return 1
    grep -w "$zskid1" dig.out.keyids.$zone.$n >/dev/null || return 1
    grep -w "$zskid2" dig.out.keyids.$zone.$n >/dev/null || return 1
    return 0
  )
  retry_quiet 10 _dig_soa || ret=1
  test "$ret" -eq 0 || echo_i "failed (expected 2 SOA RRSIG records)"
  status=$((status + ret))

  # Test inline signing with keys stored in engine (key signing).
  kskid1=$(cat "${zone}.kskid1")
  kskid2=$(cat "${zone}.kskid2")

  n=$((n + 1))
  ret=0
  echo_i "Dynamically update $zone, add new ksk ($n)"
  ksk2=$(grep -v ';' K${zone}.*.ksk2)
  cat >"update.cmd.ksk.$zone.$n" <<EOF
server 10.53.0.1 $PORT
ttl 300
zone $zone
update add $ksk2
send
EOF

  $NSUPDATE -v >"update.log.ksk.$zone.$n" <"update.cmd.ksk.$zone.$n" || ret=1
  test "$ret" -eq 0 || echo_i "failed (update failed)"
  status=$((status + ret))

  n=$((n + 1))
  ret=0
  echo_i "Test DNSKEY response for $zone after inline signing (key signing) ($n)"
  _dig_dnskey_ksk() (
    dig_with_opts "$zone" @10.53.0.1 DNSKEY >dig.out.dnskey.$zone.$n || return 1
    count=$(awk 'BEGIN { count = 0 } $4 == "DNSKEY" { count++ } END {print count}' dig.out.dnskey.$zone.$n)
    test $count -eq 4 || return 1
    awk '$4 == "RRSIG" { print $11 }' dig.out.dnskey.$zone.$n >dig.out.keyids.$zone.$n || return 1
    numsigs=$(cat dig.out.keyids.$zone.$n | wc -l)
    test $numsigs -eq 2 || return 1
    grep -w "$kskid1" dig.out.keyids.$zone.$n >/dev/null || return 1
    grep -w "$kskid2" dig.out.keyids.$zone.$n >/dev/null || return 1
    return 0
  )
  retry_quiet 10 _dig_dnskey_ksk || ret=1
  test "$ret" -eq 0 || echo_i "failed (expected 4 DNSKEY records, 2 KSK signatures)"
  status=$((status + ret))

done

# Go back to main test dir.
cd ..

n=$((n + 1))
ret=0
echo_i "Checking for assertion failure in pk11_numbits()"
$PERL ../packet.pl -a "10.53.0.1" -p "$PORT" -t udp 2037-pk11_numbits-crash-test.pkt
dig_with_opts @10.53.0.1 version.bind. CH TXT >dig.out.pk11_numbits || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
