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

# shellcheck source=conf.sh
. ../conf.sh
# shellcheck source=kasp.sh
. ../kasp.sh

dig_with_opts() {
  $DIG +tcp +noadd +nosea +nostat +nocmd +dnssec -p $PORT "$@"
}

start_time="$(TZ=UTC date +%s)"
status=0
n=0

set_zone "model2.multisigner"
set_policy "model2" "2" "3600"

# Key properties and states.
key_clear "KEY1"
set_keyrole "KEY1" "ksk"
set_keylifetime "KEY1" "0"
set_keyalgorithm "KEY1" "13" "ECDSAP256SHA256" "256"
set_keysigning "KEY1" "yes"
set_zonesigning "KEY1" "no"
set_keystate "KEY1" "GOAL" "omnipresent"
set_keystate "KEY1" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY1" "STATE_KRRSIG" "omnipresent"
set_keystate "KEY1" "STATE_DS" "omnipresent"

key_clear "KEY2"
set_keyrole "KEY2" "zsk"
set_keylifetime "KEY2" "0"
set_keyalgorithm "KEY2" "13" "ECDSAP256SHA256" "256"
set_keysigning "KEY2" "no"
set_zonesigning "KEY2" "yes"
set_keystate "KEY2" "GOAL" "omnipresent"
set_keystate "KEY2" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY2" "STATE_ZRRSIG" "omnipresent"

key_clear "KEY3"
key_clear "KEY4"

set_keytimes_model2() {
  # The first KSK is immediately published and activated.
  created=$(key_get KEY1 CREATED)
  set_keytime "KEY1" "PUBLISHED" "${created}"
  set_keytime "KEY1" "ACTIVE" "${created}"
  set_keytime "KEY1" "SYNCPUBLISH" "${created}"

  # The first ZSKs are immediately published and activated.
  created=$(key_get KEY2 CREATED)
  set_keytime "KEY2" "PUBLISHED" "${created}"
  set_keytime "KEY2" "ACTIVE" "${created}"
}

set_server "ns3" "10.53.0.3"
check_keys
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE"
set_keytimes_model2
check_keytimes
check_apex
dnssec_verify

set_server "ns4" "10.53.0.4"
check_keys
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE"
set_keytimes_model2
check_keytimes
check_apex
dnssec_verify

#
# Update DNSKEY RRset.
#

# Check that the ZSKs from the other provider are published.
zsks_are_published() {
  dig_with_opts "$ZONE" "@${SERVER}" DNSKEY >"dig.out.$DIR.test$n" || return 1
  cat dig.out.$DIR.test$n | tr [:blank:] ' ' >dig.out.$DIR.test$n.tr || return 1
  # We should have two ZSKs.
  lines=$(grep "256 3 13" dig.out.$DIR.test$n.tr | wc -l)
  test "$lines" -eq 2 || return 1
  # Both ZSKs are published.
  grep "$(cat ns3/${ZONE}.zsk | tr [:blank:] ' ')" dig.out.$DIR.test$n.tr >/dev/null || return 1
  grep "$(cat ns4/${ZONE}.zsk | tr [:blank:] ' ')" dig.out.$DIR.test$n.tr >/dev/null || return 1
  # And one KSK.
  lines=$(grep "257 3 13" dig.out.$DIR.test$n.tr | wc -l)
  test "$lines" -eq 1 || return 1
}

# Test to make sure no DNSSEC records end up in the raw journal.
no_dnssec_in_journal() {
  n=$((n + 1))
  ret=0
  echo_i "check zone ${ZONE} raw journal has no DNSSEC ($n)"
  $JOURNALPRINT "${DIR}/${ZONE}.db.jnl" >"${DIR}/${ZONE}.journal.out.test$n"
  rrset_exists NSEC "${DIR}/${ZONE}.journal.out.test$n" && ret=1
  rrset_exists NSEC3 "${DIR}/${ZONE}.journal.out.test$n" && ret=1
  rrset_exists NSEC3PARAM "${DIR}/${ZONE}.journal.out.test$n" && ret=1
  rrset_exists RRSIG "${DIR}/${ZONE}.journal.out.test$n" && ret= 1
  test "$ret" -eq 0 || echo_i "failed"
  status=$((status + ret))
}

# Check if a certain RRtype is present in the journal file.
rrset_exists() (
  rrtype=$1
  file=$2
  lines=$(awk -v rt="${rrtype}" '$5 == rt {print}' ${file} | wc -l)
  test "$lines" -gt 0
)

n=$((n + 1))
echo_i "add dnskey record: update zone ${ZONE} at ns3 with ZSK from provider ns4 ($n)"
ret=0
set_server "ns3" "10.53.0.3"
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update add $(cat "ns4/${ZONE}.zsk")
  echo send
) | $NSUPDATE || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Check the new DNSKEY RRset.
n=$((n + 1))
echo_i "check zone ${ZONE} DNSKEY RRset after update ($n)"
ret=0
retry_quiet 10 zsks_are_published || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Check the logs for find zone keys errors.
n=$((n + 1))
echo_i "make sure we did not try to sign with the keys added with nsupdate for zone ${ZONE} ($n)"
ret=0
grep "dns_zone_findkeys: error reading ./K${ZONE}.*\.private: file not found" "${DIR}/named.run" && ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Verify again.
dnssec_verify

n=$((n + 1))
echo_i "add dnskey record: - update zone ${ZONE} at ns4 with ZSK from provider ns3 ($n)"
ret=0
set_server "ns4" "10.53.0.4"
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update add $(cat "ns3/${ZONE}.zsk")
  echo send
) | $NSUPDATE || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Check the new DNSKEY RRset.
n=$((n + 1))
echo_i "check zone ${ZONE} DNSKEY RRset after update ($n)"
ret=0
retry_quiet 10 zsks_are_published || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Check the logs for find zone keys errors.
n=$((n + 1))
echo_i "make sure we did not try to sign with the keys added with nsupdate for zone ${ZONE} ($n)"
ret=0
grep "dns_zone_findkeys: error reading ./K${ZONE}.*\.private: file not found" "${DIR}/named.run" && ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Verify again.
dnssec_verify
no_dnssec_in_journal

n=$((n + 1))
echo_i "remove dnskey record: - try to remove ns3 ZSK from provider ns3 (should fail) ($n)"
ret=0
set_server "ns3" "10.53.0.3"
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update del $(cat "ns3/${ZONE}.zsk")
  echo send
) | $NSUPDATE || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Both ZSKs should still be published.
n=$((n + 1))
echo_i "check zone ${ZONE} DNSKEY RRset after failed update ($n)"
ret=0
retry_quiet 10 zsks_are_published || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "remove dnskey record: remove ns4 ZSK from provider ns3 ($n)"
ret=0
set_server "ns3" "10.53.0.3"
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update del $(cat "ns4/${ZONE}.zsk")
  echo send
) | $NSUPDATE || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# We should have only the KSK and ZSK from provider ns3.
n=$((n + 1))
echo_i "check zone ${ZONE} DNSKEY RRset after update ($n)"
ret=0
check_keys
check_apex
dnssec_verify

n=$((n + 1))
echo_i "remove dnskey record: try to remove ns4 ZSK from provider ns4 (should fail) ($n)"
ret=0
set_server "ns4" "10.53.0.4"
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update del $(cat "ns4/${ZONE}.zsk")
  echo send
) | $NSUPDATE || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Both ZSKs should still be published.
n=$((n + 1))
echo_i "check zone ${ZONE} DNSKEY RRset after failed update ($n)"
ret=0
retry_quiet 10 zsks_are_published || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "remove dnskey record: remove ns3 ZSK from provider ns4 ($n)"
ret=0
set_server "ns4" "10.53.0.4"
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update del $(cat "ns3/${ZONE}.zsk")
  echo send
) | $NSUPDATE
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# We should have only the KSK and ZSK from provider ns4.
n=$((n + 1))
echo_i "check zone ${ZONE} DNSKEY RRset after update ($n)"
ret=0
check_keys
check_apex
dnssec_verify
no_dnssec_in_journal

#
# Update CDNSKEY RRset.
#

# Check that the CDNSKEY from both providers are published.
records_published() {
  _rrtype=$1
  _expect=$2

  dig_with_opts "$ZONE" "@${SERVER}" "${_rrtype}" >"dig.out.$DIR.test$n" || return 1
  lines=$(awk -v rt="${_rrtype}" '$4 == rt {print}' dig.out.$DIR.test$n | wc -l)
  test "$lines" -eq "$_expect" || return 1
}

# Retrieve CDNSKEY records from the other provider.
dig_with_opts ${ZONE} @10.53.0.3 CDNSKEY >dig.out.ns3.cdnskey
awk '$4 == "CDNSKEY" {print}' dig.out.ns3.cdnskey >cdnskey.ns3
dig_with_opts ${ZONE} @10.53.0.4 CDNSKEY >dig.out.ns4.cdnskey
awk '$4 == "CDNSKEY" {print}' dig.out.ns4.cdnskey >cdnskey.ns4

n=$((n + 1))
echo_i "add cdnskey record: update zone ${ZONE} at ns3 with CDNSKEY from provider ns4 ($n)"
ret=0
set_server "ns3" "10.53.0.3"
# Initially there should be one CDNSKEY.
retry_quiet 10 records_published CDNSKEY 1 || ret=1
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update add $(cat "cdnskey.ns4")
  echo send
) | $NSUPDATE || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Now there should be two CDNSKEY records (we test that BIND does not
# skip it during DNSSEC maintenance).
n=$((n + 1))
echo_i "check zone ${ZONE} CDNSKEY RRset after update ($n)"
ret=0
retry_quiet 10 records_published CDNSKEY 2 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "add cdnskey record: update zone ${ZONE} at ns4 with CDNSKEY from provider ns3 ($n)"
ret=0
set_server "ns4" "10.53.0.4"
# Initially there should be one CDNSKEY.
retry_quiet 10 records_published CDNSKEY 1 || ret=1
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update add $(cat "cdnskey.ns3")
  echo send
) | $NSUPDATE || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Now there should be two CDNSKEY records (we test that BIND does not
# skip it during DNSSEC maintenance).
n=$((n + 1))
echo_i "check zone ${ZONE} CDNSKEY RRset after update ($n)"
ret=0
retry_quiet 10 records_published CDNSKEY 2 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# No DNSSEC in raw journal.
no_dnssec_in_journal

n=$((n + 1))
echo_i "remove cdnskey record: remove ns4 CDNSKEY from provider ns3 ($n)"
ret=0
set_server "ns3" "10.53.0.3"
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update del $(cat "cdnskey.ns4")
  echo send
) | $NSUPDATE || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Now there should be one CDNSKEY record again.
n=$((n + 1))
echo_i "check zone ${ZONE} CDNSKEY RRset after update ($n)"
ret=0
retry_quiet 10 records_published CDNSKEY 1 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "remove cdnskey record: remove ns3 CDNSKEY from provider ns4 ($n)"
ret=0
set_server "ns4" "10.53.0.4"
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update del $(cat "cdnskey.ns3")
  echo send
) | $NSUPDATE || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Now there should be one CDNSKEY record again.
n=$((n + 1))
echo_i "check zone ${ZONE} CDNSKEY RRset after update ($n)"ret=0
retry_quiet 10 records_published CDNSKEY 1 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# No DNSSEC in raw journal.
no_dnssec_in_journal

#
# Update CDS RRset.
#

# Retrieve CDS records from the other provider.
dig_with_opts ${ZONE} @10.53.0.3 CDS >dig.out.ns3.cds
awk '$4 == "CDS" {print}' dig.out.ns3.cds >cds.ns3
dig_with_opts ${ZONE} @10.53.0.4 CDS >dig.out.ns4.cds
awk '$4 == "CDS" {print}' dig.out.ns4.cds >cds.ns4

n=$((n + 1))
echo_i "add cds record: update zone ${ZONE} at ns3 with CDS from provider ns4 ($n)"
ret=0
set_server "ns3" "10.53.0.3"
# Initially there should be one CDS.
retry_quiet 10 records_published CDS 1 || ret=1
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update add $(cat "cds.ns4")
  echo send
) | $NSUPDATE || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Now there should be two CDS records (we test that BIND does not
# skip it during DNSSEC maintenance).
n=$((n + 1))
echo_i "check zone ${ZONE} CDS RRset after update ($n)"
ret=0
retry_quiet 10 records_published CDS 2 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "add cds record: update zone ${ZONE} at ns4 with CDS from provider ns3 ($n)"
ret=0
set_server "ns4" "10.53.0.4"
# Initially there should be one CDS.
retry_quiet 10 records_published CDS 1 || ret=1
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update add $(cat "cds.ns3")
  echo send
) | $NSUPDATE || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Now there should be two CDS records (we test that BIND does not
# skip it during DNSSEC maintenance).
n=$((n + 1))
echo_i "check zone ${ZONE} CDS RRset after update ($n)"
ret=0
retry_quiet 10 records_published CDS 2 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# No DNSSEC in raw journal.
no_dnssec_in_journal

n=$((n + 1))
echo_i "remove cds record: remove ns4 CDS from provider ns3 ($n)"
ret=0
set_server "ns3" "10.53.0.3"
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update del $(cat "cds.ns4")
  echo send
) | $NSUPDATE || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Now there should be one CDS record again.
n=$((n + 1))
echo_i "check zone ${ZONE} CDS RRset after update ($n)"
ret=0
retry_quiet 10 records_published CDS 1 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "remove cds record: remove ns3 CDS from provider ns4 ($n)"
ret=0
set_server "ns4" "10.53.0.4"
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update del $(cat "cds.ns3")
  echo send
) | $NSUPDATE || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Now there should be one CDS record again.
n=$((n + 1))
echo_i "check zone ${ZONE} CDS RRset after update ($n)"
ret=0
retry_quiet 10 records_published CDS 1 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# No DNSSEC in raw journal.
no_dnssec_in_journal

#
# Check secondary server behaviour.
#
set_zone "model2.secondary"
set_policy "model2" "2" "3600"

set_server "ns3" "10.53.0.3"
check_keys
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE"
set_keytimes_model2
check_keytimes
check_apex
dnssec_verify

set_server "ns4" "10.53.0.4"
check_keys
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE"
set_keytimes_model2
check_keytimes
check_apex
dnssec_verify

#
# Update DNSKEY RRset.
#
n=$((n + 1))
echo_i "add dnskey record: update zone ${ZONE} at ns5 with ZSKs from providers ns3 and ns4 ($n)"
ret=0
set_server "ns5" "10.53.0.5"
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update add $(cat "ns3/${ZONE}.zsk")
  echo update add $(cat "ns4/${ZONE}.zsk")
  echo send
) | $NSUPDATE || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# NS3
n=$((n + 1))
set_server "ns3" "10.53.0.3"
echo_i "check server ${DIR} zone ${ZONE} DNSKEY RRset after update ($n)"
ret=0
retry_quiet 10 zsks_are_published || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
dnssec_verify
no_dnssec_in_journal
grep "dns_zone_findkeys: error reading ./K${ZONE}.*\.private: file not found" "${DIR}/named.run" && ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# NS4
n=$((n + 1))
set_server "ns4" "10.53.0.4"
echo_i "check server ${DIR} zone ${ZONE} DNSKEY RRset after update ($n)"
ret=0
retry_quiet 10 zsks_are_published || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
dnssec_verify
no_dnssec_in_journal
grep "dns_zone_findkeys: error reading ./K${ZONE}.*\.private: file not found" "${DIR}/named.run" && ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "remove dnskey record: remove ns3 and ns4 DNSKEY records from primary ns5 ($n)"
ret=0
set_server "ns5" "10.53.0.5"
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update del $(cat "ns3/${ZONE}.zsk")
  echo update del $(cat "ns4/${ZONE}.zsk")
  echo send
) | $NSUPDATE || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Now there should be one DNSKEY record again.
# While we did remove both DNSKEY records, the bump in the wire signer, i.e
# the secondary inline-signing zone, should add back the DNSKEY belonging to
# its own KSK when re-signing the zone.
#
# NS3
n=$((n + 1))
set_server "ns3" "10.53.0.3"
echo_i "check server ${DIR} zone ${ZONE} DNSKEY RRset after update ($n)"
ret=0
check_keys
check_apex
dnssec_verify
no_dnssec_in_journal
# NS4
n=$((n + 1))
set_server "ns4" "10.53.0.4"
echo_i "check server ${DIR} zone ${ZONE} DNSKEY RRset after update ($n)"
ret=0
check_keys
check_apex
dnssec_verify
no_dnssec_in_journal

#
# Update CDNSKEY RRset.
#

# Retrieve CDNSKEY records from the providers.
n=$((n + 1))
echo_i "check initial CDSNKEY response for zone ${ZONE} at ns3 and ns4 ($n)"
ret=0
dig_with_opts ${ZONE} @10.53.0.3 CDNSKEY >dig.out.ns3.secondary.cdnskey
awk '$4 == "CDNSKEY" {print}' dig.out.ns3.secondary.cdnskey >secondary.cdnskey.ns3
dig_with_opts ${ZONE} @10.53.0.4 CDNSKEY >dig.out.ns4.secondary.cdnskey
awk '$4 == "CDNSKEY" {print}' dig.out.ns4.secondary.cdnskey >secondary.cdnskey.ns4
# Initially there should be one CDNSKEY.
set_server "ns3" "10.53.0.3"
retry_quiet 10 records_published CDNSKEY 1 || ret=1
set_server "ns4" "10.53.0.4"
retry_quiet 10 records_published CDNSKEY 1 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "add cdnskey record: update zone ${ZONE} at ns5 with CDNSKEY records from providers ns3 and ns4 ($n)"
ret=0
set_server "ns5" "10.53.0.5"
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update add $(cat "secondary.cdnskey.ns3")
  echo update add $(cat "secondary.cdnskey.ns4")
  echo send
) | $NSUPDATE || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Now there should be two CDNSKEY records (we test that BIND does not
# skip it during DNSSEC maintenance).
#
# NS3
n=$((n + 1))
set_server "ns3" "10.53.0.3"
echo_i "check server ${DIR} zone ${ZONE} CDNSKEY RRset after update ($n)"
ret=0
retry_quiet 10 records_published CDNSKEY 2 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
dnssec_verify
no_dnssec_in_journal
# NS4
n=$((n + 1))
set_server "ns4" "10.53.0.4"
echo_i "check server ${DIR} zone ${ZONE} CDNSKEY RRset after update ($n)"
ret=0
retry_quiet 10 records_published CDNSKEY 2 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
dnssec_verify
no_dnssec_in_journal

n=$((n + 1))
echo_i "remove cdnskey record: remove ns3 and ns4 CDNSKEY records from primary ns5 ($n)"
ret=0
set_server "ns5" "10.53.0.5"
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update del $(cat "secondary.cdnskey.ns3")
  echo update del $(cat "secondary.cdnskey.ns4")
  echo send
) | $NSUPDATE || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Now there should be one CDNSKEY record again.
# While we did remove both CDNSKEY records, the bump in the wire signer, i.e
# the secondary inline-signing zone, should add back the CDNSKEY belonging to
# its own KSK when re-signing the zone.
#
# NS3
n=$((n + 1))
set_server "ns3" "10.53.0.3"
echo_i "check server ${DIR} zone ${ZONE} CDNSKEY RRset after update ($n)"
ret=0
retry_quiet 10 records_published CDNSKEY 1 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
dnssec_verify
no_dnssec_in_journal
# NS4
n=$((n + 1))
set_server "ns4" "10.53.0.4"
echo_i "check server ${DIR} zone ${ZONE} CDNSKEY RRset after update ($n)"
ret=0
retry_quiet 10 records_published CDNSKEY 1 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
dnssec_verify
no_dnssec_in_journal

#
# Update CDS RRset.
#

# Retrieve CDS records from the other provider.
n=$((n + 1))
echo_i "check initial CDS response for zone ${ZONE} at ns3 and ns4 ($n)"
ret=0
dig_with_opts ${ZONE} @10.53.0.3 CDS >dig.out.ns3.secondary.cds
awk '$4 == "CDS" {print}' dig.out.ns3.secondary.cds >secondary.cds.ns3
dig_with_opts ${ZONE} @10.53.0.4 CDS >dig.out.ns4.secondary.cds
awk '$4 == "CDS" {print}' dig.out.ns4.secondary.cds >secondary.cds.ns4
# Initially there should be one CDS.
set_server "ns3" "10.53.0.3"
retry_quiet 10 records_published CDS 1 || ret=1
set_server "ns4" "10.53.0.4"
retry_quiet 10 records_published CDS 1 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "add cds record: update zone ${ZONE} at ns5 with CDS from provider ns4 ($n)"
ret=0
set_server "ns5" "10.53.0.5"
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update add $(cat "secondary.cds.ns3")
  echo update add $(cat "secondary.cds.ns4")
  echo send
) | $NSUPDATE || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Now there should be two CDS records (we test that BIND does not
# skip it during DNSSEC maintenance).
#
# NS3
n=$((n + 1))
set_server "ns3" "10.53.0.3"
echo_i "check server ${DIR} zone ${ZONE} CDS RRset after update ($n)"
ret=0
retry_quiet 10 records_published CDS 2 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
dnssec_verify
no_dnssec_in_journal
# NS4
n=$((n + 1))
set_server "ns4" "10.53.0.4"
echo_i "check server ${DIR} zone ${ZONE} CDS RRset after update ($n)"
ret=0
retry_quiet 10 records_published CDS 2 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
dnssec_verify
no_dnssec_in_journal

n=$((n + 1))
echo_i "remove cds record: remove ns3 and ns4 CDS records from primary ns5 ($n)"
ret=0
set_server "ns5" "10.53.0.5"
(
  echo zone "${ZONE}"
  echo server "${SERVER}" "${PORT}"
  echo update del $(cat "secondary.cds.ns3")
  echo update del $(cat "secondary.cds.ns4")
  echo send
) | $NSUPDATE || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
# Now there should be one CDS record again.
# While we did remove both CDS records, the bump in the wire signer, i.e
# the secondary inline-signing zone, should add back the CDS belonging to
# its own KSK when re-signing the zone.
#
# NS3
n=$((n + 1))
set_server "ns3" "10.53.0.3"
echo_i "check server ${DIR} zone ${ZONE} CDS RRset after update ($n)"
ret=0
retry_quiet 10 records_published CDS 1 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
dnssec_verify
no_dnssec_in_journal
# NS4
n=$((n + 1))
set_server "ns4" "10.53.0.4"
echo_i "check server ${DIR} zone ${ZONE} CDS RRset after update ($n)"
ret=0
retry_quiet 10 records_published CDS 1 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))
dnssec_verify
no_dnssec_in_journal

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
