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

# shellcheck source=conf.sh
. ../conf.sh
# shellcheck source=kasp.sh
. ../kasp.sh

start_time="$(TZ=UTC date +%s)"
status=0
n=0

###############################################################################
# Utilities                                                                   #
###############################################################################

# Call dig with default options.
dig_with_opts() {

	if [ -n "$TSIG" ]; then
		"$DIG" +tcp +noadd +nosea +nostat +nocmd +dnssec -p "$PORT" -y "$TSIG" "$@"
	else
		"$DIG" +tcp +noadd +nosea +nostat +nocmd +dnssec -p "$PORT" "$@"
	fi
}

# Log error and increment failure rate.
log_error() {
	echo_i "error: $1"
	ret=$((ret+1))
}

# Default next key event threshold. May be extended by wait periods.
next_key_event_threshold=100

###############################################################################
# Tests                                                                       #
###############################################################################

set_retired_removed() {
	_Lkey=$2
	_Iret=$3

	_active=$(key_get $1 ACTIVE)
	set_addkeytime "${1}" "RETIRED" "${_active}"  "${_Lkey}"
	_retired=$(key_get $1 RETIRED)
	set_addkeytime "${1}" "REMOVED" "${_retired}" "${_Iret}"
}

rollover_predecessor_keytimes() {
	_addtime=$1

	_created=$(key_get KEY1 CREATED)

	set_addkeytime  "KEY1" "PUBLISHED"   "${_created}" "${_addtime}"
	set_addkeytime  "KEY1" "SYNCPUBLISH" "${_created}" "${_addtime}"
	set_addkeytime  "KEY1" "ACTIVE"      "${_created}" "${_addtime}"
	[ "$Lksk" = 0 ] || set_retired_removed "KEY1" "${Lksk}" "${IretKSK}"

	_created=$(key_get KEY2 CREATED)
	set_addkeytime  "KEY2" "PUBLISHED"   "${_created}" "${_addtime}"
	set_addkeytime  "KEY2" "ACTIVE"      "${_created}" "${_addtime}"
	[ "$Lzsk" = 0 ] || set_retired_removed "KEY2" "${Lzsk}" "${IretZSK}"
}

# Policy parameters.
# Lksk: unlimited
# Lzsk: unlimited
Lksk=0
Lzsk=0


#################################################
# Test state before switching to dnssec-policy. #
#################################################

# Set expected key properties for migration tests.
# $1 $2: Algorithm number and string.
# $3 $4: KSK and ZSK size.
init_migration_keys() {
	key_clear        "KEY1"
	key_set          "KEY1" "LEGACY" "yes"
	set_keyrole      "KEY1" "ksk"
	set_keylifetime  "KEY1" "none"
	set_keyalgorithm "KEY1" "$1" "$2" "$3"
	set_keysigning   "KEY1" "yes"
	set_zonesigning  "KEY1" "no"

	key_clear        "KEY2"
	key_set          "KEY2" "LEGACY" "yes"
	set_keyrole      "KEY2" "zsk"
	set_keylifetime  "KEY2" "none"
	set_keyalgorithm "KEY2" "$1" "$2" "$4"
	set_keysigning   "KEY2" "no"
	set_zonesigning  "KEY2" "yes"

	key_clear        "KEY3"
	key_clear        "KEY4"
}

# Set expected key states for migration tests.
# $1: Goal
# $2: States
init_migration_states() {
	set_keystate "KEY1" "GOAL"         "$1"
	set_keystate "KEY1" "STATE_DNSKEY" "$2"
	set_keystate "KEY1" "STATE_KRRSIG" "$2"
	set_keystate "KEY1" "STATE_DS"     "$2"

	set_keystate "KEY2" "GOAL"         "$1"
	set_keystate "KEY2" "STATE_DNSKEY" "$2"
	set_keystate "KEY2" "STATE_ZRRSIG" "$2"
}

#
# Testing a good migration.
#
set_zone "migrate.kasp"
set_policy "none" "2" "7200"
set_server "ns3" "10.53.0.3"

init_migration_keys "$DEFAULT_ALGORITHM_NUMBER" "$DEFAULT_ALGORITHM" "$DEFAULT_BITS" "$DEFAULT_BITS"
init_migration_states "omnipresent" "rumoured"

# Make sure the zone is signed with legacy keys.
check_keys
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE"
# These keys are immediately published and activated.
rollover_predecessor_keytimes 0
check_keytimes
check_apex
check_subdomain
dnssec_verify
# Remember legacy key tags.
_migrate_ksk=$(key_get KEY1 ID)
_migrate_zsk=$(key_get KEY2 ID)

#
# Testing a good migration (CSK).
#
set_zone "csk.kasp"
set_policy "none" "1" "7200"
set_server "ns3" "10.53.0.3"

key_clear        "KEY1"
key_set          "KEY1" "LEGACY" "yes"
set_keyrole      "KEY1" "ksk"
# This key also acts as a ZSK.
key_set          "KEY1" "ZSK" "yes"
set_keylifetime  "KEY1" "none"
set_keyalgorithm "KEY1" "$DEFAULT_ALGORITHM_NUMBER" "$DEFAULT_ALGORITHM" "$DEFAULT_BITS" "$DEFAULT_BITS"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "yes"

set_keystate "KEY1" "GOAL"         "omnipresent"
set_keystate "KEY1" "STATE_DNSKEY" "rumoured"
set_keystate "KEY1" "STATE_KRRSIG" "rumoured"
set_keystate "KEY1" "STATE_ZRRSIG" "rumoured"
set_keystate "KEY1" "STATE_DS"     "rumoured"

key_clear "KEY2"
key_clear "KEY3"
key_clear "KEY4"

# Make sure the zone is signed with legacy key.
check_keys
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE"
# The key is immediately published and activated.
_created=$(key_get KEY1 CREATED)
set_keytime "KEY1" "PUBLISHED"   "${_created}"
set_keytime "KEY1" "SYNCPUBLISH" "${_created}"
set_keytime "KEY1" "ACTIVE"      "${_created}"

check_keytimes
check_apex
check_subdomain
dnssec_verify
# Remember legacy key tags.
_migrate_csk=$(key_get KEY1 ID)

#
# Testing a good migration (CSK, no SEP).
#
set_zone "csk-nosep.kasp"
set_policy "none" "1" "7200"
set_server "ns3" "10.53.0.3"

key_clear        "KEY1"
key_set          "KEY1" "LEGACY" "yes"
set_keyrole      "KEY1" "zsk"
# Despite the missing SEP bit, this key also acts as a KSK.
key_set          "KEY1" "KSK" "yes"
set_keylifetime  "KEY1" "none"
set_keyalgorithm "KEY1" "$DEFAULT_ALGORITHM_NUMBER" "$DEFAULT_ALGORITHM" "$DEFAULT_BITS" "$DEFAULT_BITS"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "yes"

set_keystate "KEY1" "GOAL"         "omnipresent"
set_keystate "KEY1" "STATE_DNSKEY" "rumoured"
set_keystate "KEY1" "STATE_KRRSIG" "rumoured"
set_keystate "KEY1" "STATE_ZRRSIG" "rumoured"
set_keystate "KEY1" "STATE_DS"     "rumoured"

key_clear "KEY2"
key_clear "KEY3"
key_clear "KEY4"

# Make sure the zone is signed with legacy key.
check_keys
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE"
# The key is immediately published and activated.
_created=$(key_get KEY1 CREATED)
set_keytime "KEY1" "PUBLISHED"   "${_created}"
set_keytime "KEY1" "SYNCPUBLISH" "${_created}"
set_keytime "KEY1" "ACTIVE"      "${_created}"

check_keytimes
check_apex
check_subdomain
dnssec_verify
# Remember legacy key tags.
_migrate_csk_nosep=$(key_get KEY1 ID)

#
# Testing key states derived from key timing metadata (rumoured).
#
set_zone "rumoured.kasp"
set_policy "none" "2" "300"
set_server "ns3" "10.53.0.3"

init_migration_keys "$DEFAULT_ALGORITHM_NUMBER" "$DEFAULT_ALGORITHM" "$DEFAULT_BITS" "$DEFAULT_BITS"
init_migration_states "omnipresent" "rumoured"

# Make sure the zone is signed with legacy keys.
check_keys
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE"
check_apex
check_subdomain
dnssec_verify
# Remember legacy key tags.
_rumoured_ksk=$(key_get KEY1 ID)
_rumoured_zsk=$(key_get KEY2 ID)

#
# Testing key states derived from key timing metadata (omnipresent).
#
set_zone "omnipresent.kasp"
set_policy "none" "2" "300"
set_server "ns3" "10.53.0.3"

init_migration_keys "$DEFAULT_ALGORITHM_NUMBER" "$DEFAULT_ALGORITHM" "$DEFAULT_BITS" "$DEFAULT_BITS"
init_migration_states "omnipresent" "omnipresent"

# Make sure the zone is signed with legacy keys.
check_keys
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE"
check_apex
check_subdomain
dnssec_verify
# Remember legacy key tags.
_omnipresent_ksk=$(key_get KEY1 ID)
_omnipresent_zsk=$(key_get KEY2 ID)

#
# Testing migration with unmatched existing keys (different algorithm).
#
set_zone "migrate-nomatch-algnum.kasp"
set_policy "none" "2" "300"
set_server "ns3" "10.53.0.3"

init_migration_keys "8" "RSASHA256" "2048" "2048"
init_migration_states "omnipresent" "omnipresent"

# Make sure the zone is signed with legacy keys.
check_keys
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE"

# The KSK is immediately published and activated.
# -P     : now-3900s
# -P sync: now-3h
# -A     : now-3900s
created=$(key_get KEY1 CREATED)
set_addkeytime "KEY1" "PUBLISHED"   "${created}" -3900
set_addkeytime "KEY1" "ACTIVE"      "${created}" -3900
set_addkeytime "KEY1" "SYNCPUBLISH" "${created}" -10800
# The ZSK is immediately published and activated.
# -P: now-3900s
# -A: now-12h
created=$(key_get KEY2 CREATED)
set_addkeytime "KEY2" "PUBLISHED"   "${created}" -3900
set_addkeytime "KEY2" "ACTIVE"      "${created}" -43200
check_keytimes
check_apex
check_subdomain
dnssec_verify

# Remember legacy key tags.
_migratenomatch_algnum_ksk=$(key_get KEY1 ID)
_migratenomatch_algnum_zsk=$(key_get KEY2 ID)

#
# Testing migration with unmatched existing keys (different length).
#
set_zone "migrate-nomatch-alglen.kasp"
set_policy "none" "2" "300"
set_server "ns3" "10.53.0.3"

init_migration_keys "8" "RSASHA256" "2048" "2048"
init_migration_states "omnipresent" "omnipresent"

# Make sure the zone is signed with legacy keys.
check_keys
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE"

# Set expected key times:
# - The KSK is immediately published and activated.
#   P     : now-3900s
#   P sync: now-3h
#   A     : now-3900s
created=$(key_get KEY1 CREATED)
set_addkeytime "KEY1" "PUBLISHED"   "${created}" -3900
set_addkeytime "KEY1" "ACTIVE"      "${created}" -3900
set_addkeytime "KEY1" "SYNCPUBLISH" "${created}" -10800
# - The ZSK is immediately published and activated.
#   P: now-3900s
#   A: now-12h
created=$(key_get KEY2 CREATED)
set_addkeytime "KEY2" "PUBLISHED"   "${created}" -3900
set_addkeytime "KEY2" "ACTIVE"      "${created}" -43200
check_keytimes
check_apex
check_subdomain
dnssec_verify

# Remember legacy key tags.
_migratenomatch_alglen_ksk=$(key_get KEY1 ID)
_migratenomatch_alglen_zsk=$(key_get KEY2 ID)


#############
# Reconfig. #
#############
echo_i "reconfig (migration to dnssec-policy)"
copy_setports ns3/named2.conf.in ns3/named.conf
rndc_reconfig ns3 10.53.0.3

# Calculate time passed to correctly check for next key events.
now="$(TZ=UTC date +%s)"
time_passed=$((now-start_time))
echo_i "${time_passed} seconds passed between start of tests and reconfig"

# Wait until we have seen "zone_rekey done:" message for this key.
_wait_for_done_signing() {
	_zone=$1

	_ksk=$(key_get $2 KSK)
	_zsk=$(key_get $2 ZSK)
	if [ "$_ksk" = "yes" ]; then
		_role="KSK"
		_expect_type=EXPECT_KRRSIG
	elif [ "$_zsk" = "yes" ]; then
		_role="ZSK"
		_expect_type=EXPECT_ZRRSIG
	fi

	if [ "$(key_get ${2} $_expect_type)" = "yes" ] && [ "$(key_get $2 $_role)" = "yes" ]; then
		_keyid=$(key_get $2 ID)
		_keyalg=$(key_get $2 ALG_STR)
		echo_i "wait for zone ${_zone} is done signing with $2 ${_zone}/${_keyalg}/${_keyid}"
		grep "zone_rekey done: key ${_keyid}/${_keyalg}" "${DIR}/named.run" > /dev/null || return 1
	fi

	return 0
}
wait_for_done_signing() {
	n=$((n+1))
	echo_i "wait for zone ${ZONE} is done signing ($n)"
	ret=0

	retry_quiet 30 _wait_for_done_signing ${ZONE} KEY1 || ret=1
	retry_quiet 30 _wait_for_done_signing ${ZONE} KEY2 || ret=1
	retry_quiet 30 _wait_for_done_signing ${ZONE} KEY3 || ret=1
	retry_quiet 30 _wait_for_done_signing ${ZONE} KEY4 || ret=1

	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}


################################################
# Test state after switching to dnssec-policy. #
################################################

# Policy parameters.
# ZSK now has lifetime of 60 days (5184000 seconds).
# The key is removed after Iret = TTLsig + Dprp + Dsgn + retire-safety.
Lzsk=5184000
IretZSK=867900

#
# Testing good migration.
#
set_zone "migrate.kasp"
set_policy "migrate" "2" "7200"
set_server "ns3" "10.53.0.3"

# Key properties, timings and metadata should be the same as legacy keys above.
# However, because the zsk has a lifetime, kasp will set the retired time.
init_migration_keys "$DEFAULT_ALGORITHM_NUMBER" "$DEFAULT_ALGORITHM" "$DEFAULT_BITS" "$DEFAULT_BITS"
init_migration_states "omnipresent" "rumoured"
key_set "KEY1" "LEGACY" "no"
key_set "KEY2" "LEGACY" "no"
set_keylifetime "KEY1" "${Lksk}"
set_keylifetime "KEY2" "${Lzsk}"

# Various signing policy checks.
check_keys
wait_for_done_signing
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE"

# Set expected key times:
rollover_predecessor_keytimes 0

# - Key now has lifetime of 60 days (5184000 seconds).
#   The key is removed after Iret = TTLsig + Dprp + Dsgn + retire-safety.
#   TTLsig:        1d (86400 seconds)
#   Dprp:          5m (300 seconds)
#   Dsgn:          9d (777600 seconds)
#   retire-safety: 1h (3600 seconds)
#   IretZSK:       10d65m (867900 seconds)
active=$(key_get KEY2 ACTIVE)
set_addkeytime "KEY2" "RETIRED"     "${active}"  "${Lzsk}"
retired=$(key_get KEY2 RETIRED)
set_addkeytime "KEY2" "REMOVED"     "${retired}" "${IretZSK}"

# Continue signing policy checks.
check_keytimes
check_apex
check_subdomain
dnssec_verify

# Check key tags, should be the same.
n=$((n+1))
echo_i "check that of zone ${ZONE} migration to dnssec-policy uses the same keys ($n)"
ret=0
[ $_migrate_ksk = $(key_get KEY1 ID) ] || log_error "mismatch ksk tag"
[ $_migrate_zsk = $(key_get KEY2 ID) ] || log_error "mismatch zsk tag"
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

#
# Testing a good migration (CSK).
#
set_zone "csk.kasp"
set_policy "default" "1" "7200"
set_server "ns3" "10.53.0.3"

key_clear        "KEY1"
key_set          "KEY1" "LEGACY" "no"
set_keyrole      "KEY1" "csk"
set_keylifetime  "KEY1" "0"
set_keyalgorithm "KEY1" "$DEFAULT_ALGORITHM_NUMBER" "$DEFAULT_ALGORITHM" "$DEFAULT_BITS" "$DEFAULT_BITS"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "yes"

set_keystate "KEY1" "GOAL"         "omnipresent"
set_keystate "KEY1" "STATE_DNSKEY" "rumoured"
set_keystate "KEY1" "STATE_KRRSIG" "rumoured"
set_keystate "KEY1" "STATE_ZRRSIG" "rumoured"
set_keystate "KEY1" "STATE_DS"     "rumoured"

key_clear "KEY2"
key_clear "KEY3"
key_clear "KEY4"

# Various signing policy checks.
check_keys
wait_for_done_signing
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE"

# The key was immediately published and activated.
_created=$(key_get KEY1 CREATED)
set_keytime "KEY1" "PUBLISHED"   "${_created}"
set_keytime "KEY1" "SYNCPUBLISH" "${_created}"
set_keytime "KEY1" "ACTIVE"      "${_created}"

# Continue signing policy checks.
check_keytimes
check_apex
check_subdomain
dnssec_verify

# Check key tags, should be the same.
n=$((n+1))
echo_i "check that of zone ${ZONE} migration to dnssec-policy uses the same key ($n)"
ret=0
[ $_migrate_csk = $(key_get KEY1 ID) ] || log_error "mismatch csk tag"
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

#
# Testing a good migration (CSK, no SEP).
#
set_zone "csk-nosep.kasp"
set_policy "default" "1" "7200"
set_server "ns3" "10.53.0.3"

key_clear        "KEY1"
key_set          "KEY1" "LEGACY" "no"
set_keyrole      "KEY1" "csk"
key_set          "KEY1" "FLAGS" "256"
set_keylifetime  "KEY1" "0"
set_keyalgorithm "KEY1" "$DEFAULT_ALGORITHM_NUMBER" "$DEFAULT_ALGORITHM" "$DEFAULT_BITS" "$DEFAULT_BITS"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "yes"

set_keystate "KEY1" "GOAL"         "omnipresent"
set_keystate "KEY1" "STATE_DNSKEY" "rumoured"
set_keystate "KEY1" "STATE_KRRSIG" "rumoured"
set_keystate "KEY1" "STATE_ZRRSIG" "rumoured"
set_keystate "KEY1" "STATE_DS"     "rumoured"

key_clear "KEY2"
key_clear "KEY3"
key_clear "KEY4"

# Various signing policy checks.
check_keys
wait_for_done_signing
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE"

# The key was immediately published and activated.
_created=$(key_get KEY1 CREATED)
set_keytime "KEY1" "PUBLISHED"   "${_created}"
set_keytime "KEY1" "SYNCPUBLISH" "${_created}"
set_keytime "KEY1" "ACTIVE"      "${_created}"

# Continue signing policy checks.
check_keytimes
check_apex
check_subdomain
dnssec_verify

# Check key tags, should be the same.
n=$((n+1))
echo_i "check that of zone ${ZONE} migration to dnssec-policy uses the same key ($n)"
ret=0
[ $_migrate_csk_nosep = $(key_get KEY1 ID) ] || log_error "mismatch csk tag"
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

#
# Test migration to dnssec-policy, existing keys do not match key algorithm.
#
set_zone "migrate-nomatch-algnum.kasp"
set_policy "migrate-nomatch-algnum" "4" "300"
set_server "ns3" "10.53.0.3"
# The legacy keys need to be retired, but otherwise stay present until the
# new keys are omnipresent, and can be used to construct a chain of trust.
init_migration_keys "8" "RSASHA256" "2048" "2048"
init_migration_states "hidden" "omnipresent"
key_set "KEY1" "LEGACY" "no"
key_set "KEY2" "LEGACY" "no"

set_keyrole      "KEY3" "ksk"
set_keylifetime  "KEY3" "0"
set_keyalgorithm "KEY3" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY3" "yes"
set_zonesigning  "KEY3" "no"

set_keyrole      "KEY4" "zsk"
set_keylifetime  "KEY4" "5184000"
set_keyalgorithm "KEY4" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY4" "no"
set_zonesigning  "KEY4" "yes"

set_keystate "KEY3" "GOAL"         "omnipresent"
set_keystate "KEY3" "STATE_DNSKEY" "rumoured"
set_keystate "KEY3" "STATE_KRRSIG" "rumoured"
set_keystate "KEY3" "STATE_DS"     "hidden"

set_keystate "KEY4" "GOAL"         "omnipresent"
set_keystate "KEY4" "STATE_DNSKEY" "rumoured"
set_keystate "KEY4" "STATE_ZRRSIG" "rumoured"

# Various signing policy checks.
check_keys
wait_for_done_signing
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE"

# Set expected key times:
# - KSK must be retired since it no longer matches the policy.
#   P     : now-3900s
#   P sync: now-3h
#   A     : now-3900s
# - The key is removed after the retire interval:
#   IretKSK = TTLds + DprpP + retire_safety.
#   TTLds:         2h (7200 seconds)
#   Dprp:          1h (3600 seconds)
#   retire-safety: 1h (3600 seconds)
#   IretKSK:       4h (14400 seconds)
IretKSK=14400
created=$(key_get KEY1 CREATED)
set_addkeytime "KEY1" "PUBLISHED"   "${created}" -3900
set_addkeytime "KEY1" "ACTIVE"      "${created}" -3900
set_addkeytime "KEY1" "SYNCPUBLISH" "${created}" -10800
keyfile=$(key_get KEY1 BASEFILE)
grep "; Inactive:" "${keyfile}.key" > retired.test${n}.ksk
retired=$(awk '{print $3}' < retired.test${n}.ksk)
set_keytime    "KEY1" "RETIRED" "${retired}"
set_addkeytime "KEY1" "REMOVED" "${retired}" "${IretKSK}"
# - ZSK must be retired since it no longer matches the policy.
#   P: now-3900s
#   A: now-12h
# - The key is removed after the retire interval:
#   IretZSK = TTLsig + Dprp + Dsgn + retire-safety.
#   TTLsig:        11h (39600 seconds)
#   Dprp:          1h (3600 seconds)
#   Dsgn:          9d (777600 seconds)
#   retire-safety: 1h (3600 seconds)
#   IretZSK:       9d13h (824400 seconds)
IretZSK=824400
Lzsk=5184000
created=$(key_get KEY2 CREATED)
set_addkeytime "KEY2" "PUBLISHED"   "${created}" -3900
set_addkeytime "KEY2" "ACTIVE"      "${created}" -43200
keyfile=$(key_get KEY2 BASEFILE)
grep "; Inactive:" "${keyfile}.key" > retired.test${n}.zsk
retired=$(awk '{print $3}' < retired.test${n}.zsk)
set_keytime    "KEY2" "RETIRED" "${retired}"
set_addkeytime "KEY2" "REMOVED" "${retired}" "${IretZSK}"
# - The new KSK is immediately published and activated.
created=$(key_get KEY3 CREATED)
set_keytime    "KEY3" "PUBLISHED"   "${created}"
set_keytime    "KEY3" "ACTIVE"      "${created}"
# - It takes TTLsig + Dprp + publish-safety hours to propagate the zone.
#   TTLsig:         11h (39600 seconds)
#   Dprp:           1h (3600 seconds)
#   publish-safety: 1h (3600 seconds)
#   Ipub:           13h (46800 seconds)
Ipub=46800
set_addkeytime "KEY3" "SYNCPUBLISH" "${created}" "${Ipub}"
# - The ZSK is immediately published and activated.
created=$(key_get KEY4 CREATED)
set_keytime    "KEY4" "PUBLISHED"   "${created}"
set_keytime    "KEY4" "ACTIVE"      "${created}"
active=$(key_get KEY4 ACTIVE)
set_addkeytime "KEY4" "RETIRED"     "${active}"  "${Lzsk}"
retired=$(key_get KEY4 RETIRED)
set_addkeytime "KEY4" "REMOVED"     "${retired}" "${IretZSK}"

# Continue signing policy checks.
check_keytimes
check_apex
check_subdomain
dnssec_verify

# Check key tags, should be the same.
n=$((n+1))
echo_i "check that of zone ${ZONE} migration to dnssec-policy keeps existing keys ($n)"
ret=0
[ $_migratenomatch_algnum_ksk = $(key_get KEY1 ID) ] || log_error "mismatch ksk tag"
[ $_migratenomatch_algnum_zsk = $(key_get KEY2 ID) ] || log_error "mismatch zsk tag"
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

#
# Test migration to dnssec-policy, existing keys do not match key length.
#
set_zone "migrate-nomatch-alglen.kasp"
set_policy "migrate-nomatch-alglen" "4" "300"
set_server "ns3" "10.53.0.3"

# The legacy keys need to be retired, but otherwise stay present until the
# new keys are omnipresent, and can be used to construct a chain of trust.
init_migration_keys "8" "RSASHA256" "2048" "2048"
init_migration_states "hidden" "omnipresent"
key_set "KEY1" "LEGACY" "no"
key_set "KEY2" "LEGACY" "no"

set_keyrole      "KEY3" "ksk"
set_keylifetime  "KEY3" "0"
set_keyalgorithm "KEY3" "8" "RSASHA256" "3072"
set_keysigning   "KEY3" "yes"
set_zonesigning  "KEY3" "no"

set_keyrole      "KEY4" "zsk"
set_keylifetime  "KEY4" "5184000"
set_keyalgorithm "KEY4" "8" "RSASHA256" "3072"
set_keysigning   "KEY4" "no"
# This key is considered to be prepublished, so it is not yet signing.
set_zonesigning  "KEY4" "no"

set_keystate "KEY3" "GOAL"         "omnipresent"
set_keystate "KEY3" "STATE_DNSKEY" "rumoured"
set_keystate "KEY3" "STATE_KRRSIG" "rumoured"
set_keystate "KEY3" "STATE_DS"     "hidden"

set_keystate "KEY4" "GOAL"         "omnipresent"
set_keystate "KEY4" "STATE_DNSKEY" "rumoured"
set_keystate "KEY4" "STATE_ZRRSIG" "hidden"

# Various signing policy checks.
check_keys
wait_for_done_signing
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE"

# Set expected key times:
# - KSK must be retired since it no longer matches the policy.
#   P     : now-3900s
#   P sync: now-3h
#   A     : now-3900s
# - The key is removed after the retire interval:
#   IretKSK = TTLds + DprpP + retire_safety.
#   TTLds:         2h (7200 seconds)
#   Dprp:          1h (3600 seconds)
#   retire-safety: 1h (3600 seconds)
#   IretKSK:       4h (14400 seconds)
IretKSK=14400
created=$(key_get KEY1 CREATED)
set_addkeytime "KEY1" "PUBLISHED"   "${created}" -3900
set_addkeytime "KEY1" "ACTIVE"      "${created}" -3900
set_addkeytime "KEY1" "SYNCPUBLISH" "${created}" -10800
keyfile=$(key_get KEY1 BASEFILE)
grep "; Inactive:" "${keyfile}.key" > retired.test${n}.ksk
retired=$(awk '{print $3}' < retired.test${n}.ksk)
set_keytime    "KEY1" "RETIRED" "${retired}"
set_addkeytime "KEY1" "REMOVED" "${retired}" "${IretKSK}"
# - ZSK must be retired since it no longer matches the policy.
#   P: now-3900s
#   A: now-12h
# - The key is removed after the retire interval:
#   IretZSK = TTLsig + Dprp + Dsgn + retire-safety.
#   TTLsig:         11h (39600 seconds)
#   Dprp:           1h (3600 seconds)
#   Dsgn:           9d (777600 seconds)
#   publish-safety: 1h (3600 seconds)
#   IretZSK:        9d13h (824400 seconds)
IretZSK=824400
Lzsk=5184000
created=$(key_get KEY2 CREATED)
set_addkeytime "KEY2" "PUBLISHED"   "${created}" -3900
set_addkeytime "KEY2" "ACTIVE"      "${created}" -43200
keyfile=$(key_get KEY2 BASEFILE)
grep "; Inactive:" "${keyfile}.key" > retired.test${n}.zsk
retired=$(awk '{print $3}' < retired.test${n}.zsk)
set_keytime    "KEY2" "RETIRED" "${retired}"
set_addkeytime "KEY2" "REMOVED" "${retired}" "${IretZSK}"
# - The new KSK is immediately published and activated.
created=$(key_get KEY3 CREATED)
set_keytime    "KEY3" "PUBLISHED"   "${created}"
set_keytime    "KEY3" "ACTIVE"      "${created}"
# - It takes TTLsig + Dprp + publish-safety hours to propagate the zone.
#   TTLsig:         11h (39600 seconds)
#   Dprp:           1h (3600 seconds)
#   publish-safety: 1h (3600 seconds)
#   Ipub:           13h (46800 seconds)
Ipub=46800
set_addkeytime "KEY3" "SYNCPUBLISH" "${created}" "${Ipub}"
# - The ZSK is immediately published and activated.
created=$(key_get KEY4 CREATED)
set_keytime    "KEY4" "PUBLISHED"   "${created}"
set_keytime    "KEY4" "ACTIVE"      "${created}"
active=$(key_get KEY4 ACTIVE)
set_addkeytime "KEY4" "RETIRED"     "${active}"  "${Lzsk}"
retired=$(key_get KEY4 RETIRED)
set_addkeytime "KEY4" "REMOVED"     "${retired}" "${IretZSK}"

# Continue signing policy checks.
check_keytimes
check_apex
check_subdomain
dnssec_verify

# Check key tags, should be the same.
n=$((n+1))
echo_i "check that of zone ${ZONE} migration to dnssec-policy keeps existing keys ($n)"
ret=0
[ $_migratenomatch_alglen_ksk = $(key_get KEY1 ID) ] || log_error "mismatch ksk tag"
[ $_migratenomatch_alglen_zsk = $(key_get KEY2 ID) ] || log_error "mismatch zsk tag"
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

########################################################
# Testing key states derived from key timing metadata. #
########################################################

# Policy parameters.
# KSK has lifetime of 60 days (5184000 seconds).
# The KSK is removed after Iret = DprpP + TTLds + retire-safety =
# 4h = 14400 seconds.
Lksk=5184000
IretKSK=14400
# ZSK has lifetime of 60 days (5184000 seconds).
# The ZSK is removed after Iret = TTLsig + Dprp + Dsgn + retire-safety =
# 181h = 651600 seconds.
Lzsk=5184000
IretZSK=651600

#
# Testing rumoured state.
#
set_zone "rumoured.kasp"
set_policy "timing-metadata" "2" "300"
set_server "ns3" "10.53.0.3"

# Key properties, timings and metadata should be the same as legacy keys above.
init_migration_keys "$DEFAULT_ALGORITHM_NUMBER" "$DEFAULT_ALGORITHM" "$DEFAULT_BITS" "$DEFAULT_BITS"
init_migration_states "omnipresent" "rumoured"
key_set "KEY1" "LEGACY" "no"
key_set "KEY2" "LEGACY" "no"
set_keylifetime "KEY1" "${Lksk}"
set_keylifetime "KEY2" "${Lzsk}"

# Various signing policy checks.
check_keys
wait_for_done_signing
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE"

# Set expected key times:
#
# Tds="now-2h"    (7200)
# Tkey="now-300s" (300)
# Tsig="now-11h"  (39600)
created=$(key_get KEY1 CREATED)
set_addkeytime      "KEY1" "PUBLISHED"   "${created}" -300
set_addkeytime      "KEY1" "ACTIVE"      "${created}" -300
set_addkeytime      "KEY1" "SYNCPUBLISH" "${created}"  -7200
set_retired_removed "KEY1" "${Lksk}" "${IretKSK}"
created=$(key_get KEY2 CREATED)
set_addkeytime      "KEY2" "PUBLISHED"   "${created}"  -300
set_addkeytime      "KEY2" "ACTIVE"      "${created}"  -39600
set_retired_removed "KEY2" "${Lzsk}" "${IretZSK}"

# Continue signing policy checks.
check_keytimes
check_apex
check_subdomain
dnssec_verify

# Check key tags, should be the same.
n=$((n+1))
echo_i "check that of zone ${ZONE} migration to dnssec-policy uses the same keys ($n)"
ret=0
[ $_rumoured_ksk = $(key_get KEY1 ID) ] || log_error "mismatch ksk tag"
[ $_rumoured_zsk = $(key_get KEY2 ID) ] || log_error "mismatch zsk tag"
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

#
# Testing omnipresent state.
#
set_zone "omnipresent.kasp"
set_policy "timing-metadata" "2" "300"
set_server "ns3" "10.53.0.3"

# Key properties, timings and metadata should be the same as legacy keys above.
init_migration_keys "$DEFAULT_ALGORITHM_NUMBER" "$DEFAULT_ALGORITHM" "$DEFAULT_BITS" "$DEFAULT_BITS"
init_migration_states "omnipresent" "omnipresent"
key_set "KEY1" "LEGACY" "no"
key_set "KEY2" "LEGACY" "no"
set_keylifetime "KEY1" "${Lksk}"
set_keylifetime "KEY2" "${Lzsk}"

# Various signing policy checks.
check_keys
wait_for_done_signing
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE"

# Set expected key times:
#
# Tds="now-3h"     (10800)
# Tkey="now-3900s" (3900)
# Tsig="now-12h"   (43200)
created=$(key_get KEY1 CREATED)
set_addkeytime      "KEY1" "PUBLISHED"   "${created}" -3900
set_addkeytime      "KEY1" "ACTIVE"      "${created}" -3900
set_addkeytime      "KEY1" "SYNCPUBLISH" "${created}"  -10800
set_retired_removed "KEY1" "${Lksk}" "${IretKSK}"
created=$(key_get KEY2 CREATED)
set_addkeytime      "KEY2" "PUBLISHED"   "${created}"  -3900
set_addkeytime      "KEY2" "ACTIVE"      "${created}"  -43200
set_retired_removed "KEY2" "${Lzsk}" "${IretZSK}"

# Continue signing policy checks.
check_keytimes
check_apex
check_subdomain
dnssec_verify

# Check key tags, should be the same.
n=$((n+1))
echo_i "check that of zone ${ZONE} migration to dnssec-policy uses the same keys ($n)"
ret=0
[ $_omnipresent_ksk = $(key_get KEY1 ID) ] || log_error "mismatch ksk tag"
[ $_omnipresent_zsk = $(key_get KEY2 ID) ] || log_error "mismatch zsk tag"
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))


######################################
# Testing good migration with views. #
######################################
init_view_migration() {
	key_clear        "KEY1"
	key_set          "KEY1" "LEGACY" "yes"
	set_keyrole      "KEY1" "ksk"
	set_keylifetime  "KEY1" "0"
	set_keysigning   "KEY1" "yes"
	set_zonesigning  "KEY1" "no"

	key_clear        "KEY2"
	key_set          "KEY2" "LEGACY" "yes"
	set_keyrole      "KEY2" "zsk"
	set_keylifetime  "KEY2" "0"
	set_keysigning   "KEY2" "no"
	set_zonesigning  "KEY2" "yes"

	key_clear        "KEY3"
	key_clear        "KEY4"

	set_keystate "KEY1" "GOAL"         "omnipresent"
	set_keystate "KEY1" "STATE_DNSKEY" "rumoured"
	set_keystate "KEY1" "STATE_KRRSIG" "rumoured"
	set_keystate "KEY1" "STATE_DS"     "rumoured"

	set_keystate "KEY2" "GOAL"         "omnipresent"
	set_keystate "KEY2" "STATE_DNSKEY" "rumoured"
	set_keystate "KEY2" "STATE_ZRRSIG" "rumoured"
}

set_keytimes_view_migration() {
	# Key is six months in use.
	created=$(key_get KEY1 CREATED)
	set_addkeytime "KEY1" "PUBLISHED"   "${created}" -16070400
	set_addkeytime "KEY1" "SYNCPUBLISH" "${created}" -16070400
	set_addkeytime "KEY1" "ACTIVE"      "${created}" -16070400
	created=$(key_get KEY2 CREATED)
	set_addkeytime "KEY2" "PUBLISHED"   "${created}" -16070400
	set_addkeytime "KEY2" "ACTIVE"      "${created}" -16070400
}

# Zone view.rsasha256.kasp (external)
set_zone "view-rsasha256.kasp"
set_policy "rsasha256" "2" "300"
set_server "ns4" "10.53.0.4"
init_view_migration
set_keyalgorithm "KEY1" "8" "RSASHA256" "2048"
set_keyalgorithm "KEY2" "8" "RSASHA256" "2048"
TSIG="$DEFAULT_HMAC:external:$VIEW1"
wait_for_nsec
# Make sure the zone is signed with legacy keys.
check_keys
set_keytimes_view_migration
check_keytimes
dnssec_verify

n=$((n+1))
# check subdomain
echo_i "check TXT $ZONE (view ext) rrset is signed correctly ($n)"
ret=0
dig_with_opts "view.${ZONE}" "@${SERVER}" TXT > "dig.out.$DIR.test$n.txt" || log_error "dig view.${ZONE} TXT failed"
grep "status: NOERROR" "dig.out.$DIR.test$n.txt" > /dev/null || log_error "mismatch status in DNS response"
grep "view.${ZONE}\..*${DEFAULT_TTL}.*IN.*TXT.*external" "dig.out.$DIR.test$n.txt" > /dev/null || log_error "missing view.${ZONE} TXT record in response"
check_signatures TXT "dig.out.$DIR.test$n.txt" "ZSK"
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

# Remember legacy key tags.
_migrate_ext8_ksk=$(key_get KEY1 ID)
_migrate_ext8_zsk=$(key_get KEY2 ID)

# Zone view.rsasha256.kasp (internal)
set_zone "view-rsasha256.kasp"
set_policy "rsasha256" "2" "300"
set_server "ns4" "10.53.0.4"
init_view_migration
set_keyalgorithm "KEY1" "8" "RSASHA256" "2048"
set_keyalgorithm "KEY2" "8" "RSASHA256" "2048"
TSIG="$DEFAULT_HMAC:internal:$VIEW2"
wait_for_nsec
# Make sure the zone is signed with legacy keys.
check_keys
set_keytimes_view_migration
check_keytimes
dnssec_verify

n=$((n+1))
# check subdomain
echo_i "check TXT $ZONE (view int) rrset is signed correctly ($n)"
ret=0
dig_with_opts "view.${ZONE}" "@${SERVER}" TXT > "dig.out.$DIR.test$n.txt" || log_error "dig view.${ZONE} TXT failed"
grep "status: NOERROR" "dig.out.$DIR.test$n.txt" > /dev/null || log_error "mismatch status in DNS response"
grep "view.${ZONE}\..*${DEFAULT_TTL}.*IN.*TXT.*internal" "dig.out.$DIR.test$n.txt" > /dev/null || log_error "missing view.${ZONE} TXT record in response"
check_signatures TXT "dig.out.$DIR.test$n.txt" "ZSK"
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

# Remember legacy key tags.
_migrate_int8_ksk=$(key_get KEY1 ID)
_migrate_int8_zsk=$(key_get KEY2 ID)

# Reconfig dnssec-policy.
echo_i "reconfig to switch to dnssec-policy"
copy_setports ns4/named2.conf.in ns4/named.conf
rndc_reconfig ns4 10.53.0.4

# Calculate time passed to correctly check for next key events.
now="$(TZ=UTC date +%s)"
time_passed=$((now-start_time))
echo_i "${time_passed} seconds passed between start of tests and reconfig"

#
# Testing migration (RSASHA256, views).
#
set_zone "view-rsasha256.kasp"
set_policy "rsasha256" "3" "300"
set_server "ns4" "10.53.0.4"
init_migration_keys "8" "RSASHA256" "2048" "2048"
init_migration_states "omnipresent" "rumoured"
# Key properties, timings and metadata should be the same as legacy keys above.
# However, because the keys have a lifetime, kasp will set the retired time.
key_set          "KEY1" "LEGACY" "no"
set_keylifetime  "KEY1" "31536000"
set_keystate     "KEY1" "STATE_DNSKEY" "omnipresent"
set_keystate     "KEY1" "STATE_KRRSIG" "omnipresent"
set_keystate     "KEY1" "STATE_DS"     "omnipresent"

key_set          "KEY2" "LEGACY" "no"
set_keylifetime  "KEY2" "8035200"
set_keystate     "KEY2" "STATE_DNSKEY" "omnipresent"
set_keystate     "KEY2" "STATE_ZRRSIG" "omnipresent"
# The ZSK needs to be replaced.
set_keystate     "KEY2" "GOAL" "hidden"
set_keystate     "KEY3" "GOAL" "omnipresent"
set_keyrole      "KEY3" "zsk"
set_keylifetime  "KEY3" "8035200"
set_keyalgorithm "KEY3" "8" "RSASHA256" "2048"
set_keysigning   "KEY3" "no"
set_zonesigning  "KEY3" "no" # not yet
set_keystate     "KEY3" "STATE_DNSKEY" "rumoured"
set_keystate     "KEY3" "STATE_ZRRSIG" "hidden"

# Various signing policy checks (external).
TSIG="$DEFAULT_HMAC:external:$VIEW1"
check_keys
wait_for_done_signing
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE" "ext"
set_keytimes_view_migration

# Set expected key times:
published=$(key_get KEY1 PUBLISHED)
set_keytime "KEY1" "ACTIVE"      "${published}"
set_keytime "KEY1" "SYNCPUBLISH" "${published}"
# Lifetime: 1 year (8035200 seconds)
active=$(key_get KEY1 ACTIVE)
set_addkeytime "KEY1" "RETIRED" "${active}"  "31536000"
# Retire interval:
# DS TTL:                  1d
# Parent zone propagation: 3h
# Retire safety:           1h
# Total:                   100800 seconds
retired=$(key_get KEY1 RETIRED)
set_addkeytime "KEY1" "REMOVED" "${retired}" "100800"

published=$(key_get KEY2 PUBLISHED)
set_keytime "KEY2" "ACTIVE" "${published}"
# Lifetime: 3 months (8035200 seconds)
active=$(key_get KEY2 ACTIVE)
set_addkeytime "KEY2" "RETIRED" "${active}" "8035200"
# Retire interval:
# Sign delay:             9d (14-5)
# Max zone TTL:           1d
# Retire safety:          1h
# Zone propagation delay: 300s
# Total:                  867900 seconds
retired=$(key_get KEY2 RETIRED)
set_addkeytime "KEY2" "REMOVED" "${retired}" "867900"

created=$(key_get KEY3 CREATED)
set_keytime    "KEY3" "PUBLISHED" "${created}"
# Publication interval:
# DNSKEY TTL:             300s
# Publish safety:         1h
# Zone propagation delay: 300s
# Total:                  4200 seconds
set_addkeytime "KEY3" "ACTIVE" "${created}" "4200"
# Lifetime: 3 months (8035200 seconds)
active=$(key_get KEY3 ACTIVE)
set_addkeytime "KEY3" "RETIRED" "${active}" "8035200"
# Retire interval:
# Sign delay:             9d (14-5)
# Max zone TTL:           1d
# Retire safety:          1h
# Zone propagation delay: 300s
# Total:                  867900 seconds
retired=$(key_get KEY3 RETIRED)
set_addkeytime "KEY3" "REMOVED" "${retired}" "867900"

# Continue signing policy checks.
check_keytimes
check_apex
dnssec_verify

# Various signing policy checks (internal).
TSIG="$DEFAULT_HMAC:internal:$VIEW2"
check_keys
wait_for_done_signing
check_dnssecstatus "$SERVER" "$POLICY" "$ZONE" "int"
set_keytimes_view_migration
check_keytimes
check_apex
dnssec_verify

# Check key tags, should be the same.
n=$((n+1))
echo_i "check that of zone ${ZONE} migration to dnssec-policy uses the same keys ($n)"
ret=0
[ $_migrate_ext8_ksk = $_migrate_int8_ksk ] || log_error "mismatch ksk tag"
[ $_migrate_ext8_zsk = $_migrate_int8_zsk ] || log_error "mismatch zsk tag"
[ $_migrate_ext8_ksk = $(key_get KEY1 ID) ] || log_error "mismatch ksk tag"
[ $_migrate_ext8_zsk = $(key_get KEY2 ID) ] || log_error "mismatch zsk tag"
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
