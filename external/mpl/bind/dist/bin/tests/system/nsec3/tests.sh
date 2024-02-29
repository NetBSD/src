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

# Log errors and increment $ret.
log_error() {
  echo_i "error: $1"
  ret=$((ret + 1))
}

# Call dig with default options.
dig_with_opts() {
  $DIG +tcp +noadd +nosea +nostat +nocmd +dnssec -p "$PORT" "$@"
}

# Call rndc.
rndccmd() {
  "$RNDC" -c ../_common/rndc.conf -p "$CONTROLPORT" -s "$@"
}

# Set zone name ($1) and policy ($2) for testing nsec3.
set_zone_policy() {
  ZONE=$1
  POLICY=$2
  NUM_KEYS=$3
  DNSKEY_TTL=$4
}
# Set expected NSEC3 parameters: flags ($1), iterations ($2), and
# salt length ($3).
set_nsec3param() {
  FLAGS=$1
  ITERATIONS=$2
  SALTLEN=$3
  # Reset salt.
  SALT=""
}

# Set expected default dnssec-policy keys values.
set_key_default_values() {
  key_clear $1

  set_keyrole $1 "csk"
  set_keylifetime $1 "0"
  set_keyalgorithm $1 "13" "ECDSAP256SHA256" "256"
  set_keysigning $1 "yes"
  set_zonesigning $1 "yes"

  set_keystate $1 "GOAL" "omnipresent"
  set_keystate $1 "STATE_DNSKEY" "rumoured"
  set_keystate $1 "STATE_KRRSIG" "rumoured"
  set_keystate $1 "STATE_ZRRSIG" "rumoured"
  set_keystate $1 "STATE_DS" "hidden"
}

# Set expected rsasha1 dnssec-policy keys values.
set_key_rsasha1_values() {
  key_clear $1

  set_keyrole $1 "csk"
  set_keylifetime $1 "0"
  set_keyalgorithm $1 "5" "RSASHA1" "2048"
  set_keysigning $1 "yes"
  set_zonesigning $1 "yes"

  set_keystate $1 "GOAL" "omnipresent"
  set_keystate $1 "STATE_DNSKEY" "rumoured"
  set_keystate $1 "STATE_KRRSIG" "rumoured"
  set_keystate $1 "STATE_ZRRSIG" "rumoured"
  set_keystate $1 "STATE_DS" "hidden"
}

# Update the key states.
set_key_states() {
  set_keystate $1 "GOAL" "$2"
  set_keystate $1 "STATE_DNSKEY" "$3"
  set_keystate $1 "STATE_KRRSIG" "$4"
  set_keystate $1 "STATE_ZRRSIG" "$5"
  set_keystate $1 "STATE_DS" "$6"
}

# The apex NSEC3PARAM record indicates that it is signed.
_wait_for_nsec3param() {
  dig_with_opts +noquestion "@${SERVER}" "$ZONE" NSEC3PARAM >"dig.out.test$n.wait" || return 1
  grep "${ZONE}\..*IN.*NSEC3PARAM.*1.*0.*${ITERATIONS}.*${SALT}" "dig.out.test$n.wait" >/dev/null || return 1
  grep "${ZONE}\..*IN.*RRSIG" "dig.out.test$n.wait" >/dev/null || return 1
  return 0
}
# The apex NSEC record indicates that it is signed.
_wait_for_nsec() {
  dig_with_opts +noquestion "@${SERVER}" "$ZONE" NSEC >"dig.out.test$n.wait" || return 1
  grep "NS SOA" "dig.out.test$n.wait" >/dev/null || return 1
  grep "${ZONE}\..*IN.*RRSIG" "dig.out.test$n.wait" >/dev/null || return 1
  grep "${ZONE}\..*IN.*NSEC3PARAM" "dig.out.test$n.wait" >/dev/null && return 1
  return 0
}

# Wait for the zone to be signed.
wait_for_zone_is_signed() {
  n=$((n + 1))
  ret=0
  echo_i "wait for ${ZONE} to be signed with $1 ($n)"

  if [ "$1" = "nsec3" ]; then
    retry_quiet 10 _wait_for_nsec3param || log_error "wait for ${ZONE} to be signed failed"
  else
    retry_quiet 10 _wait_for_nsec || log_error "wait for ${ZONE} to be signed failed"
  fi

  test "$ret" -eq 0 || echo_i "failed"
  status=$((status + ret))
}

# Test: check DNSSEC verify
_check_dnssec_verify() {
  dig_with_opts @$SERVER "${ZONE}" AXFR >"dig.out.test$n.axfr.$ZONE" || return 1
  $VERIFY -z -o "$ZONE" "dig.out.test$n.axfr.$ZONE" >"verify.out.test$n.$ZONE" 2>&1 || return 1
  return 0
}

# Test: check NSEC in answers
_check_nsec_nsec3param() {
  dig_with_opts +noquestion @$SERVER "${ZONE}" NSEC3PARAM >"dig.out.test$n.nsec3param.$ZONE" || return 1
  grep "NSEC3PARAM" "dig.out.test$n.nsec3param.$ZONE" >/dev/null && return 1
  return 0
}

_check_nsec_nxdomain() {
  dig_with_opts @$SERVER "nosuchname.${ZONE}" >"dig.out.test$n.nxdomain.$ZONE" || return 1
  grep "${ZONE}.*IN.*NSEC.*NS.*SOA.*RRSIG.*NSEC.*DNSKEY" "dig.out.test$n.nxdomain.$ZONE" >/dev/null || return 1
  grep "NSEC3" "dig.out.test$n.nxdomain.$ZONE" >/dev/null && return 1
  return 0
}

check_nsec() {
  wait_for_zone_is_signed "nsec"

  n=$((n + 1))
  echo_i "check DNSKEY rrset is signed correctly for zone ${ZONE} ($n)"
  ret=0
  check_keys
  retry_quiet 10 _check_apex_dnskey || log_error "bad DNSKEY RRset for zone ${ZONE}"
  test "$ret" -eq 0 || echo_i "failed"
  status=$((status + ret))

  n=$((n + 1))
  echo_i "verify DNSSEC for zone ${ZONE} ($n)"
  ret=0
  retry_quiet 10 _check_dnssec_verify || log_error "DNSSEC verify failed for zone ${ZONE}"
  test "$ret" -eq 0 || echo_i "failed"
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check NSEC3PARAM response for zone ${ZONE} ($n)"
  ret=0
  retry_quiet 10 _check_nsec_nsec3param || log_error "unexpected NSEC3PARAM in response for zone ${ZONE}"
  test "$ret" -eq 0 || echo_i "failed"
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check NXDOMAIN response for zone ${ZONE} ($n)"
  ret=0
  retry_quiet 10 _check_nsec_nxdomain || log_error "bad NXDOMAIN response for zone ${ZONE}"
  test "$ret" -eq 0 || echo_i "failed"
  status=$((status + ret))
}

# Test: check NSEC3 parameters in answers
_check_nsec3_nsec3param() {
  dig_with_opts +noquestion @$SERVER "${ZONE}" NSEC3PARAM >"dig.out.test$n.nsec3param.$ZONE" || return 1
  grep "${ZONE}.*0.*IN.*NSEC3PARAM.*1.*0.*${ITERATIONS}.*${SALT}" "dig.out.test$n.nsec3param.$ZONE" >/dev/null || return 1

  if [ -z "$SALT" ]; then
    SALT=$(awk '$4 == "NSEC3PARAM" { print $8 }' dig.out.test$n.nsec3param.$ZONE)
  fi
  return 0
}

_check_nsec3_nxdomain() {
  dig_with_opts @$SERVER "nosuchname.${ZONE}" >"dig.out.test$n.nxdomain.$ZONE" || return 1
  grep ".*\.${ZONE}.*IN.*NSEC3.*1.${FLAGS}.*${ITERATIONS}.*${SALT}" "dig.out.test$n.nxdomain.$ZONE" >/dev/null || return 1
  return 0
}

check_nsec3() {
  wait_for_zone_is_signed "nsec3"

  n=$((n + 1))
  echo_i "check that NSEC3PARAM 1 0 ${ITERATIONS} is published zone ${ZONE} ($n)"
  ret=0
  retry_quiet 10 _check_nsec3_nsec3param || log_error "bad NSEC3PARAM response for ${ZONE}"
  test "$ret" -eq 0 || echo_i "failed"
  status=$((status + ret))

  n=$((n + 1))
  echo_i "check NXDOMAIN response has correct NSEC3 1 ${FLAGS} ${ITERATIONS} ${SALT} for zone ${ZONE} ($n)"
  ret=0
  retry_quiet 10 _check_nsec3_nxdomain || log_error "bad NXDOMAIN response for zone ${ZONE}"
  test "$ret" -eq 0 || echo_i "failed"
  status=$((status + ret))

  n=$((n + 1))
  echo_i "verify DNSSEC for zone ${ZONE} ($n)"
  ret=0
  retry_quiet 10 _check_dnssec_verify || log_error "DNSSEC verify failed for zone ${ZONE}"
  test "$ret" -eq 0 || echo_i "failed"
  status=$((status + ret))
}

start_time="$(TZ=UTC date +%s)"
status=0
n=0

key_clear "KEY1"
key_clear "KEY2"
key_clear "KEY3"
key_clear "KEY4"

# Zone: nsec-to-nsec3.kasp.
set_zone_policy "nsec-to-nsec3.kasp" "nsec" 1 3600
set_server "ns3" "10.53.0.3"
set_key_default_values "KEY1"
echo_i "initial check zone ${ZONE}"
check_nsec

if ($SHELL ../testcrypto.sh -q RSASHA1); then
  # Zone: rsasha1-to-nsec3.kasp.
  set_zone_policy "rsasha1-to-nsec3.kasp" "rsasha1" 1 3600
  set_server "ns3" "10.53.0.3"
  set_key_rsasha1_values "KEY1"
  echo_i "initial check zone ${ZONE}"
  check_nsec

  # Zone: rsasha1-to-nsec3-wait.kasp.
  set_zone_policy "rsasha1-to-nsec3-wait.kasp" "rsasha1" 1 3600
  set_server "ns3" "10.53.0.3"
  set_key_rsasha1_values "KEY1"
  set_key_states "KEY1" "omnipresent" "omnipresent" "omnipresent" "omnipresent" "omnipresent"
  echo_i "initial check zone ${ZONE}"
  check_nsec

  # Zone: nsec3-to-rsasha1.kasp.
  set_zone_policy "nsec3-to-rsasha1.kasp" "nsec3" 1 3600
  set_server "ns3" "10.53.0.3"
  set_key_rsasha1_values "KEY1"
  echo_i "initial check zone ${ZONE}"
  check_nsec3

  # Zone: nsec3-to-rsasha1-ds.kasp.
  set_zone_policy "nsec3-to-rsasha1-ds.kasp" "nsec3" 1 3600
  set_server "ns3" "10.53.0.3"
  set_key_rsasha1_values "KEY1"
  set_key_states "KEY1" "omnipresent" "omnipresent" "omnipresent" "omnipresent" "omnipresent"
  echo_i "initial check zone ${ZONE}"
  check_nsec3
fi

# Zone: nsec3.kasp.
set_zone_policy "nsec3.kasp" "nsec3" 1 3600
set_nsec3param "0" "0" "0"
set_key_default_values "KEY1"
echo_i "initial check zone ${ZONE}"
check_nsec3

# Zone: nsec3-dynamic.kasp.
set_zone_policy "nsec3-dynamic.kasp" "nsec3" 1 3600
set_nsec3param "0" "0" "0"
set_key_default_values "KEY1"
echo_i "initial check zone ${ZONE}"
check_nsec3

# Zone: nsec3-change.kasp.
set_zone_policy "nsec3-change.kasp" "nsec3" 1 3600
set_nsec3param "0" "0" "0"
set_key_default_values "KEY1"
echo_i "initial check zone ${ZONE}"
check_nsec3

# Zone: nsec3-dynamic-change.kasp.
set_zone_policy "nsec3-dynamic-change.kasp" "nsec3" 1 3600
set_nsec3param "0" "0" "0"
set_key_default_values "KEY1"
echo_i "initial check zone ${ZONE}"
check_nsec3

# Zone: nsec3-dynamic-to-inline.kasp.
set_zone_policy "nsec3-dynamic-to-inline.kasp" "nsec3" 1 3600
set_nsec3param "0" "0" "0"
set_key_default_values "KEY1"
echo_i "initial check zone ${ZONE}"
check_nsec3

# Zone: nsec3-inline-to-dynamic.kasp.
set_zone_policy "nsec3-inline-to-dynamic.kasp" "nsec3" 1 3600
set_nsec3param "0" "0" "0"
set_key_default_values "KEY1"
echo_i "initial check zone ${ZONE}"
check_nsec3

# Zone: nsec3-to-nsec.kasp.
set_zone_policy "nsec3-to-nsec.kasp" "nsec3" 1 3600
set_nsec3param "0" "0" "0"
set_key_default_values "KEY1"
echo_i "initial check zone ${ZONE}"
check_nsec3

# Zone: nsec3-to-optout.kasp.
set_zone_policy "nsec3-to-optout.kasp" "nsec3" 1 3600
set_nsec3param "0" "0" "0"
set_key_default_values "KEY1"
echo_i "initial check zone ${ZONE}"
check_nsec3

# Zone: nsec3-from-optout.kasp.
set_zone_policy "nsec3-from-optout.kasp" "optout" 1 3600
set_nsec3param "1" "0" "0"
set_key_default_values "KEY1"
echo_i "initial check zone ${ZONE}"
check_nsec3

# Zone: nsec3-other.kasp.
set_zone_policy "nsec3-other.kasp" "nsec3-other" 1 3600
set_nsec3param "1" "11" "8"
set_key_default_values "KEY1"
echo_i "initial check zone ${ZONE}"
check_nsec3

# Zone: nsec3-xfr-inline.kasp.
# This is a secondary zone, where the primary is signed with NSEC3 but
# the dnssec-policy dictates NSEC.
set_zone_policy "nsec3-xfr-inline.kasp" "nsec" 1 3600
set_key_default_values "KEY1"
echo_i "initial check zone ${ZONE}"
check_nsec

# Zone: nsec3-dynamic-update-inline.kasp.
set_zone_policy "nsec3-dynamic-update-inline.kasp" "nsec" 1 3600
set_key_default_values "KEY1"
echo_i "initial check zone ${ZONE}"
check_nsec

n=$((n + 1))
echo_i "dynamic update dnssec-policy zone ${ZONE} with NSEC3 ($n)"
ret=0
$NSUPDATE >update.out.$ZONE.test$n 2>&1 <<END || ret=1
server 10.53.0.3 ${PORT}
zone ${ZONE}.
update add 04O18462RI5903H8RDVL0QDT5B528DUJ.${ZONE}. 3600 NSEC3 0 0 0 408A4B2D412A4E95 1JMDDPMTFF8QQLIOINSIG4CR9OTICAOC A RRSIG
send
END
wait_for_log 10 "updating zone '${ZONE}/IN': update failed: explicit NSEC3 updates are not allowed in secure zones (REFUSED)" ns3/named.run || ret=1
check_nsec

# Reconfig named.
ret=0
echo_i "reconfig dnssec-policy to trigger nsec3 rollovers"
copy_setports ns3/named2.conf.in ns3/named.conf
rndc_reconfig ns3 10.53.0.3

# Zone: nsec-to-nsec3.kasp. (reconfigured)
set_zone_policy "nsec-to-nsec3.kasp" "nsec3" 1 3600
set_nsec3param "0" "0" "0"
set_key_default_values "KEY1"
echo_i "check zone ${ZONE} after reconfig"
check_nsec3

if ($SHELL ../testcrypto.sh -q RSASHA1); then
  # Zone: rsasha1-to-nsec3.kasp.
  set_zone_policy "rsasha1-to-nsec3.kasp" "nsec3" 2 3600
  set_server "ns3" "10.53.0.3"
  set_key_rsasha1_values "KEY1"
  set_key_states "KEY1" "hidden" "unretentive" "unretentive" "unretentive" "hidden"
  set_keysigning "KEY1" "no"
  set_zonesigning "KEY1" "no"
  set_key_default_values "KEY2"
  echo_i "check zone ${ZONE} after reconfig"
  check_nsec3

  # Zone: rsasha1-to-nsec3-wait.kasp.
  set_zone_policy "rsasha1-to-nsec3-wait.kasp" "nsec3" 2 3600
  set_server "ns3" "10.53.0.3"
  set_key_rsasha1_values "KEY1"
  set_key_states "KEY1" "hidden" "omnipresent" "omnipresent" "omnipresent" "omnipresent"
  set_key_default_values "KEY2"
  echo_i "check zone ${ZONE} after reconfig"
  check_nsec

  # Zone: nsec3-to-rsasha1.kasp.
  set_zone_policy "nsec3-to-rsasha1.kasp" "rsasha1" 2 3600
  set_nsec3param "1" "0" "0"
  set_server "ns3" "10.53.0.3"
  set_key_default_values "KEY1"
  set_key_states "KEY1" "hidden" "unretentive" "unretentive" "unretentive" "hidden"
  set_keysigning "KEY1" "no"
  set_zonesigning "KEY1" "no"
  set_key_rsasha1_values "KEY2"
  echo_i "check zone ${ZONE} after reconfig"
  check_nsec

  # Zone: nsec3-to-rsasha1-ds.kasp.
  set_zone_policy "nsec3-to-rsasha1-ds.kasp" "rsasha1" 2 3600
  set_nsec3param "1" "0" "0"
  set_server "ns3" "10.53.0.3"
  set_key_default_values "KEY1"
  set_key_states "KEY1" "hidden" "omnipresent" "omnipresent" "omnipresent" "omnipresent"
  set_key_rsasha1_values "KEY2"
  echo_i "check zone ${ZONE} after reconfig"
  check_nsec

  key_clear "KEY1"
  key_clear "KEY2"
fi

# Zone: nsec3.kasp. (same)
set_zone_policy "nsec3.kasp" "nsec3" 1 3600
set_nsec3param "0" "0" "0"
set_key_default_values "KEY1"
echo_i "check zone ${ZONE} after reconfig"
check_nsec3

# Zone: nsec3-dyamic.kasp. (same)
set_zone_policy "nsec3-dynamic.kasp" "nsec3" 1 3600
set_nsec3param "0" "0" "0"
set_key_default_values "KEY1"
echo_i "check zone ${ZONE} after reconfig"
check_nsec3

# Zone: nsec3-change.kasp. (reconfigured)
set_zone_policy "nsec3-change.kasp" "nsec3-other" 1 3600
set_nsec3param "1" "11" "8"
set_key_default_values "KEY1"
echo_i "check zone ${ZONE} after reconfig"
check_nsec3

# Zone: nsec3-dynamic-change.kasp. (reconfigured)
set_zone_policy "nsec3-dynamic-change.kasp" "nsec3-other" 1 3600
set_nsec3param "1" "11" "8"
set_key_default_values "KEY1"
echo_i "check zone ${ZONE} after reconfig"
check_nsec3

# Zone: nsec3-dynamic-to-inline.kasp. (same)
set_zone_policy "nsec3-dynamic-to-inline.kasp" "nsec3" 1 3600
set_nsec3param "0" "0" "0"
set_key_default_values "KEY1"
echo_i "check zone ${ZONE} after reconfig"
check_nsec3

# Zone: nsec3-inline-to-dynamic.kasp. (same)
set_zone_policy "nsec3-inline-to-dynamic.kasp" "nsec3" 1 3600
set_nsec3param "0" "0" "0"
set_key_default_values "KEY1"
echo_i "initial check zone ${ZONE}"
check_nsec3

# Zone: nsec3-to-nsec.kasp. (reconfigured)
set_zone_policy "nsec3-to-nsec.kasp" "nsec" 1 3600
set_nsec3param "1" "11" "8"
set_key_default_values "KEY1"
echo_i "check zone ${ZONE} after reconfig"
check_nsec

# Zone: nsec3-to-optout.kasp. (reconfigured)
# DISABLED:
# There is a bug in the nsec3param building code that thinks when the
# optout bit is changed, the chain already exists. [GL #2216]
#set_zone_policy "nsec3-to-optout.kasp" "optout" 1 3600
#set_nsec3param "1" "0" "0"
#set_key_default_values "KEY1"
#echo_i "check zone ${ZONE} after reconfig"
#check_nsec3

# Zone: nsec3-from-optout.kasp. (reconfigured)
# DISABLED:
# There is a bug in the nsec3param building code that thinks when the
# optout bit is changed, the chain already exists. [GL #2216]
#set_zone_policy "nsec3-from-optout.kasp" "nsec3" 1 3600
#set_nsec3param "0" "0" "0"
#set_key_default_values "KEY1"
#echo_i "check zone ${ZONE} after reconfig"
#check_nsec3

# Zone: nsec3-other.kasp. (same)
set_zone_policy "nsec3-other.kasp" "nsec3-other" 1 3600
set_nsec3param "1" "11" "8"
set_key_default_values "KEY1"
echo_i "check zone ${ZONE} after reconfig"
check_nsec3

# Using rndc signing -nsec3param (should fail)
set_zone_policy "nsec3-change.kasp" "nsec3-other" 1 3600
echo_i "use rndc signing -nsec3param ${ZONE} to change NSEC3 settings"
rndccmd $SERVER signing -nsec3param 1 1 12 ffff $ZONE >rndc.signing.test$n.$ZONE || log_error "failed to call rndc signing -nsec3param $ZONE"
grep "zone uses dnssec-policy, use rndc dnssec command instead" rndc.signing.test$n.$ZONE >/dev/null || log_error "rndc signing -nsec3param should fail"
check_nsec3

# Test NSEC3 and NSEC3PARAM is the same after restart
set_zone_policy "nsec3.kasp" "nsec3" 1 3600
set_nsec3param "0" "0" "0"
set_key_default_values "KEY1"
echo_i "check zone ${ZONE} before restart"
check_nsec3

# Restart named, NSEC3 should stay the same.
ret=0
echo "stop ns3"
stop_server --use-rndc --port ${CONTROLPORT} ${DIR} || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))

ret=0
echo "start ns3"
start_server --noclean --restart --port ${PORT} ${DIR}
test "$ret" -eq 0 || echo_i "failed"
status=$((status + ret))

prevsalt="${SALT}"
set_zone_policy "nsec3.kasp" "nsec3" 1 3600
set_nsec3param "0" "0" "0"
set_key_default_values "KEY1"
SALT="${prevsalt}"
echo_i "check zone ${ZONE} after restart has salt ${SALT}"
check_nsec3

# Zone: nsec3-fails-to-load.kasp. (should be fixed after reload)
cp ns3/template.db.in ns3/nsec3-fails-to-load.kasp.db
rndc_reload ns3 10.53.0.3

set_zone_policy "nsec3-fails-to-load.kasp" "nsec3" 1 3600
set_nsec3param "0" "0" "0"
set_key_default_values "KEY1"
echo_i "check zone ${ZONE} after reload"
check_nsec3

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
