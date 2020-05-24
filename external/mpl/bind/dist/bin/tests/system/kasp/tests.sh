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

# shellcheck source=conf.sh
SYSTEMTESTTOP=..
. "$SYSTEMTESTTOP/conf.sh"

start_time="$(TZ=UTC date +%s)"
status=0
n=0

###############################################################################
# Constants                                                                   #
###############################################################################
DEFAULT_TTL=300

###############################################################################
# Query properties                                                            #
###############################################################################
TSIG=""
SHA1="FrSt77yPTFx6hTs4i2tKLB9LmE0="
SHA224="hXfwwwiag2QGqblopofai9NuW28q/1rH4CaTnA=="
SHA256="R16NojROxtxH/xbDl//ehDsHm5DjWTQ2YXV+hGC2iBY="
VIEW1="YPfMoAk6h+3iN8MDRQC004iSNHY="
VIEW2="4xILSZQnuO1UKubXHkYUsvBRPu8="

###############################################################################
# Key properties                                                              #
###############################################################################
# ID=0
# EXPECT=1
# ROLE=2
# KSK=3
# ZSK=4
# LIFETIME=5
# ALG_NUM=6
# ALG_STR=7
# ALG_LEN=8
# PUBLISHED=9
# ACTIVE=10
# RETIRED=11
# REVOKED=12
# REMOVED=13
# GOAL=14
# STATE_DNSKEY=15
# STATE_ZRRSIG=16
# STATE_KRRSIG=17
# STATE_DS=18
# EXPECT_ZRRSIG=19
# EXPECT_KRRSIG=20
# LEGACY=21

key_key() {
	echo "${1}__${2}"
}

key_get() {
	eval "echo \${$(key_key "$1" "$2")}"
}

key_set() {
	eval "$(key_key "$1" "$2")='$3'"
}

# Clear key state.
#
# This will update either the KEY1, KEY2, or KEY3 array.
key_clear() {
	key_set "$1" "ID" 'no'
	key_set "$1" "EXPECT" 'no'
	key_set "$1" "ROLE" 'none'
	key_set "$1" "KSK" 'no'
	key_set "$1" "ZSK" 'no'
	key_set "$1" "LIFETIME" 'none'
	key_set "$1" "ALG_NUM" '0'
	key_set "$1" "ALG_STR" 'none'
	key_set "$1" "ALG_LEN" '0'
	key_set "$1" "PUBLISHED" 'none'
	key_set "$1" "ACTIVE" 'none'
	key_set "$1" "RETIRED" 'none'
	key_set "$1" "REVOKED" 'none'
	key_set "$1" "REMOVED" 'none'
	key_set "$1" "GOAL" 'none'
	key_set "$1" "STATE_DNSKEY" 'none'
	key_set "$1" "STATE_KRRSIG" 'none'
	key_set "$1" "STATE_ZRRSIG" 'none'
	key_set "$1" "STATE_DS" 'none'
	key_set "$1" "EXPECT_ZRRSIG" 'no'
	key_set "$1" "EXPECT_KRRSIG" 'no'
	key_set "$1" "LEGACY" 'no'
}

# Start clear.
# There can be at most 4 keys at the same time during a rollover:
# 2x KSK, 2x ZSK
key_clear "KEY1"
key_clear "KEY2"
key_clear "KEY3"
key_clear "KEY4"

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

# RNDC.
rndccmd() {
	"$RNDC" -c "$SYSTEMTESTTOP/common/rndc.conf" -p "$CONTROLPORT" -s "$@"
}

# Print IDs of keys used for generating RRSIG records for RRsets of type $1
# found in dig output file $2.
get_keys_which_signed() {
	_qtype=$1
	_output=$2
	# The key ID is the 11th column of the RRSIG record line.
	awk -v qt="$_qtype" '$4 == "RRSIG" && $5 == qt {print $11}' < "$_output"
}

# Get the key ids from key files for zone $2 in directory $1.
get_keyids() {
	_dir=$1
	_zone=$2
	_regex="K${_zone}.+*+*.key"

	find "${_dir}" -mindepth 1 -maxdepth 1 -name "${_regex}" | sed "s,$_dir/K${_zone}.+\([0-9]\{3\}\)+\([0-9]\{5\}\).key,\2,"
}

# By default log errors and don't quit immediately.
_log=1
log_error() {
	test $_log -eq 1 && echo_i "error: $1"
	ret=$((ret+1))
}
# Set server key-directory ($1) and address ($2) for testing keys.
set_server() {
	DIR=$1
	SERVER=$2
}
# Set zone name for testing keys.
set_zone() {
	ZONE=$1
}
# Set policy settings (name $1, number of keys $2, dnskey ttl $3) for testing keys.
set_policy() {
	POLICY=$1
	NUM_KEYS=$2
	DNSKEY_TTL=$3
}

# Set key properties for testing keys.
# $1: Key to update (KEY1, KEY2, ...)
# $2: Value
set_keyrole() {
	key_set "$1" "EXPECT" "yes"
	key_set "$1" "ROLE" "$2"
	key_set "$1" "KSK" "no"
	key_set "$1" "ZSK" "no"
	test "$2" = "ksk" && key_set "$1" "KSK" "yes"
	test "$2" = "zsk" && key_set "$1" "ZSK" "yes"
	test "$2" = "csk" && key_set "$1" "KSK" "yes"
	test "$2" = "csk" && key_set "$1" "ZSK" "yes"
}
set_keylifetime() {
	key_set "$1" "EXPECT" "yes"
	key_set "$1" "LIFETIME" "$2"
}
# The algorithm value consists of three parts:
# $2: Algorithm (number)
# $3: Algorithm (string-format)
# $4: Algorithm length
set_keyalgorithm() {
	key_set "$1" "EXPECT" "yes"
	key_set "$1" "ALG_NUM" "$2"
	key_set "$1" "ALG_STR" "$3"
	key_set "$1" "ALG_LEN" "$4"
}
set_keysigning() {
	key_set "$1" "EXPECT" "yes"
	key_set "$1" "EXPECT_KRRSIG" "$2"
}
set_zonesigning() {
	key_set "$1" "EXPECT" "yes"
	key_set "$1" "EXPECT_ZRRSIG" "$2"
}

# Set key timing metadata. Set to "none" to unset.
# These times are hard to test, so it is just an indication that we expect the
# respective timing metadata in the key files.
# $1: Key to update (KEY1, KEY2, ...)
# $2: Time to update (PUBLISHED, ACTIVE, RETIRED, REVOKED, or REMOVED).
# $3: Value
set_keytime() {
	key_set "$1" "EXPECT" "yes"
	key_set "$1" "$2" "$3"
}

# Set key state metadata. Set to "none" to unset.
# $1: Key to update (KEY1, KEY2, ...)
# $2: Key state to update (GOAL, STATE_DNSKEY, STATE_ZRRSIG, STATE_KRRSIG, or STATE_DS)
# $3: Value
set_keystate() {
	key_set "$1" "EXPECT" "yes"
	key_set "$1" "$2" "$3"
}

# Check the key $1 with id $2.
# This requires environment variables to be set.
#
# This will set the following environment variables for testing:
# BASE_FILE="${_dir}/K${_zone}.+${_alg_numpad}+${_key_idpad}"
# KEY_FILE="${BASE_FILE}.key"
# PRIVATE_FILE="${BASE_FILE}.private"
# STATE_FILE="${BASE_FILE}.state"
# KEY_ID=$(echo $1 | sed 's/^0\{0,4\}//')
check_key() {
	_dir="$DIR"
	_zone="$ZONE"
	_role=$(key_get "$1" ROLE)
	_key_idpad="$2"
	_key_id=$(echo "$_key_idpad" | sed 's/^0\{0,4\}//')
	_alg_num=$(key_get "$1" ALG_NUM)
	_alg_numpad=$(printf "%03d" "$_alg_num")
	_alg_string=$(key_get "$1" ALG_STR)
	_length=$(key_get "$1" "ALG_LEN")
	_dnskey_ttl="$DNSKEY_TTL"
	_lifetime=$(key_get "$1" LIFETIME)
	_legacy=$(key_get "$1" LEGACY)

	_published=$(key_get "$1" PUBLISHED)
	_active=$(key_get "$1" ACTIVE)
	_retired=$(key_get "$1" RETIRED)
	_revoked=$(key_get "$1" REVOKED)
	_removed=$(key_get "$1" REMOVED)

	_goal=$(key_get "$1" GOAL)
	_state_dnskey=$(key_get "$1" STATE_DNSKEY)
	_state_zrrsig=$(key_get "$1" STATE_ZRRSIG)
	_state_krrsig=$(key_get "$1" STATE_KRRSIG)
	_state_ds=$(key_get "$1" STATE_DS)

	_ksk="no"
	_zsk="no"
	if [ "$_role" = "ksk" ]; then
		_role2="key-signing"
		_ksk="yes"
		_flags="257"
	elif [ "$_role" = "zsk" ]; then
		_role2="zone-signing"
		_zsk="yes"
		_flags="256"
	elif [ "$_role" = "csk" ]; then
		_role2="key-signing"
		_zsk="yes"
		_ksk="yes"
		_flags="257"
	fi

	BASE_FILE="${_dir}/K${_zone}.+${_alg_numpad}+${_key_idpad}"
	KEY_FILE="${BASE_FILE}.key"
	PRIVATE_FILE="${BASE_FILE}.private"
	STATE_FILE="${BASE_FILE}.state"
	KEY_ID="${_key_id}"

	# Check file existence.
	[ -s "$KEY_FILE" ] || ret=1
	[ -s "$PRIVATE_FILE" ] || ret=1
	if [ "$_legacy" == "no" ]; then
		[ -s "$STATE_FILE" ] || ret=1
	fi
	[ "$ret" -eq 0 ] || log_error "${BASE_FILE} files missing"
	[ "$ret" -eq 0 ] || return

	test $_log -eq 1 && echo_i "check key file $BASE_FILE"

	# Check the public key file.
	grep "This is a ${_role2} key, keyid ${_key_id}, for ${_zone}." "$KEY_FILE" > /dev/null || log_error "mismatch top comment in $KEY_FILE"
	grep "${_zone}\. ${_dnskey_ttl} IN DNSKEY ${_flags} 3 ${_alg_num}" "$KEY_FILE" > /dev/null || log_error "mismatch DNSKEY record in $KEY_FILE"
	# Now check the private key file.
	grep "Private-key-format: v1.3" "$PRIVATE_FILE" > /dev/null || log_error "mismatch private key format in $PRIVATE_FILE"
	grep "Algorithm: ${_alg_num} (${_alg_string})" "$PRIVATE_FILE" > /dev/null || log_error "mismatch algorithm in $PRIVATE_FILE"
	# Now check the key state file.
	if [ "$_legacy" == "no" ]; then
		grep "This is the state of key ${_key_id}, for ${_zone}." "$STATE_FILE" > /dev/null || log_error "mismatch top comment in $STATE_FILE"
		if [ "$_lifetime" == "none" ]; then
			grep "Lifetime: " "$STATE_FILE" > /dev/null && log_error "unexpected lifetime in $STATE_FILE"
		else
			grep "Lifetime: ${_lifetime}" "$STATE_FILE" > /dev/null || log_error "mismatch lifetime in $STATE_FILE"
		fi
		grep "Algorithm: ${_alg_num}" "$STATE_FILE" > /dev/null || log_error "mismatch algorithm in $STATE_FILE"
		grep "Length: ${_length}" "$STATE_FILE" > /dev/null || log_error "mismatch length in $STATE_FILE"
		grep "KSK: ${_ksk}" "$STATE_FILE" > /dev/null || log_error "mismatch ksk in $STATE_FILE"
		grep "ZSK: ${_zsk}" "$STATE_FILE" > /dev/null || log_error "mismatch zsk in $STATE_FILE"

		# Check key states.
		if [ "$_goal" = "none" ]; then
			grep "GoalState: " "$STATE_FILE" > /dev/null && log_error "unexpected goal state in $STATE_FILE"
		else
			grep "GoalState: ${_goal}" "$STATE_FILE" > /dev/null || log_error "mismatch goal state in $STATE_FILE"
		fi

		if [ "$_state_dnskey" = "none" ]; then
			grep "DNSKEYState: " "$STATE_FILE" > /dev/null && log_error "unexpected dnskey state in $STATE_FILE"
			grep "DNSKEYChange: " "$STATE_FILE" > /dev/null && log_error "unexpected dnskey change in $STATE_FILE"
		else
			grep "DNSKEYState: ${_state_dnskey}" "$STATE_FILE" > /dev/null || log_error "mismatch dnskey state in $STATE_FILE"
			grep "DNSKEYChange: " "$STATE_FILE" > /dev/null || log_error "mismatch dnskey change in $STATE_FILE"
		fi

		if [ "$_state_zrrsig" = "none" ]; then
			grep "ZRRSIGState: " "$STATE_FILE" > /dev/null && log_error "unexpected zrrsig state in $STATE_FILE"
			grep "ZRRSIGChange: " "$STATE_FILE" > /dev/null && log_error "unexpected zrrsig change in $STATE_FILE"
		else
			grep "ZRRSIGState: ${_state_zrrsig}" "$STATE_FILE" > /dev/null || log_error "mismatch zrrsig state in $STATE_FILE"
			grep "ZRRSIGChange: " "$STATE_FILE" > /dev/null || log_error "mismatch zrrsig change in $STATE_FILE"
		fi

		if [ "$_state_krrsig" = "none" ]; then
			grep "KRRSIGState: " "$STATE_FILE" > /dev/null && log_error "unexpected krrsig state in $STATE_FILE"
			grep "KRRSIGChange: " "$STATE_FILE" > /dev/null && log_error "unexpected krrsig change in $STATE_FILE"
		else
			grep "KRRSIGState: ${_state_krrsig}" "$STATE_FILE" > /dev/null || log_error "mismatch krrsig state in $STATE_FILE"
			grep "KRRSIGChange: " "$STATE_FILE" > /dev/null || log_error "mismatch krrsig change in $STATE_FILE"
		fi

		if [ "$_state_ds" = "none" ]; then
			grep "DSState: " "$STATE_FILE" > /dev/null && log_error "unexpected ds state in $STATE_FILE"
			grep "DSChange: " "$STATE_FILE" > /dev/null && log_error "unexpected ds change in $STATE_FILE"
		else
			grep "DSState: ${_state_ds}" "$STATE_FILE" > /dev/null || log_error "mismatch ds state in $STATE_FILE"
			grep "DSChange: " "$STATE_FILE" > /dev/null || log_error "mismatch ds change in $STATE_FILE"
		fi
	fi

	# Check timing metadata.
	if [ "$_published" = "none" ]; then
		grep "; Publish:" "$KEY_FILE" > /dev/null && log_error "unexpected publish comment in $KEY_FILE"
		grep "Publish:" "$PRIVATE_FILE" > /dev/null && log_error "unexpected publish in $PRIVATE_FILE"
		if [ "$_legacy" == "no" ]; then
			grep "Published: " "$STATE_FILE" > /dev/null && log_error "unexpected publish in $STATE_FILE"
		fi
	else
		grep "; Publish:" "$KEY_FILE" > /dev/null || log_error "mismatch publish comment in $KEY_FILE"
		grep "Publish:" "$PRIVATE_FILE" > /dev/null || log_error "mismatch publish in $PRIVATE_FILE"
		if [ "$_legacy" == "no" ]; then
			grep "Published:" "$STATE_FILE" > /dev/null || log_error "mismatch publish in $STATE_FILE"
		fi
	fi

	if [ "$_active" = "none" ]; then
		grep "; Activate:" "$KEY_FILE" > /dev/null && log_error "unexpected active comment in $KEY_FILE"
		grep "Activate:" "$PRIVATE_FILE" > /dev/null && log_error "unexpected active in $PRIVATE_FILE"
		if [ "$_legacy" == "no" ]; then
			grep "Active: " "$STATE_FILE" > /dev/null && log_error "unexpected active in $STATE_FILE"
		fi
	else
		grep "; Activate:" "$KEY_FILE" > /dev/null || log_error "mismatch active comment in $KEY_FILE"
		grep "Activate:" "$PRIVATE_FILE" > /dev/null || log_error "mismatch active in $PRIVATE_FILE"
		if [ "$_legacy" == "no" ]; then
			grep "Active: " "$STATE_FILE" > /dev/null || log_error "mismatch active in $STATE_FILE"
		fi
	fi

	if [ "$_retired" = "none" ]; then
		grep "; Inactive:" "$KEY_FILE" > /dev/null && log_error "unexpected retired comment in $KEY_FILE"
		grep "Inactive:" "$PRIVATE_FILE" > /dev/null && log_error "unexpected retired in $PRIVATE_FILE"
		if [ "$_legacy" == "no" ]; then
			grep "Retired: " "$STATE_FILE" > /dev/null && log_error "unexpected retired in $STATE_FILE"
		fi
	else
		grep "; Inactive:" "$KEY_FILE" > /dev/null || log_error "mismatch retired comment in $KEY_FILE"
		grep "Inactive:" "$PRIVATE_FILE" > /dev/null || log_error "mismatch retired in $PRIVATE_FILE"
		if [ "$_legacy" == "no" ]; then
			grep "Retired: " "$STATE_FILE" > /dev/null || log_error "mismatch retired in $STATE_FILE"
		fi
	fi

	if [ "$_revoked" = "none" ]; then
		grep "; Revoke:" "$KEY_FILE" > /dev/null && log_error "unexpected revoked comment in $KEY_FILE"
		grep "Revoke:" "$PRIVATE_FILE" > /dev/null && log_error "unexpected revoked in $PRIVATE_FILE"
		if [ "$_legacy" == "no" ]; then
			grep "Revoked: " "$STATE_FILE" > /dev/null && log_error "unexpected revoked in $STATE_FILE"
		fi
	else
		grep "; Revoke:" "$KEY_FILE" > /dev/null || log_error "mismatch revoked comment in $KEY_FILE"
		grep "Revoke:" "$PRIVATE_FILE" > /dev/null || log_error "mismatch revoked in $PRIVATE_FILE"
		if [ "$_legacy" == "no" ]; then
			grep "Revoked: " "$STATE_FILE" > /dev/null || log_error "mismatch revoked in $STATE_FILE"
		fi
	fi

	if [ "$_removed" = "none" ]; then
		grep "; Delete:" "$KEY_FILE" > /dev/null && log_error "unexpected removed comment in $KEY_FILE"
		grep "Delete:" "$PRIVATE_FILE" > /dev/null && log_error "unexpected removed in $PRIVATE_FILE"
		if [ "$_legacy" == "no" ]; then
			grep "Removed: " "$STATE_FILE" > /dev/null && log_error "unexpected removed in $STATE_FILE"
		fi
	else
		grep "; Delete:" "$KEY_FILE" > /dev/null || log_error "mismatch removed comment in $KEY_FILE"
		grep "Delete:" "$PRIVATE_FILE" > /dev/null || log_error "mismatch removed in $PRIVATE_FILE"
		if [ "$_legacy" == "no" ]; then
			grep "Removed: " "$STATE_FILE" > /dev/null || log_error "mismatch removed in $STATE_FILE"
		fi
	fi

	grep "; Created:" "$KEY_FILE" > /dev/null || log_error "mismatch created comment in $KEY_FILE"
	grep "Created:" "$PRIVATE_FILE" > /dev/null || log_error "mismatch created in $PRIVATE_FILE"
	if [ "$_legacy" == "no" ]; then
		grep "Generated: " "$STATE_FILE" > /dev/null || log_error "mismatch generated in $STATE_FILE"
	fi
}

# Check the key with key id $1 and see if it is unused.
# This requires environment variables to be set.
#
# This will set the following environment variables for testing:
# BASE_FILE="${_dir}/K${_zone}.+${_alg_numpad}+${_key_idpad}"
# KEY_FILE="${BASE_FILE}.key"
# PRIVATE_FILE="${BASE_FILE}.private"
# STATE_FILE="${BASE_FILE}.state"
# KEY_ID=$(echo $1 | sed 's/^0\{0,4\}//')
key_unused() {
	_dir=$DIR
	_zone=$ZONE
	_key_idpad=$1
	_key_id=$(echo "$_key_idpad" | sed 's/^0\{0,4\}//')
	_alg_num=$2
        _alg_numpad=$(printf "%03d" "$_alg_num")

	BASE_FILE="${_dir}/K${_zone}.+${_alg_numpad}+${_key_idpad}"
	KEY_FILE="${BASE_FILE}.key"
	PRIVATE_FILE="${BASE_FILE}.private"
	STATE_FILE="${BASE_FILE}.state"
	KEY_ID="${_key_id}"

	test $_log -eq 1 && echo_i "key unused $KEY_ID?"

	# Check file existence.
	[ -s "$KEY_FILE" ] || ret=1
	[ -s "$PRIVATE_FILE" ] || ret=1
	[ -s "$STATE_FILE" ] || ret=1
	[ "$ret" -eq 0 ] || return

	# Check timing metadata.
	grep "; Publish:" "$KEY_FILE" > /dev/null && log_error "unexpected publish comment in $KEY_FILE"
	grep "; Activate:" "$KEY_FILE" > /dev/null && log_error "unexpected active comment in $KEY_FILE"
	grep "; Inactive:" "$KEY_FILE" > /dev/null && log_error "unexpected retired comment in $KEY_FILE"
	grep "; Revoke:" "$KEY_FILE" > /dev/null && log_error "unexpected revoked comment in $KEY_FILE"
	grep "; Delete:" "$KEY_FILE" > /dev/null && log_error "unexpected removed comment in $KEY_FILE"

	grep "Publish:" "$PRIVATE_FILE" > /dev/null && log_error "unexpected publish in $PRIVATE_FILE"
	grep "Activate:" "$PRIVATE_FILE" > /dev/null && log_error "unexpected active in $PRIVATE_FILE"
	grep "Inactive:" "$PRIVATE_FILE" > /dev/null && log_error "unexpected retired in $PRIVATE_FILE"
	grep "Revoke:" "$PRIVATE_FILE" > /dev/null && log_error "unexpected revoked in $PRIVATE_FILE"
	grep "Delete:" "$PRIVATE_FILE" > /dev/null && log_error "unexpected removed in $PRIVATE_FILE"

	if [ "$_legacy" == "no" ]; then
		grep "Published: " "$STATE_FILE" > /dev/null && log_error "unexpected publish in $STATE_FILE"
		grep "Active: " "$STATE_FILE" > /dev/null && log_error "unexpected active in $STATE_FILE"
		grep "Retired: " "$STATE_FILE" > /dev/null && log_error "unexpected retired in $STATE_FILE"
		grep "Revoked: " "$STATE_FILE" > /dev/null && log_error "unexpected revoked in $STATE_FILE"
		grep "Removed: " "$STATE_FILE" > /dev/null && log_error "unexpected removed in $STATE_FILE"
	fi
}

# Test: dnssec-verify zone $1.
dnssec_verify()
{
	n=$((n+1))
	echo_i "dnssec-verify zone ${ZONE} ($n)"
	ret=0
	dig_with_opts "$ZONE" "@${SERVER}" AXFR > dig.out.axfr.test$n || log_error "dig ${ZONE} AXFR failed"
	$VERIFY -z -o "$ZONE" dig.out.axfr.test$n > /dev/null || log_error "dnssec verify zone $ZONE failed"
	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}

# Wait for the zone to be signed.
# The apex NSEC record indicates that it is signed.
_wait_for_nsec() {
	dig_with_opts "@${SERVER}" -y "$TSIG" "$ZONE" NSEC > "dig.out.nsec.test$n" || return 1
	grep "NS SOA" "dig.out.nsec.test$n" > /dev/null || return 1
	grep "${ZONE}\..*IN.*RRSIG" "dig.out.nsec.test$n" > /dev/null || return 1
	return 0
}

wait_for_nsec() {
	n=$((n+1))
	ret=0
	echo_i "wait for ${ZONE} to be signed ($n)"
	retry_quiet 10 _wait_for_nsec  || log_error "wait for ${ZONE} to be signed failed"
	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}

# Default next key event threshold. May be extended by wait periods.
next_key_event_threshold=100

###############################################################################
# Tests                                                                       #
###############################################################################

#
# dnssec-keygen
#
set_zone "kasp"
set_policy "kasp" "4" "200"
set_server "keys" "10.53.0.1"

n=$((n+1))
echo_i "check that 'dnssec-keygen -k' (configured policy) creates valid files ($n)"
ret=0
$KEYGEN -K keys -k "$POLICY" -l kasp.conf "$ZONE" > "keygen.out.$POLICY.test$n" 2>/dev/null || ret=1
lines=$(wc -l < "keygen.out.$POLICY.test$n")
test "$lines" -eq $NUM_KEYS || log_error "wrong number of keys created for policy kasp: $lines"
# Temporarily don't log errors because we are searching multiple files.
_log=0

# Key properties.
set_keyrole      "KEY1" "csk"
set_keylifetime  "KEY1" "31536000"
set_keyalgorithm "KEY1" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "yes"

set_keyrole      "KEY2" "ksk"
set_keylifetime  "KEY2" "31536000"
set_keyalgorithm "KEY2" "8" "RSASHA256" "2048"
set_keysigning   "KEY2" "yes"
set_zonesigning  "KEY2" "no"

set_keyrole      "KEY3" "zsk"
set_keylifetime  "KEY3" "2592000"
set_keyalgorithm "KEY3" "8" "RSASHA256" "1024"
set_keysigning   "KEY3" "no"
set_zonesigning  "KEY3" "yes"

set_keyrole      "KEY4" "zsk"
set_keylifetime  "KEY4" "16070400"
set_keyalgorithm "KEY4" "8" "RSASHA256" "2000"
set_keysigning   "KEY4" "no"
set_zonesigning  "KEY4" "yes"

lines=$(get_keyids "$DIR" "$ZONE" | wc -l)
test "$lines" -eq $NUM_KEYS || log_error "bad number of key ids"

ids=$(get_keyids "$DIR" "$ZONE")
for id in $ids; do
	# There are four key files with the same algorithm.
	# Check them until a match is found.
	ret=0 && check_key "KEY1" "$id"
	test "$ret" -eq 0 && continue

	ret=0 && check_key "KEY2" "$id"
	test "$ret" -eq 0 && continue

	ret=0 && check_key "KEY3" "$id"
	test "$ret" -eq 0 && continue

	ret=0 && check_key "KEY4" "$id"

	# If ret is still non-zero, non of the files matched.
	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
done
# Turn error logs on again.
_log=1

n=$((n+1))
echo_i "check that 'dnssec-keygen -k' (default policy) creates valid files ($n)"
ret=0
set_zone "kasp"
set_policy "default" "1" "3600"
set_server "." "10.53.0.1"
# Key properties.
set_keyrole      "KEY1" "csk"
set_keylifetime  "KEY1" "0"
set_keyalgorithm "KEY1" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "yes"

key_clear "KEY2"
key_clear "KEY3"
key_clear "KEY4"

$KEYGEN -k "$POLICY" "$ZONE" > "keygen.out.$POLICY.test$n" 2>/dev/null || ret=1
lines=$(wc -l < "keygen.out.$POLICY.test$n")
test "$lines" -eq $NUM_KEYS || log_error "wrong number of keys created for policy default: $lines"
ids=$(get_keyids "$DIR" "$ZONE")
for id in $ids; do
	check_key "KEY1" "$id"
done
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

#
# dnssec-settime
#

# These test builds upon the latest created key with dnssec-keygen and uses the
# environment variables BASE_FILE, KEY_FILE, PRIVATE_FILE and STATE_FILE.
CMP_FILE="${BASE_FILE}.cmp"
n=$((n+1))
echo_i "check that 'dnssec-settime' by default does not edit key state file ($n)"
ret=0
cp "$STATE_FILE" "$CMP_FILE"
$SETTIME -P +3600 "$BASE_FILE" > /dev/null || log_error "settime failed"
grep "; Publish: " "$KEY_FILE" > /dev/null || log_error "mismatch published in $KEY_FILE"
grep "Publish: " "$PRIVATE_FILE" > /dev/null || log_error "mismatch published in $PRIVATE_FILE"
$DIFF "$CMP_FILE" "$STATE_FILE" || log_error "unexpected file change in $STATE_FILE"
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

n=$((n+1))
echo_i "check that 'dnssec-settime -s' also sets publish time metadata and states in key state file ($n)"
ret=0
cp "$STATE_FILE" "$CMP_FILE"
now=$(date +%Y%m%d%H%M%S)
$SETTIME -s -P "$now" -g "omnipresent" -k "rumoured" "$now" -z "omnipresent" "$now" -r "rumoured" "$now" -d "hidden" "$now" "$BASE_FILE" > /dev/null || log_error "settime failed"
set_keytime  "KEY1" "PUBLISHED"    "yes"
set_keystate "KEY1" "GOAL"         "omnipresent"
set_keystate "KEY1" "STATE_DNSKEY" "rumoured"
set_keystate "KEY1" "STATE_KRRSIG" "rumoured"
set_keystate "KEY1" "STATE_ZRRSIG" "omnipresent"
set_keystate "KEY1" "STATE_DS"     "hidden"
check_key "KEY1" "$id"
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

n=$((n+1))
echo_i "check that 'dnssec-settime -s' also unsets publish time metadata and states in key state file ($n)"
ret=0
cp "$STATE_FILE" "$CMP_FILE"
$SETTIME -s -P "none" -g "none" -k "none" "$now" -z "none" "$now" -r "none" "$now" -d "none" "$now" "$BASE_FILE" > /dev/null || log_error "settime failed"
set_keytime  "KEY1" "PUBLISHED"    "none"
set_keystate "KEY1" "GOAL"         "none"
set_keystate "KEY1" "STATE_DNSKEY" "none"
set_keystate "KEY1" "STATE_KRRSIG" "none"
set_keystate "KEY1" "STATE_ZRRSIG" "none"
set_keystate "KEY1" "STATE_DS"     "none"
check_key "KEY1" "$id"
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

n=$((n+1))
echo_i "check that 'dnssec-settime -s' also sets active time metadata and states in key state file (uppercase) ($n)"
ret=0
cp "$STATE_FILE" "$CMP_FILE"
now=$(date +%Y%m%d%H%M%S)
$SETTIME -s -A "$now" -g "HIDDEN" -k "UNRETENTIVE" "$now" -z "UNRETENTIVE" "$now" -r "OMNIPRESENT" "$now" -d "OMNIPRESENT" "$now" "$BASE_FILE" > /dev/null || log_error "settime failed"
set_keytime  "KEY1" "ACTIVE"       "yes"
set_keystate "KEY1" "GOAL"         "hidden"
set_keystate "KEY1" "STATE_DNSKEY" "unretentive"
set_keystate "KEY1" "STATE_KRRSIG" "omnipresent"
set_keystate "KEY1" "STATE_ZRRSIG" "unretentive"
set_keystate "KEY1" "STATE_DS"     "omnipresent"
check_key "KEY1" "$id"
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

#
# named
#
#
#  The NSEC record at the apex of the zone and its RRSIG records are
#  added as part of the last step in signing a zone.  We wait for the
#  NSEC records to appear before proceeding with a counter to prevent
#  infinite loops if there is a error.
#
n=$((n+1))
echo_i "waiting for kasp signing changes to take effect ($n)"
i=0
while [ $i -lt 30 ]
do
	ret=0
	while read -r zone
	do
		dig_with_opts "$zone" @10.53.0.3 nsec > "dig.out.ns3.test$n.$zone" || ret=1
		grep "NS SOA" "dig.out.ns3.test$n.$zone" > /dev/null || ret=1
		grep "$zone\..*IN.*RRSIG" "dig.out.ns3.test$n.$zone" > /dev/null || ret=1
	done < ns3/zones

	while read -r zone
	do
		dig_with_opts "$zone" @10.53.0.6 nsec > "dig.out.ns6.test$n.$zone" || ret=1
		grep "NS SOA" "dig.out.ns6.test$n.$zone" > /dev/null || ret=1
		grep "$zone\..*IN.*RRSIG" "dig.out.ns6.test$n.$zone" > /dev/null || ret=1
	done < ns6/zones

	i=$((i+1))
	if [ $ret = 0 ]; then break; fi
	echo_i "waiting ... ($i)"
	sleep 1
done
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

next_key_event_threshold=$((next_key_event_threshold+i))

# Check keys for a configured zone. This verifies:
# 1. The right number of keys exist in the key pool ($1).
# 2. The right number of keys is active. Checks KEY1, KEY2, KEY3, and KEY4.
#
# It is expected that KEY1, KEY2, KEY3, and KEY4 arrays are set correctly.
# Found key identifiers are stored in the right key array.
check_keys()
{
	n=$((n+1))
	echo_i "check keys are created for zone ${ZONE} ($n)"
	ret=0

	n=$((n+1))
	echo_i "check number of keys for zone ${ZONE} in dir ${DIR} ($n)"
	ret=0
	_numkeys=$(get_keyids "$DIR" "$ZONE" | wc -l)
	test "$_numkeys" -eq "$NUM_KEYS" || log_error "bad number ($_numkeys) of key files for zone $ZONE (expected $NUM_KEYS)"
	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))

	# Temporarily don't log errors because we are searching multiple files.
	_log=0

	# Clear key ids.
	key_set KEY1 ID "no"
	key_set KEY2 ID "no"
	key_set KEY3 ID "no"
	key_set KEY4 ID "no"

	# Check key files.
	_ids=$(get_keyids "$DIR" "$ZONE")
	for _id in $_ids; do
		# There are three key files with the same algorithm.
		# Check them until a match is found.
		echo_i "check key id $_id"

		if [ "no" = "$(key_get KEY1 ID)" ] && [ "$(key_get KEY1 EXPECT)" = "yes" ]; then
			ret=0
			check_key "KEY1" "$_id"
			test "$ret" -eq 0 && key_set KEY1 "ID" "$KEY_ID" && continue
		fi
		if [ "no" = "$(key_get KEY2 ID)" ] && [ "$(key_get KEY2 EXPECT)" = "yes" ]; then
			ret=0
			check_key "KEY2" "$_id"
			test "$ret" -eq 0 && key_set KEY2 "ID" "$KEY_ID" && continue
		fi
		if [ "no" = "$(key_get KEY3 ID)" ] && [ "$(key_get KEY3 EXPECT)" = "yes"  ]; then
			ret=0
			check_key "KEY3" "$_id"
			test "$ret" -eq 0 && key_set KEY3 "ID" "$KEY_ID" && continue
		fi
		if [ "no" = "$(key_get KEY4 ID)" ] && [ "$(key_get KEY4 EXPECT)" = "yes"  ]; then
			ret=0
			check_key "KEY4" "$_id"
			test "$ret" -eq 0 && key_set KEY4 "ID" "$KEY_ID" && continue
		fi

		# This may be an unused key. Assume algorithm of KEY1.
		ret=0 && key_unused "$_id" "$(key_get KEY1 ALG_NUM)"
		test "$ret" -eq 0 && continue

		# If ret is still non-zero, none of the files matched.
		test "$ret" -eq 0 || echo_i "failed"
		status=$((status+1))
	done

	# Turn error logs on again.
	_log=1

	ret=0
	if [ "$(key_get KEY1 EXPECT)" = "yes" ]; then
		echo_i "KEY1 ID $(key_get KEY1 ID)"
		test "no" = "$(key_get KEY1 ID)" && log_error "No KEY1 found for zone ${ZONE}"
	fi
	if [ "$(key_get KEY2 EXPECT)" = "yes" ]; then
		echo_i "KEY2 ID $(key_get KEY2 ID)"
		test "no" = "$(key_get KEY2 ID)" && log_error "No KEY2 found for zone ${ZONE}"
	fi
	if [ "$(key_get KEY3 EXPECT)" = "yes" ]; then
		echo_i "KEY3 ID $(key_get KEY3 ID)"
		test "no" = "$(key_get KEY3 ID)" && log_error "No KEY3 found for zone ${ZONE}"
	fi
	if [ "$(key_get KEY4 EXPECT)" = "yes" ]; then
		echo_i "KEY4 ID $(key_get KEY4 ID)"
		test "no" = "$(key_get KEY4 ID)" && log_error "No KEY4 found for zone ${ZONE}"
	fi
	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}

# Check if RRset of type $1 in file $2 is signed with the right keys.
# The right keys are the ones that expect a signature and matches the role $3.
check_signatures() {
	_qtype=$1
	_file=$2
	_role=$3

	if [ "$_role" = "KSK" ]; then
		_expect_type=EXPECT_KRRSIG
	elif [ "$_role" = "ZSK" ]; then
		_expect_type=EXPECT_ZRRSIG
	fi

	if [ "$(key_get KEY1 "$_expect_type")" = "yes" ] && [ "$(key_get KEY1 "$_role")" = "yes" ]; then
		get_keys_which_signed "$_qtype" "$_file" | grep "^$(key_get KEY1 ID)$" > /dev/null || log_error "${_qtype} RRset not signed with key $(key_get KEY1 ID)"
	elif [ "$(key_get KEY1 EXPECT)" = "yes" ]; then
		get_keys_which_signed "$_qtype" "$_file" | grep "^$(key_get KEY1 ID)$" > /dev/null && log_error "${_qtype} RRset signed unexpectedly with key $(key_get KEY1 ID)"
	fi

	if [ "$(key_get KEY2 "$_expect_type")" = "yes" ] && [ "$(key_get KEY2 "$_role")" = "yes" ]; then
		get_keys_which_signed "$_qtype" "$_file" | grep "^$(key_get KEY2 ID)$" > /dev/null || log_error "${_qtype} RRset not signed with key $(key_get KEY2 ID)"
	elif [ "$(key_get KEY2 EXPECT)" = "yes" ]; then
		get_keys_which_signed "$_qtype" "$_file" | grep "^$(key_get KEY2 ID)$" > /dev/null && log_error "${_qtype} RRset signed unexpectedly with key $(key_get KEY2 ID)"
	fi

	if [ "$(key_get KEY3 "$_expect_type")" = "yes" ] && [ "$(key_get KEY3 "$_role")" = "yes" ]; then
		get_keys_which_signed "$_qtype" "$_file" | grep "^$(key_get KEY3 ID)$" > /dev/null || log_error "${_qtype} RRset not signed with key $(key_get KEY3 ID)"
	elif [ "$(key_get KEY3 EXPECT)" = "yes" ]; then
		get_keys_which_signed "$_qtype" "$_file" | grep "^$(key_get KEY3 ID)$" > /dev/null && log_error "${_qtype} RRset signed unexpectedly with key $(key_get KEY3 ID)"
	fi

	if [ "$(key_get KEY4 "$_expect_type")" = "yes" ] && [ "$(key_get KEY4 "$_role")" = "yes" ]; then
		get_keys_which_signed "$_qtype" "$_file" | grep "^$(key_get KEY4 ID)$" > /dev/null || log_error "${_qtype} RRset not signed with key $(key_get KEY4 ID)"
	elif [ "$(key_get KEY4 EXPECT)" = "yes" ]; then
		get_keys_which_signed "$_qtype" "$_file" | grep "^$(key_get KEY4 ID)$" > /dev/null && log_error "${_qtype} RRset signed unexpectedly with key $(key_get KEY4 ID)"
	fi
}

response_has_cds_for_key() (
	awk -v zone="${ZONE%%.}." \
	    -v ttl="${DNSKEY_TTL}" \
	    -v qtype="CDS" \
	    -v keyid="$(key_get "${1}" ID)" \
	    -v keyalg="$(key_get "${1}" ALG_NUM)" \
	    -v hashalg="2" \
	    'BEGIN { ret=1; }
	     $1 == zone && $2 == ttl && $4 == qtype && $5 == keyid && $6 == keyalg && $7 == hashalg { ret=0; exit; }
	     END { exit ret; }' \
	    "$2"
)

response_has_cdnskey_for_key() (
	awk -v zone="${ZONE%%.}." \
	    -v ttl="${DNSKEY_TTL}" \
	    -v qtype="CDNSKEY" \
	    -v flags="257" \
	    -v keyalg="$(key_get "${1}" ALG_NUM)" \
	    'BEGIN { ret=1; }
	     $1 == zone && $2 == ttl && $4 == qtype && $5 == flags && $7 == keyalg { ret=0; exit; }
	     END { exit ret; }' \
	    "$2"
)

# Test CDS and CDNSKEY publication.
check_cds() {

	n=$((n+1))
	echo_i "check CDS and CDNSKEY rrset are signed correctly for zone ${ZONE} ($n)"
	ret=0

	dig_with_opts "$ZONE" "@${SERVER}" "CDS" > "dig.out.$DIR.test$n.cds" || log_error "dig ${ZONE} CDS failed"
	grep "status: NOERROR" "dig.out.$DIR.test$n.cds" > /dev/null || log_error "mismatch status in DNS response"

	dig_with_opts "$ZONE" "@${SERVER}" "CDNSKEY" > "dig.out.$DIR.test$n.cdnskey" || log_error "dig ${ZONE} CDNSKEY failed"
	grep "status: NOERROR" "dig.out.$DIR.test$n.cdnskey" > /dev/null || log_error "mismatch status in DNS response"

	if [ "$(key_get KEY1 STATE_DS)" = "rumoured" ] || [ "$(key_get KEY1 STATE_DS)" = "omnipresent" ]; then
		response_has_cds_for_key KEY1 "dig.out.$DIR.test$n.cds" || log_error "missing CDS record in response for key $(key_get KEY1 ID)"
		check_signatures "CDS" "dig.out.$DIR.test$n.cds" "KSK"
		response_has_cdnskey_for_key KEY1 "dig.out.$DIR.test$n.cdnskey" || log_error "missing CDNSKEY record in response for key $(key_get KEY1 ID)"
		check_signatures "CDNSKEY" "dig.out.$DIR.test$n.cdnskey" "KSK"
	elif [ "$(key_get KEY1 EXPECT)" = "yes" ]; then
		response_has_cds_for_key KEY1 "dig.out.$DIR.test$n.cds" && log_error "unexpected CDS record in response for key $(key_get KEY1 ID)"
		# KEY1 should not have an associated CDNSKEY, but there may be
		# one for another key.  Since the CDNSKEY has no field for key
		# id, it is hard to check what key the CDNSKEY may belong to
		# so let's skip this check for now.
	fi

	if [ "$(key_get KEY2 STATE_DS)" = "rumoured" ] || [ "$(key_get KEY2 STATE_DS)" = "omnipresent" ]; then
		response_has_cds_for_key KEY2 "dig.out.$DIR.test$n.cds" || log_error "missing CDS record in response for key $(key_get KEY2 ID)"
		check_signatures "CDS" "dig.out.$DIR.test$n.cds" "KSK"
		response_has_cdnskey_for_key KEY2 "dig.out.$DIR.test$n.cdnskey" || log_error "missing CDNSKEY record in response for key $(key_get KEY2 ID)"
		check_signatures "CDNSKEY" "dig.out.$DIR.test$n.cdnskey" "KSK"
	elif [ "$(key_get KEY2 EXPECT)" = "yes" ]; then
		response_has_cds_for_key KEY2 "dig.out.$DIR.test$n.cds" && log_error "unexpected CDS record in response for key $(key_get KEY2 ID)"
		# KEY2 should not have an associated CDNSKEY, but there may be
		# one for another key.  Since the CDNSKEY has no field for key
		# id, it is hard to check what key the CDNSKEY may belong to
		# so let's skip this check for now.
	fi

	if [ "$(key_get KEY3 STATE_DS)" = "rumoured" ] || [ "$(key_get KEY3 STATE_DS)" = "omnipresent" ]; then
		response_has_cds_for_key KEY3 "dig.out.$DIR.test$n.cds" || log_error "missing CDS record in response for key $(key_get KEY3 ID)"
		check_signatures "CDS" "dig.out.$DIR.test$n.cds" "KSK"
		response_has_cdnskey_for_key KEY3 "dig.out.$DIR.test$n.cdnskey" || log_error "missing CDNSKEY record in response for key $(key_get KEY3 ID)"
		check_signatures "CDNSKEY" "dig.out.$DIR.test$n.cdnskey" "KSK"
	elif [ "$(key_get KEY3 EXPECT)" = "yes" ]; then
		response_has_cds_for_key KEY3 "dig.out.$DIR.test$n.cds" && log_error "unexpected CDS record in response for key $(key_get KEY3 ID)"
		# KEY3 should not have an associated CDNSKEY, but there may be
		# one for another key.  Since the CDNSKEY has no field for key
		# id, it is hard to check what key the CDNSKEY may belong to
		# so let's skip this check for now.
	fi

	if [ "$(key_get KEY4 STATE_DS)" = "rumoured" ] || [ "$(key_get KEY4 STATE_DS)" = "omnipresent" ]; then
		response_has_cds_for_key KEY4 "dig.out.$DIR.test$n.cds" || log_error "missing CDS record in response for key $(key_get KEY4 ID)"
		check_signatures "CDS" "dig.out.$DIR.test$n.cds" "KSK"
		response_has_cdnskey_for_key KEY4 "dig.out.$DIR.test$n.cdnskey" || log_error "missing CDNSKEY record in response for key $(key_get KEY4 ID)"
		check_signatures "CDNSKEY" "dig.out.$DIR.test$n.cdnskey" "KSK"
	elif [ "$(key_get KEY4 EXPECT)" = "yes" ]; then
		response_has_cds_for_key KEY4 "dig.out.$DIR.test$n.cds" && log_error "unexpected CDS record in response for key $(key_get KEY4 ID)"
		# KEY4 should not have an associated CDNSKEY, but there may be
		# one for another key.  Since the CDNSKEY has no field for key
		# id, it is hard to check what key the CDNSKEY may belong to
		# so let's skip this check for now.
	fi

	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}

# Test the apex of a configured zone. This checks that the SOA and DNSKEY
# RRsets are signed correctly and with the appropriate keys.
check_apex() {
	# Test DNSKEY query.
	_qtype="DNSKEY"
	n=$((n+1))
	echo_i "check ${_qtype} rrset is signed correctly for zone ${ZONE} ($n)"
	ret=0
	dig_with_opts "$ZONE" "@${SERVER}" $_qtype > "dig.out.$DIR.test$n" || log_error "dig ${ZONE} ${_qtype} failed"
	grep "status: NOERROR" "dig.out.$DIR.test$n" > /dev/null || log_error "mismatch status in DNS response"


	if [ "$(key_get KEY1 STATE_DNSKEY)" = "rumoured" ] || [ "$(key_get KEY1 STATE_DNSKEY)" = "omnipresent" ]; then
		grep "${ZONE}\..*${DNSKEY_TTL}.*IN.*${_qtype}.*257.*.3.*$(key_get KEY1 ALG_NUM)" "dig.out.$DIR.test$n" > /dev/null || log_error "missing ${_qtype} record in response for key $(key_get KEY1 ID)"
		check_signatures $_qtype "dig.out.$DIR.test$n" "KSK"
		numkeys=$((numkeys+1))
	elif [ "$(key_get KEY1 EXPECT)" = "yes" ]; then
		grep "${ZONE}\.*${DNSKEY_TTL}.*IN.*${_qtype}.*257.*.3.*$(key_get KEY1 ALG_NUM)" "dig.out.$DIR.test$n" > /dev/null && log_error "unexpected ${_qtype} record in response for key $(key_get KEY1 ID)"
	fi

	if [ "$(key_get KEY2 STATE_DNSKEY)" = "rumoured" ] || [ "$(key_get KEY2 STATE_DNSKEY)" = "omnipresent" ]; then
		grep "${ZONE}\..*${DNSKEY_TTL}.*IN.*${_qtype}.*257.*.3.*$(key_get KEY2 ALG_NUM)" "dig.out.$DIR.test$n" > /dev/null || log_error "missing ${_qtype} record in response for key $(key_get KEY2 ID)"
		check_signatures $_qtype "dig.out.$DIR.test$n" "KSK"
		numkeys=$((numkeys+1))
	elif [ "$(key_get KEY2 EXPECT)" = "yes" ]; then
		grep "${ZONE}\.*${DNSKEY_TTL}.*IN.*${_qtype}.*257.*.3.*$(key_get KEY2 ALG_NUM)" "dig.out.$DIR.test$n" > /dev/null && log_error "unexpected ${_qtype} record in response for key $(key_get KEY2 ID)"
	fi

	if [ "$(key_get KEY3 STATE_DNSKEY)" = "rumoured" ] || [ "$(key_get KEY3 STATE_DNSKEY)" = "omnipresent" ]; then
		grep "${ZONE}\..*${DNSKEY_TTL}.*IN.*${_qtype}.*257.*.3.*$(key_get KEY3 ALG_NUM)" "dig.out.$DIR.test$n" > /dev/null || log_error "missing ${_qtype} record in response for key $(key_get KEY3 ID)"
		check_signatures $_qtype "dig.out.$DIR.test$n" "KSK"
		numkeys=$((numkeys+1))
	elif [ "$(key_get KEY3 EXPECT)" = "yes" ]; then
		grep "${ZONE}\..*${DNSKEY_TTL}.*IN.*${_qtype}.*257.*.3.*$(key_get KEY3 ALG_NUM)" "dig.out.$DIR.test$n" > /dev/null && log_error "unexpected ${_qtype} record in response for key $(key_get KEY3 ID)"
	fi

	if [ "$(key_get KEY4 STATE_DNSKEY)" = "rumoured" ] || [ "$(key_get KEY4 STATE_DNSKEY)" = "omnipresent" ]; then
		grep "${ZONE}\..*${DNSKEY_TTL}.*IN.*${_qtype}.*257.*.3.*$(key_get KEY4 ALG_NUM)" "dig.out.$DIR.test$n" > /dev/null || log_error "missing ${_qtype} record in response for key $(key_get KEY4 ID)"
		check_signatures $_qtype "dig.out.$DIR.test$n" "KSK"
		numkeys=$((numkeys+1))
	elif [ "$(key_get KEY4 EXPECT)" = "yes" ]; then
		grep "${ZONE}\..*${DNSKEY_TTL}.*IN.*${_qtype}.*257.*.3.*$(key_get KEY4 ALG_NUM)" "dig.out.$DIR.test$n" > /dev/null && log_error "unexpected ${_qtype} record in response for key $(key_get KEY4 ID)"
	fi

	lines=$(get_keys_which_signed $_qtype "dig.out.$DIR.test$n" | wc -l)
	check_signatures $_qtype "dig.out.$DIR.test$n" "KSK"
	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))

	# Test SOA query.
	_qtype="SOA"
	n=$((n+1))
	echo_i "check ${_qtype} rrset is signed correctly for zone ${ZONE} ($n)"
	ret=0
	dig_with_opts "$ZONE" "@${SERVER}" $_qtype > "dig.out.$DIR.test$n" || log_error "dig ${ZONE} ${_qtype} failed"
	grep "status: NOERROR" "dig.out.$DIR.test$n" > /dev/null || log_error "mismatch status in DNS response"
	grep "${ZONE}\..*${DEFAULT_TTL}.*IN.*${_qtype}.*" "dig.out.$DIR.test$n" > /dev/null || log_error "missing ${_qtype} record in response"
	lines=$(get_keys_which_signed $_qtype "dig.out.$DIR.test$n" | wc -l)
	check_signatures $_qtype "dig.out.$DIR.test$n" "ZSK"
	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))

	# Test CDS and CDNSKEY publication.
	check_cds
}

# Test an RRset below the apex and verify it is signed correctly.
check_subdomain() {
	_qtype="A"
	n=$((n+1))
	echo_i "check ${_qtype} a.${ZONE} rrset is signed correctly for zone ${ZONE} ($n)"
	ret=0
	dig_with_opts "a.$ZONE" "@${SERVER}" $_qtype > "dig.out.$DIR.test$n" || log_error "dig a.${ZONE} ${_qtype} failed"
	grep "status: NOERROR" "dig.out.$DIR.test$n" > /dev/null || log_error "mismatch status in DNS response"
	grep "a.${ZONE}\..*${DEFAULT_TTL}.*IN.*${_qtype}.*10\.0\.0\.1" "dig.out.$DIR.test$n" > /dev/null || log_error "missing a.${ZONE} ${_qtype} record in response"
	lines=$(get_keys_which_signed $_qtype "dig.out.$DIR.test$n" | wc -l)
	check_signatures $_qtype "dig.out.$DIR.test$n" "ZSK"
	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}

#
# Zone: default.kasp.
#

# Check the zone with default kasp policy has loaded and is signed.
set_zone "default.kasp"
set_policy "default" "1" "3600"
set_server "ns3" "10.53.0.3"
# Key properties.
set_keyrole      "KEY1" "csk"
set_keylifetime  "KEY1" "0"
set_keyalgorithm "KEY1" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "yes"

# The first key is immediately published and activated.
set_keytime  "KEY1" "PUBLISHED"    "yes"
set_keytime  "KEY1" "ACTIVE"       "yes"
# DNSKEY, RRSIG (ksk), RRSIG (zsk) are published. DS needs to wait.
set_keystate "KEY1" "GOAL"         "omnipresent"
set_keystate "KEY1" "STATE_DNSKEY" "rumoured"
set_keystate "KEY1" "STATE_KRRSIG" "rumoured"
set_keystate "KEY1" "STATE_ZRRSIG" "rumoured"
set_keystate "KEY1" "STATE_DS"     "hidden"

check_keys
check_apex
check_subdomain
dnssec_verify

# Update zone.
n=$((n+1))
echo_i "modify unsigned zone file and check that new record is signed for zone ${ZONE} ($n)"
ret=0
cp "${DIR}/template2.db.in" "${DIR}/${ZONE}.db"
rndccmd 10.53.0.3 reload "$ZONE" > /dev/null || log_error "rndc reload zone ${ZONE} failed"

update_is_signed() {
	dig_with_opts "a.${ZONE}" "@${SERVER}" A > "dig.out.$DIR.test$n.a" || return 1
	grep "status: NOERROR" "dig.out.$DIR.test$n.a" > /dev/null || return 1
	grep "a.${ZONE}\..*${DEFAULT_TTL}.*IN.*A.*10\.0\.0\.11" "dig.out.$DIR.test$n.a" > /dev/null || return 1
	lines=$(get_keys_which_signed A "dig.out.$DIR.test$n.a" | wc -l)
	test "$lines" -eq 1 || return 1
	get_keys_which_signed A "dig.out.$DIR.test$n.a" | grep "^${KEY_ID}$" > /dev/null || return 1

	dig_with_opts "d.${ZONE}" "@${SERVER}" A > "dig.out.$DIR.test$n".d || return 1
	grep "status: NOERROR" "dig.out.$DIR.test$n".d > /dev/null || return 1
	grep "d.${ZONE}\..*${DEFAULT_TTL}.*IN.*A.*10\.0\.0\.4" "dig.out.$DIR.test$n".d > /dev/null || return 1
	lines=$(get_keys_which_signed A "dig.out.$DIR.test$n".d | wc -l)
	test "$lines" -eq 1 || return 1
	get_keys_which_signed A "dig.out.$DIR.test$n".d | grep "^${KEY_ID}$" > /dev/null || return 1
}

retry_quiet 10 update_is_signed || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

#
# Zone: dynamic.kasp
#
set_zone "dynamic.kasp"
set_policy "default" "1" "3600"
set_server "ns3" "10.53.0.3"
# Key properties, timings and states same as above.
check_keys
check_apex
check_subdomain
dnssec_verify

# Update zone with nsupdate.
n=$((n+1))
echo_i "nsupdate zone and check that new record is signed for zone ${ZONE} ($n)"
ret=0
(
echo zone ${ZONE}
echo server 10.53.0.3 "$PORT"
echo update del "a.${ZONE}" 300 A 10.0.0.1
echo update add "a.${ZONE}" 300 A 10.0.0.11
echo update add "d.${ZONE}" 300 A 10.0.0.4
echo send
) | $NSUPDATE

retry_quiet 10 update_is_signed || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))


#
# Zone: dynamic-inline-signing.kasp
#
set_zone "dynamic-inline-signing.kasp"
set_policy "default" "1" "3600"
set_server "ns3" "10.53.0.3"
# Key properties, timings and states same as above.
check_keys
check_apex
check_subdomain
dnssec_verify

# Update zone with freeze/thaw.
n=$((n+1))
echo_i "modify unsigned zone file and check that new record is signed for zone ${ZONE} ($n)"
ret=0
rndccmd 10.53.0.3 freeze "$ZONE" > /dev/null || log_error "rndc freeze zone ${ZONE} failed"
sleep 1
cp "${DIR}/template2.db.in" "${DIR}/${ZONE}.db"
rndccmd 10.53.0.3 thaw "$ZONE" > /dev/null || log_error "rndc thaw zone ${ZONE} failed"

retry_quiet 10 update_is_signed || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

#
# Zone: inline-signing.kasp
#
set_zone "inline-signing.kasp"
set_policy "default" "1" "3600"
set_server "ns3" "10.53.0.3"
# Key properties, timings and states same as above.
check_keys
check_apex
check_subdomain
dnssec_verify

#
# Zone: rsasha1.kasp.
#
set_zone "rsasha1.kasp"
set_policy "rsasha1" "3" "1234"
set_server "ns3" "10.53.0.3"
# Key properties.
key_clear        "KEY1"
set_keyrole      "KEY1" "ksk"
set_keylifetime  "KEY1" "315360000"
set_keyalgorithm "KEY1" "5" "RSASHA1" "2048"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "no"

key_clear        "KEY2"
set_keyrole      "KEY2" "zsk"
set_keylifetime  "KEY2" "157680000"
set_keyalgorithm "KEY2" "5" "RSASHA1" "2048"
set_keysigning   "KEY2" "no"
set_zonesigning  "KEY2" "yes"

key_clear        "KEY3"
set_keyrole      "KEY3" "zsk"
set_keylifetime  "KEY3" "31536000"
set_keyalgorithm "KEY3" "5" "RSASHA1" "2000"
set_keysigning   "KEY3" "no"
set_zonesigning  "KEY3" "yes"
# The first keys are immediately published and activated.
# Because lifetime > 0, retired timing is also set.
set_keytime  "KEY1" "PUBLISHED"    "yes"
set_keytime  "KEY1" "ACTIVE"       "yes"
set_keytime  "KEY1" "RETIRED"      "yes"

set_keytime  "KEY2" "PUBLISHED"    "yes"
set_keytime  "KEY2" "ACTIVE"       "yes"
set_keytime  "KEY2" "RETIRED"      "yes"

set_keytime  "KEY3" "PUBLISHED"    "yes"
set_keytime  "KEY3" "ACTIVE"       "yes"
set_keytime  "KEY3" "RETIRED"      "yes"
# KSK: DNSKEY, RRSIG (ksk) published. DS needs to wait.
# ZSK: DNSKEY, RRSIG (zsk) published.
set_keystate "KEY1" "GOAL"         "omnipresent"
set_keystate "KEY1" "STATE_DNSKEY" "rumoured"
set_keystate "KEY1" "STATE_KRRSIG" "rumoured"
set_keystate "KEY1" "STATE_DS"     "hidden"

set_keystate "KEY2" "GOAL"         "omnipresent"
set_keystate "KEY2" "STATE_DNSKEY" "rumoured"
set_keystate "KEY2" "STATE_ZRRSIG" "rumoured"

set_keystate "KEY3" "GOAL"         "omnipresent"
set_keystate "KEY3" "STATE_DNSKEY" "rumoured"
set_keystate "KEY3" "STATE_ZRRSIG" "rumoured"
# Three keys only.
key_clear "KEY4"

check_keys
check_apex
check_subdomain
dnssec_verify

#
# Zone: unsigned.kasp.
#
set_zone "unsigned.kasp"
set_policy "none" "0" "0"
set_server "ns3" "10.53.0.3"

key_clear "KEY1"
key_clear "KEY2"
key_clear "KEY3"
key_clear "KEY4"

check_keys
check_apex
check_subdomain

#
# Zone: unlimited.kasp.
#
set_zone "unlimited.kasp"
set_policy "unlimited" "1" "1234"
set_server "ns3" "10.53.0.3"
# Key properties.
set_keyrole      "KEY1" "csk"
set_keylifetime  "KEY1" "0"
set_keyalgorithm "KEY1" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "yes"
# The first key is immediately published and activated.
set_keytime  "KEY1" "PUBLISHED"    "yes"
set_keytime  "KEY1" "ACTIVE"       "yes"
set_keytime  "KEY1" "RETIRED"      "none"
# DNSKEY, RRSIG (ksk), RRSIG (zsk) are published. DS needs to wait.
set_keystate "KEY1" "GOAL"         "omnipresent"
set_keystate "KEY1" "STATE_DNSKEY" "rumoured"
set_keystate "KEY1" "STATE_KRRSIG" "rumoured"
set_keystate "KEY1" "STATE_ZRRSIG" "rumoured"
set_keystate "KEY1" "STATE_DS"     "hidden"

check_keys
check_apex
check_subdomain
dnssec_verify

#
# Zone: inherit.kasp.
#
set_zone "inherit.kasp"
set_policy "rsasha1" "3" "1234"
set_server "ns3" "10.53.0.3"

# Key properties.
key_clear        "KEY1"
set_keyrole      "KEY1" "ksk"
set_keylifetime  "KEY1" "315360000"
set_keyalgorithm "KEY1" "5" "RSASHA1" "2048"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "no"

key_clear        "KEY2"
set_keyrole      "KEY2" "zsk"
set_keylifetime  "KEY2" "157680000"
set_keyalgorithm "KEY2" "5" "RSASHA1" "2048"
set_keysigning   "KEY2" "no"
set_zonesigning  "KEY2" "yes"

key_clear        "KEY3"
set_keyrole      "KEY3" "zsk"
set_keylifetime  "KEY3" "31536000"
set_keyalgorithm "KEY3" "5" "RSASHA1" "2000"
set_keysigning   "KEY3" "no"
set_zonesigning  "KEY3" "yes"
# The first keys are immediately published and activated.
# Because lifetime > 0, retired timing is also set.
set_keytime  "KEY1" "PUBLISHED"    "yes"
set_keytime  "KEY1" "ACTIVE"       "yes"
set_keytime  "KEY1" "RETIRED"      "yes"

set_keytime  "KEY2" "PUBLISHED"    "yes"
set_keytime  "KEY2" "ACTIVE"       "yes"
set_keytime  "KEY2" "RETIRED"      "yes"

set_keytime  "KEY3" "PUBLISHED"    "yes"
set_keytime  "KEY3" "ACTIVE"       "yes"
set_keytime  "KEY3" "RETIRED"      "yes"
# KSK: DNSKEY, RRSIG (ksk) published. DS needs to wait.
# ZSK: DNSKEY, RRSIG (zsk) published.
set_keystate "KEY1" "GOAL"         "omnipresent"
set_keystate "KEY1" "STATE_DNSKEY" "rumoured"
set_keystate "KEY1" "STATE_KRRSIG" "rumoured"
set_keystate "KEY1" "STATE_DS"     "hidden"

set_keystate "KEY2" "GOAL"         "omnipresent"
set_keystate "KEY2" "STATE_DNSKEY" "rumoured"
set_keystate "KEY2" "STATE_ZRRSIG" "rumoured"

set_keystate "KEY3" "GOAL"         "omnipresent"
set_keystate "KEY3" "STATE_DNSKEY" "rumoured"
set_keystate "KEY3" "STATE_ZRRSIG" "rumoured"
# Three keys only.
key_clear "KEY4"

check_keys
check_apex
check_subdomain
dnssec_verify

#
# Zone: dnssec-keygen.kasp.
#
set_zone "dnssec-keygen.kasp"
set_policy "rsasha1" "3" "1234"
set_server "ns3" "10.53.0.3"
# Key properties, timings and states same as above.

check_keys
check_apex
check_subdomain
dnssec_verify

#
# Zone: some-keys.kasp.
#
set_zone "some-keys.kasp"
set_policy "rsasha1" "3" "1234"
set_server "ns3" "10.53.0.3"
# Key properties, timings and states same as above.

check_keys
check_apex
check_subdomain
dnssec_verify

#
# Zone: legacy-keys.kasp.
#
set_zone "legacy-keys.kasp"
set_policy "rsasha1" "3" "1234"
set_server "ns3" "10.53.0.3"
# Key properties, timings and states same as above.

check_keys
check_apex
check_subdomain
dnssec_verify

#
# Zone: pregenerated.kasp.
#
# There are more pregenerated keys than needed, hence the number of keys is
# six, not three.
set_zone "pregenerated.kasp"
set_policy "rsasha1" "6" "1234"
set_server "ns3" "10.53.0.3"
# Key properties, timings and states same as above.

check_keys
check_apex
check_subdomain
dnssec_verify

#
# Zone: rumoured.kasp.
#
# There are three keys in rumoured state.
set_zone "rumoured.kasp"
set_policy "rsasha1" "3" "1234"
set_server "ns3" "10.53.0.3"
# Key properties, timings and states same as above.

check_keys
check_apex
check_subdomain
dnssec_verify

#
# Zone: secondary.kasp.
#
set_zone "secondary.kasp"
set_policy "rsasha1" "3" "1234"
set_server "ns3" "10.53.0.3"
# Key properties, timings and states same as above.

check_keys
check_apex
check_subdomain
dnssec_verify

# Update zone.
n=$((n+1))
echo_i "check that we correctly sign the zone after IXFR for zone ${ZONE} ($n)"
ret=0
cp ns2/secondary.kasp.db.in2 ns2/secondary.kasp.db
rndccmd 10.53.0.2 reload "$ZONE" > /dev/null || log_error "rndc reload zone ${ZONE} failed"
_log=0
i=0
while [ $i -lt 5 ]
do
	ret=0

	dig_with_opts "a.${ZONE}" "@${SERVER}" A > "dig.out.$DIR.test$n.a" || log_error "dig a.${ZONE} A failed"
	grep "status: NOERROR" "dig.out.$DIR.test$n.a" > /dev/null || log_error "mismatch status in DNS response"
	grep "a.${ZONE}\..*${DEFAULT_TTL}.*IN.*A.*10\.0\.0\.11" "dig.out.$DIR.test$n.a" > /dev/null || log_error "missing a.${ZONE} A record in response"
	check_signatures $_qtype "dig.out.$DIR.test$n.a" "ZSK"

	dig_with_opts "d.${ZONE}" "@${SERVER}" A > "dig.out.$DIR.test$n.d" || log_error "dig d.${ZONE} A failed"
	grep "status: NOERROR" "dig.out.$DIR.test$n.d" > /dev/null || log_error "mismatch status in DNS response"
	grep "d.${ZONE}\..*${DEFAULT_TTL}.*IN.*A.*10\.0\.0\.4" "dig.out.$DIR.test$n.d" > /dev/null || log_error "missing d.${ZONE} A record in response"
	lines=$(get_keys_which_signed A "dig.out.$DIR.test$n.d" | wc -l)
	check_signatures $_qtype "dig.out.$DIR.test$n.d" "ZSK"

	i=$((i+1))
	if [ $ret = 0 ]; then break; fi
	echo_i "waiting ... ($i)"
	sleep 1
done
_log=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

# TODO: we might want to test:
# - configuring a zone with too many active keys (should trigger retire).
# - configuring a zone with keys not matching the policy.

#
# Zone: rsasha1-nsec3.kasp.
#
set_zone "rsasha1-nsec3.kasp"
set_policy "rsasha1-nsec3" "3" "1234"
set_server "ns3" "10.53.0.3"
# Key properties.
set_keyalgorithm "KEY1" "7" "NSEC3RSASHA1" "2048"
set_keyalgorithm "KEY2" "7" "NSEC3RSASHA1" "2048"
set_keyalgorithm "KEY3" "7" "NSEC3RSASHA1" "2000"
# Key timings and states same as above.

check_keys
check_apex
check_subdomain
dnssec_verify

#
# Zone: rsasha256.kasp.
#
set_zone "rsasha256.kasp"
set_policy "rsasha256" "3" "1234"
set_server "ns3" "10.53.0.3"
# Key properties.
set_keyalgorithm "KEY1" "8" "RSASHA256" "2048"
set_keyalgorithm "KEY2" "8" "RSASHA256" "2048"
set_keyalgorithm "KEY3" "8" "RSASHA256" "2000"
# Key timings and states same as above.

check_keys
check_apex
check_subdomain
dnssec_verify

#
# Zone: rsasha512.kasp.
#
set_zone "rsasha512.kasp"
set_policy "rsasha512" "3" "1234"
set_server "ns3" "10.53.0.3"
# Key properties.
set_keyalgorithm "KEY1" "10" "RSASHA512" "2048"
set_keyalgorithm "KEY2" "10" "RSASHA512" "2048"
set_keyalgorithm "KEY3" "10" "RSASHA512" "2000"
# Key timings and states same as above.

check_keys
check_apex
check_subdomain
dnssec_verify

#
# Zone: ecdsa256.kasp.
#
set_zone "ecdsa256.kasp"
set_policy "ecdsa256" "3" "1234"
set_server "ns3" "10.53.0.3"
# Key properties.
set_keyalgorithm "KEY1" "13" "ECDSAP256SHA256" "256"
set_keyalgorithm "KEY2" "13" "ECDSAP256SHA256" "256"
set_keyalgorithm "KEY3" "13" "ECDSAP256SHA256" "256"
# Key timings and states same as above.

check_keys
check_apex
check_subdomain
dnssec_verify

#
# Zone: ecdsa512.kasp.
#
set_zone "ecdsa384.kasp"
set_policy "ecdsa384" "3" "1234"
set_server "ns3" "10.53.0.3"
# Key properties.
set_keyalgorithm "KEY1" "14" "ECDSAP384SHA384" "384"
set_keyalgorithm "KEY2" "14" "ECDSAP384SHA384" "384"
set_keyalgorithm "KEY3" "14" "ECDSAP384SHA384" "384"
# Key timings and states same as above.

check_keys
check_apex
check_subdomain
dnssec_verify

# TODO: ED25519 and ED448.

#
# Zone: expired-sigs.autosign.
#
set_zone "expired-sigs.autosign"
set_policy "autosign" "2" "300"
set_server "ns3" "10.53.0.3"
# Key properties.
key_clear        "KEY1"
set_keyrole      "KEY1" "ksk"
set_keylifetime  "KEY1" "63072000"
set_keyalgorithm "KEY1" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "no"

key_clear        "KEY2"
set_keyrole      "KEY2" "zsk"
set_keylifetime  "KEY2" "31536000"
set_keyalgorithm "KEY2" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY2" "no"
set_zonesigning  "KEY2" "yes"
# Key timings.
set_keytime  "KEY1" "PUBLISHED"    "yes"
set_keytime  "KEY1" "ACTIVE"       "yes"
set_keytime  "KEY1" "RETIRED"      "yes"

set_keytime  "KEY2" "PUBLISHED"    "yes"
set_keytime  "KEY2" "ACTIVE"       "yes"
set_keytime  "KEY2" "RETIRED"      "yes"
# Both KSK and ZSK stay OMNIPRESENT.
set_keystate "KEY1" "GOAL"         "omnipresent"
set_keystate "KEY1" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY1" "STATE_KRRSIG" "omnipresent"
set_keystate "KEY1" "STATE_DS"     "omnipresent"

set_keystate "KEY2" "GOAL"         "omnipresent"
set_keystate "KEY2" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY2" "STATE_ZRRSIG" "omnipresent"
# Expect only two keys.
key_clear "KEY3"
key_clear "KEY4"

check_keys
check_apex
check_subdomain
dnssec_verify

# Verify all signatures have been refreshed.
check_rrsig_refresh() {
	# Apex.
	_qtypes="DNSKEY SOA NS NSEC"
	for _qtype in $_qtypes
	do
		n=$((n+1))
		echo_i "check ${_qtype} rrsig is refreshed correctly for zone ${ZONE} ($n)"
		ret=0
		dig_with_opts "$ZONE" "@${SERVER}" "$_qtype" > "dig.out.$DIR.test$n" || log_error "dig ${ZONE} ${_qtype} failed"
		grep "status: NOERROR" "dig.out.$DIR.test$n" > /dev/null || log_error "mismatch status in DNS response"
		grep "${ZONE}\..*IN.*RRSIG.*${_qtype}.*${ZONE}" "dig.out.$DIR.test$n" > "rrsig.out.$ZONE.$_qtype" || log_error "missing RRSIG (${_qtype}) record in response"
		# If this exact RRSIG is also in the zone file it is not refreshed.
		_rrsig=$(cat "rrsig.out.$ZONE.$_qtype")
		grep "${_rrsig}" "${DIR}/${ZONE}.db" > /dev/null && log_error "RRSIG (${_qtype}) not refreshed in zone ${ZONE}"
		test "$ret" -eq 0 || echo_i "failed"
		status=$((status+ret))
	done

	# Below apex.
	_labels="a b c ns3"
	for _label in $_labels;
	do
		_qtypes="A NSEC"
		for _qtype in $_qtypes
		do
			n=$((n+1))
			echo_i "check ${_label} ${_qtype} rrsig is refreshed correctly for zone ${ZONE} ($n)"
			ret=0
			dig_with_opts "${_label}.${ZONE}" "@${SERVER}" "$_qtype" > "dig.out.$DIR.test$n" || log_error "dig ${_label}.${ZONE} ${_qtype} failed"
			grep "status: NOERROR" "dig.out.$DIR.test$n" > /dev/null || log_error "mismatch status in DNS response"
			grep "${ZONE}\..*IN.*RRSIG.*${_qtype}.*${ZONE}" "dig.out.$DIR.test$n" > "rrsig.out.$ZONE.$_qtype" || log_error "missing RRSIG (${_qtype}) record in response"
			_rrsig=$(cat "rrsig.out.$ZONE.$_qtype")
			grep "${_rrsig}" "${DIR}/${ZONE}.db" > /dev/null && log_error "RRSIG (${_qtype}) not refreshed in zone ${ZONE}"
			test "$ret" -eq 0 || echo_i "failed"
			status=$((status+ret))
		done
	done
}

check_rrsig_refresh

#
# Zone: fresh-sigs.autosign.
#
set_zone "fresh-sigs.autosign"
set_policy "autosign" "2" "300"
set_server "ns3" "10.53.0.3"
# Key properties, timings and states same as above.

check_keys
check_apex
check_subdomain
dnssec_verify

# Verify signature reuse.
check_rrsig_reuse() {
	# Apex.
	_qtypes="NS NSEC"
	for _qtype in $_qtypes
	do
		n=$((n+1))
		echo_i "check ${_qtype} rrsig is reused correctly for zone ${ZONE} ($n)"
		ret=0
		dig_with_opts "$ZONE" "@${SERVER}" "$_qtype" > "dig.out.$DIR.test$n" || log_error "dig ${ZONE} ${_qtype} failed"
		grep "status: NOERROR" "dig.out.$DIR.test$n" > /dev/null || log_error "mismatch status in DNS response"
		grep "${ZONE}\..*IN.*RRSIG.*${_qtype}.*${ZONE}" "dig.out.$DIR.test$n" > "rrsig.out.$ZONE.$_qtype" || log_error "missing RRSIG (${_qtype}) record in response"
		# If this exact RRSIG is also in the zone file it is not refreshed.
		_rrsig=$(awk '{print $5, $6, $7, $8, $9, $10, $11, $12, $13, $14;}' < "rrsig.out.$ZONE.$_qtype")
		grep "${_rrsig}" "${DIR}/${ZONE}.db" > /dev/null || log_error "RRSIG (${_qtype}) not reused in zone ${ZONE}"
		test "$ret" -eq 0 || echo_i "failed"
		status=$((status+ret))
	done

	# Below apex.
	_labels="a b c ns3"
	for _label in $_labels;
	do
		_qtypes="A NSEC"
		for _qtype in $_qtypes
		do
			n=$((n+1))
			echo_i "check ${_label} ${_qtype} rrsig is reused correctly for zone ${ZONE} ($n)"
			ret=0
			dig_with_opts "${_label}.${ZONE}" "@${SERVER}" "$_qtype" > "dig.out.$DIR.test$n" || log_error "dig ${_label}.${ZONE} ${_qtype} failed"
			grep "status: NOERROR" "dig.out.$DIR.test$n" > /dev/null || log_error "mismatch status in DNS response"
			grep "${ZONE}\..*IN.*RRSIG.*${_qtype}.*${ZONE}" "dig.out.$DIR.test$n" > "rrsig.out.$ZONE.$_qtype" || log_error "missing RRSIG (${_qtype}) record in response"
			_rrsig=$(awk '{print $5, $6, $7, $8, $9, $10, $11, $12, $13, $14;}' < "rrsig.out.$ZONE.$_qtype")
			grep "${_rrsig}" "${DIR}/${ZONE}.db" > /dev/null || log_error "RRSIG (${_qtype}) not reused in zone ${ZONE}"
			test "$ret" -eq 0 || echo_i "failed"
			status=$((status+ret))
		done
	done
}

check_rrsig_reuse

#
# Zone: unfresh-sigs.autosign.
#
set_zone "unfresh-sigs.autosign"
set_policy "autosign" "2" "300"
set_server "ns3" "10.53.0.3"
# Key properties, timings and states same as above.

check_keys
check_apex
check_subdomain
dnssec_verify
check_rrsig_refresh

#
# Zone: zsk-missing.autosign.
#
set_zone "zsk-missing.autosign"
set_policy "autosign" "2" "300"
set_server "ns3" "10.53.0.3"
# Key properties, timings and states same as above.
# TODO.

#
# Zone: zsk-retired.autosign.
#
set_zone "zsk-retired.autosign"
set_policy "autosign" "3" "300"
set_server "ns3" "10.53.0.3"
# The third key is not yet expected to be signing.
set_keyrole      "KEY3" "zsk"
set_keylifetime  "KEY3" "31536000"
set_keyalgorithm "KEY3" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY3" "no"
set_zonesigning  "KEY3" "no"
# Key timings.
set_keytime  "KEY3" "PUBLISHED"    "yes"
set_keytime  "KEY3" "ACTIVE"       "yes"
set_keytime  "KEY3" "RETIRED"      "yes"
# The ZSK goal is set to HIDDEN but records stay OMNIPRESENT until the new ZSK
# is active.
set_keystate "KEY2" "GOAL"         "hidden"
set_keystate "KEY2" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY2" "STATE_ZRRSIG" "omnipresent"
# A new ZSK should be introduced, so expect a key with goal OMNIPRESENT,
# the DNSKEY introduced (RUMOURED) and the signatures HIDDEN.
set_keystate "KEY3" "GOAL"         "omnipresent"
set_keystate "KEY3" "STATE_DNSKEY" "rumoured"
set_keystate "KEY3" "STATE_ZRRSIG" "hidden"

check_keys
check_apex
check_subdomain
dnssec_verify
check_rrsig_refresh

#
# Test dnssec-policy inheritance.
#

# These zones should be unsigned:
# ns2/unsigned.tld
# ns4/none.inherit.signed
# ns4/none.override.signed
# ns4/inherit.none.signed
# ns4/none.none.signed
# ns5/inherit.inherit.unsigned
# ns5/none.inherit.unsigned
# ns5/none.override.unsigned
# ns5/inherit.none.unsigned
# ns5/none.none.unsigned
key_clear "KEY1"
key_clear "KEY2"
key_clear "KEY3"
key_clear "KEY4"

set_zone "unsigned.tld"
set_policy "none" "0" "0"
set_server "ns2" "10.53.0.2"
TSIG=""
check_keys
check_apex
check_subdomain

set_zone "none.inherit.signed"
set_policy "none" "0" "0"
set_server "ns4" "10.53.0.4"
TSIG="hmac-sha1:sha1:$SHA1"
check_keys
check_apex
check_subdomain

set_zone "none.override.signed"
set_policy "none" "0" "0"
set_server "ns4" "10.53.0.4"
TSIG="hmac-sha224:sha224:$SHA224"
check_keys
check_apex
check_subdomain

set_zone "inherit.none.signed"
set_policy "none" "0" "0"
set_server "ns4" "10.53.0.4"
TSIG="hmac-sha256:sha256:$SHA256"
check_keys
check_apex
check_subdomain

set_zone "none.none.signed"
set_policy "none" "0" "0"
set_server "ns4" "10.53.0.4"
TSIG="hmac-sha256:sha256:$SHA256"
check_keys
check_apex
check_subdomain

set_zone "inherit.inherit.unsigned"
set_policy "none" "0" "0"
set_server "ns5" "10.53.0.5"
TSIG="hmac-sha1:sha1:$SHA1"
check_keys
check_apex
check_subdomain

set_zone "none.inherit.unsigned"
set_policy "none" "0" "0"
set_server "ns5" "10.53.0.5"
TSIG="hmac-sha1:sha1:$SHA1"
check_keys
check_apex
check_subdomain

set_zone "none.override.unsigned"
set_policy "none" "0" "0"
set_server "ns5" "10.53.0.5"
TSIG="hmac-sha224:sha224:$SHA224"
check_keys
check_apex
check_subdomain

set_zone "inherit.none.unsigned"
set_policy "none" "0" "0"
set_server "ns5" "10.53.0.5"
TSIG="hmac-sha256:sha256:$SHA256"
check_keys
check_apex
check_subdomain

set_zone "none.none.unsigned"
set_policy "none" "0" "0"
set_server "ns5" "10.53.0.5"
TSIG="hmac-sha256:sha256:$SHA256"
check_keys
check_apex
check_subdomain

# These zones should be signed with the default policy:
# ns2/signed.tld
# ns4/override.inherit.signed
# ns4/inherit.override.signed
# ns5/override.inherit.signed
# ns5/inherit.override.signed
set_keyrole      "KEY1" "csk"
set_keylifetime  "KEY1" "0"
set_keyalgorithm "KEY1" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "yes"

set_keytime  "KEY1" "PUBLISHED"    "yes"
set_keytime  "KEY1" "ACTIVE"       "yes"
set_keytime  "KEY1" "RETIRED"      "none"

set_keystate "KEY1" "GOAL"         "omnipresent"
set_keystate "KEY1" "STATE_DNSKEY" "rumoured"
set_keystate "KEY1" "STATE_KRRSIG" "rumoured"
set_keystate "KEY1" "STATE_ZRRSIG" "rumoured"
set_keystate "KEY1" "STATE_DS"     "hidden"

set_zone "signed.tld"
set_policy "default" "1" "3600"
set_server "ns2" "10.53.0.2"
TSIG=""
check_keys
check_apex
check_subdomain
dnssec_verify

set_zone "override.inherit.signed"
set_policy "default" "1" "3600"
set_server "ns4" "10.53.0.4"
TSIG="hmac-sha1:sha1:$SHA1"
check_keys
check_apex
check_subdomain
dnssec_verify

set_zone "inherit.override.signed"
set_policy "default" "1" "3600"
set_server "ns4" "10.53.0.4"
TSIG="hmac-sha224:sha224:$SHA224"
check_keys
check_apex
check_subdomain
dnssec_verify

set_zone "override.inherit.unsigned"
set_policy "default" "1" "3600"
set_server "ns5" "10.53.0.5"
TSIG="hmac-sha1:sha1:$SHA1"
check_keys
check_apex
check_subdomain
dnssec_verify

set_zone "inherit.override.unsigned"
set_policy "default" "1" "3600"
set_server "ns5" "10.53.0.5"
TSIG="hmac-sha224:sha224:$SHA224"
check_keys
check_apex
check_subdomain
dnssec_verify

# These zones should be signed with the test policy:
# ns4/inherit.inherit.signed
# ns4/override.override.signed
# ns4/override.none.signed
# ns5/override.override.unsigned
# ns5/override.none.unsigned
# ns4/example.net (both views)
set_keyrole      "KEY1" "csk"
set_keylifetime  "KEY1" "0"
set_keyalgorithm "KEY1" "14" "ECDSAP384SHA384" "384"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "yes"

set_zone "inherit.inherit.signed"
set_policy "test" "1" "3600"
set_server "ns4" "10.53.0.4"
TSIG="hmac-sha1:sha1:$SHA1"
wait_for_nsec
check_keys
check_apex
check_subdomain
dnssec_verify

set_zone "override.override.signed"
set_policy "test" "1" "3600"
set_server "ns4" "10.53.0.4"
TSIG="hmac-sha224:sha224:$SHA224"
wait_for_nsec
check_keys
check_apex
check_subdomain
dnssec_verify

set_zone "override.none.signed"
set_policy "test" "1" "3600"
set_server "ns4" "10.53.0.4"
TSIG="hmac-sha256:sha256:$SHA256"
wait_for_nsec
check_keys
check_apex
check_subdomain
dnssec_verify

set_zone "override.override.unsigned"
set_policy "test" "1" "3600"
set_server "ns5" "10.53.0.5"
TSIG="hmac-sha224:sha224:$SHA224"
wait_for_nsec
check_keys
check_apex
check_subdomain
dnssec_verify

set_zone "override.none.unsigned"
set_policy "test" "1" "3600"
set_server "ns5" "10.53.0.5"
TSIG="hmac-sha256:sha256:$SHA256"
wait_for_nsec
check_keys
check_apex
check_subdomain
dnssec_verify

set_zone "example.net"
set_server "ns4" "10.53.0.4"
TSIG="hmac-sha1:keyforview1:$VIEW1"
wait_for_nsec
check_keys
check_apex
dnssec_verify
n=$((n+1))
# check subdomain
echo_i "check TXT example.net (view example1) rrset is signed correctly ($n)"
ret=0
dig_with_opts "view.${ZONE}" "@${SERVER}" TXT > "dig.out.$DIR.test$n.txt" || log_error "dig view.${ZONE} TXT failed"
grep "status: NOERROR" "dig.out.$DIR.test$n.txt" > /dev/null || log_error "mismatch status in DNS response"
grep "view.${ZONE}\..*${DEFAULT_TTL}.*IN.*TXT.*view1" "dig.out.$DIR.test$n.txt" > /dev/null || log_error "missing view.${ZONE} TXT record in response"
check_signatures TXT "dig.out.$DIR.test$n.txt" "ZSK"
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

TSIG="hmac-sha1:keyforview2:$VIEW2"
wait_for_nsec
check_keys
check_apex
dnssec_verify
n=$((n+1))
# check subdomain
echo_i "check TXT example.net (view example2) rrset is signed correctly ($n)"
ret=0
dig_with_opts "view.${ZONE}" "@${SERVER}" TXT > "dig.out.$DIR.test$n.txt" || log_error "dig view.${ZONE} TXT failed"
grep "status: NOERROR" "dig.out.$DIR.test$n.txt" > /dev/null || log_error "mismatch status in DNS response"
grep "view.${ZONE}\..*${DEFAULT_TTL}.*IN.*TXT.*view2" "dig.out.$DIR.test$n.txt" > /dev/null || log_error "missing view.${ZONE} TXT record in response"
check_signatures TXT "dig.out.$DIR.test$n.txt" "ZSK"
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

# Clear TSIG.
TSIG=""

#
# Testing DNSSEC introduction.
#

#
# Zone: step1.enable-dnssec.autosign.
#
set_zone "step1.enable-dnssec.autosign"
set_policy "enable-dnssec" "1" "300"
set_server "ns3" "10.53.0.3"
# Key properties.
key_clear        "KEY1"
set_keyrole      "KEY1" "csk"
set_keylifetime  "KEY1" "0"
set_keyalgorithm "KEY1" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "yes"
# Key timings.
set_keytime  "KEY1" "PUBLISHED"    "yes"
set_keytime  "KEY1" "ACTIVE"       "yes"
# The DNSKEY and signatures are introduced first, the DS remains hidden.
set_keystate "KEY1" "GOAL"         "omnipresent"
set_keystate "KEY1" "STATE_DNSKEY" "rumoured"
set_keystate "KEY1" "STATE_KRRSIG" "rumoured"
set_keystate "KEY1" "STATE_ZRRSIG" "rumoured"
set_keystate "KEY1" "STATE_DS"     "hidden"
# This policy lists only one key (CSK).
key_clear "KEY2"
key_clear "KEY3"
key_clear "KEY4"

check_keys
check_apex
check_subdomain
dnssec_verify

check_next_key_event() {
	_expect=$1

	n=$((n+1))
	echo_i "check next key event for zone ${ZONE} ($n)"
	ret=0
	grep "zone ${ZONE}.*: next key event in .* seconds" "${DIR}/named.run" > "keyevent.out.$ZONE.test$n" || log_error "no next key event for zone ${ZONE}"

	# Get the latest next key event.
	_time=$(awk '{print $10}' < "keyevent.out.$ZONE.test$n" | tail -1)

	# The next key event time must within threshold of the
	# expected time.
	_expectmin=$((_expect-next_key_event_threshold))
	_expectmax=$((_expect+next_key_event_threshold))

	test $_expectmin -le "$_time" || log_error "bad next key event time ${_time} for zone ${ZONE} (expect ${_expect})"
	test $_expectmax -ge "$_time" || log_error "bad next key event time ${_time} for zone ${ZONE} (expect ${_expect})"

	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}

# Next key event is when the DNSKEY RRset becomes OMNIPRESENT: DNSKEY TTL plus
# publish safety plus the zone propagation delay: 900 seconds.
check_next_key_event 900

#
# Zone: step2.enable-dnssec.autosign.
#
set_zone "step2.enable-dnssec.autosign"
set_policy "enable-dnssec" "1" "300"
set_server "ns3" "10.53.0.3"
# The DNSKEY is omnipresent, but the zone signatures not yet.
# Thus, the DS remains hidden.
set_keystate "KEY1" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY1" "STATE_KRRSIG" "omnipresent"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the zone signatures become OMNIPRESENT: max-zone-ttl
# plus zone propagation delay plus retire safety minus the already elapsed
# 900 seconds: 12h + 300s + 20m - 900 = 44700 - 900 = 43800 seconds
check_next_key_event 43800

#
# Zone: step3.enable-dnssec.autosign.
#
set_zone "step3.enable-dnssec.autosign"
set_policy "enable-dnssec" "1" "300"
set_server "ns3" "10.53.0.3"
# The DS can be introduced.
set_keystate "KEY1" "STATE_ZRRSIG" "omnipresent"
set_keystate "KEY1" "STATE_DS"     "rumoured"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the DS can move to the OMNIPRESENT state.  This occurs
# when the parent registration and propagation delay have passed, plus the
# DS TTL and retire safety delay: 1d + 1h + 2h + 20m = 27h20m = 98400 seconds
check_next_key_event 98400

#
# Zone: step4.enable-dnssec.autosign.
#
set_zone "step4.enable-dnssec.autosign"
set_policy "enable-dnssec" "1" "300"
set_server "ns3" "10.53.0.3"
# The DS is omnipresent.
set_keystate "KEY1" "STATE_DS" "omnipresent"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is never, the zone dnssec-policy has been established. So we
# fall back to the default loadkeys interval.
check_next_key_event 3600

#
# Testing ZSK Pre-Publication rollover.
#

#
# Zone: step1.zsk-prepub.autosign.
#
set_zone "step1.zsk-prepub.autosign"
set_policy "zsk-prepub" "2" "3600"
set_server "ns3" "10.53.0.3"
# Key properties.
key_clear        "KEY1"
set_keyrole      "KEY1" "ksk"
set_keylifetime  "KEY1" "63072000"
set_keyalgorithm "KEY1" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "no"

key_clear        "KEY2"
set_keyrole      "KEY2" "zsk"
set_keylifetime  "KEY2" "2592000"
set_keyalgorithm "KEY2" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY2" "no"
set_zonesigning  "KEY2" "yes"
# Key timings.
set_keytime  "KEY1" "PUBLISHED"    "yes"
set_keytime  "KEY1" "ACTIVE"       "yes"
set_keytime  "KEY1" "RETIRED"      "yes"

set_keytime  "KEY2" "PUBLISHED"    "yes"
set_keytime  "KEY2" "ACTIVE"       "yes"
set_keytime  "KEY2" "RETIRED"      "yes"
# Both KSK (KEY1) and ZSK (KEY2) start in OMNIPRESENT.
set_keystate "KEY1" "GOAL"         "omnipresent"
set_keystate "KEY1" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY1" "STATE_KRRSIG" "omnipresent"
set_keystate "KEY1" "STATE_DS"     "omnipresent"

set_keystate "KEY2" "GOAL"         "omnipresent"
set_keystate "KEY2" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY2" "STATE_ZRRSIG" "omnipresent"
# Initially only two keys.
key_clear "KEY3"
key_clear "KEY4"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the successor ZSK needs to be published.  That is
# the ZSK lifetime - prepublication time.  The prepublication time is DNSKEY
# TTL plus publish safety plus the zone propagation delay.  For the
# zsk-prepub policy that means: 30d - 3600s + 1d + 1h = 2498400 seconds.
check_next_key_event 2498400

#
# Zone: step2.zsk-prepub.autosign.
#
set_zone "step2.zsk-prepub.autosign"
set_policy "zsk-prepub" "3" "3600"
set_server "ns3" "10.53.0.3"
# New ZSK (KEY3) is prepublished, but not yet signing.
key_clear        "KEY3"
set_keyrole      "KEY3" "zsk"
set_keylifetime  "KEY3" "2592000"
set_keyalgorithm "KEY3" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY3" "no"
set_zonesigning  "KEY3" "no"
# Key timings.
set_keytime  "KEY3" "PUBLISHED" "yes"
set_keytime  "KEY3" "ACTIVE"    "yes"
set_keytime  "KEY3" "RETIRED"   "yes"
# Key states.
set_keystate "KEY3" "GOAL"   "omnipresent"
set_keystate "KEY3" "STATE_DNSKEY" "rumoured"
set_keystate "KEY3" "STATE_ZRRSIG" "hidden"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the successor ZSK becomes OMNIPRESENT.  That is the
# DNSKEY TTL plus the zone propagation delay, plus the publish-safety. For
# the zsk-prepub policy, this means: 3600s + 1h + 1d = 93600 seconds.
check_next_key_event 93600

#
# Zone: step3.zsk-prepub.autosign.
#
set_zone "step3.zsk-prepub.autosign"
set_policy "zsk-prepub" "3" "3600"
set_server "ns3" "10.53.0.3"
# ZSK (KEY2) no longer is actively signing, RRSIG state in UNRETENTIVE.
# New ZSK (KEY3) is now actively signing, RRSIG state in RUMOURED.
set_zonesigning  "KEY2" "no"
set_keystate     "KEY2" "GOAL" "hidden"
set_keystate     "KEY2" "STATE_ZRRSIG" "unretentive"
set_zonesigning  "KEY3" "yes"
set_keystate     "KEY3" "STATE_DNSKEY" "omnipresent"
set_keystate     "KEY3" "STATE_ZRRSIG" "rumoured"

check_keys
check_apex
# Subdomain still has good signatures of ZSK (KEY2).
# Set expected zone signing on for KEY2 and off for KEY3,
# testing whether signatures which are still valid are being reused.
set_zonesigning  "KEY2" "yes"
set_zonesigning  "KEY3" "no"
check_subdomain
# Restore the expected zone signing properties.
set_zonesigning  "KEY2" "no"
set_zonesigning  "KEY3" "yes"
dnssec_verify

# Next key event is when all the RRSIG records have been replaced with
# signatures of the new ZSK, in other words when ZRRSIG becomes OMNIPRESENT.
# That is Dsgn plus the maximum zone TTL plus the zone propagation delay plus
# retire-safety. For the zsk-prepub policy that means: 1w (because 2w validity
# and refresh within a week) + 1d + 1h + 2d = 10d1h = 867600 seconds.
check_next_key_event 867600

#
# Zone: step4.zsk-prepub.autosign.
#
set_zone "step4.zsk-prepub.autosign"
set_policy "zsk-prepub" "3" "3600"
set_server "ns3" "10.53.0.3"
# ZSK (KEY2) DNSKEY is no longer needed.
# ZSK (KEY3) is now actively signing, RRSIG state in RUMOURED.
set_keystate "KEY2" "STATE_DNSKEY" "unretentive"
set_keystate "KEY2" "STATE_ZRRSIG" "hidden"
set_keystate "KEY3" "STATE_ZRRSIG" "omnipresent"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the DNSKEY enters the HIDDEN state.  This is the
# DNSKEY TTL plus zone propagation delay. For the zsk-prepub policy this is:
# 3600s + 1h = 7200s
check_next_key_event 7200

#
# Zone: step5.zsk-prepub.autosign.
#
set_zone "step5.zsk-prepub.autosign"
set_policy "zsk-prepub" "3" "3600"
set_server "ns3" "10.53.0.3"
# ZSK (KEY3) DNSKEY is now completely HIDDEN and removed.
set_keytime  "KEY2" "REMOVED" "yes"
set_keystate "KEY2" "STATE_DNSKEY" "hidden"

# ZSK (KEY3) remains actively signing, staying in OMNIPRESENT.
check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the new successor needs to be published.  This is the
# ZSK lifetime minus Iret minus Ipub minus DNSKEY TTL.  For the zsk-prepub
# policy this is: 30d - 867600s - 93600s - 3600s = 1627200 seconds.
check_next_key_event 1627200

#
# Testing KSK Double-KSK rollover.
#

#
# Zone: step1.ksk-doubleksk.autosign.
#
set_zone "step1.ksk-doubleksk.autosign"
set_policy "ksk-doubleksk" "2" "7200"
set_server "ns3" "10.53.0.3"
# Key properties.
key_clear        "KEY1"
set_keyrole      "KEY1" "ksk"
set_keylifetime  "KEY1" "5184000"
set_keyalgorithm "KEY1" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "no"

key_clear        "KEY2"
set_keyrole      "KEY2" "zsk"
set_keylifetime  "KEY2" "31536000"
set_keyalgorithm "KEY2" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY2" "no"
set_zonesigning  "KEY2" "yes"
# Key timings.
set_keytime  "KEY1" "PUBLISHED" "yes"
set_keytime  "KEY1" "ACTIVE"    "yes"
set_keytime  "KEY1" "RETIRED"   "yes"

set_keytime  "KEY2" "PUBLISHED" "yes"
set_keytime  "KEY2" "ACTIVE"    "yes"
set_keytime  "KEY2" "RETIRED"   "yes"
# Both KSK (KEY1) and ZSK (KEY2) start in OMNIPRESENT.
set_keystate "KEY1" "GOAL"         "omnipresent"
set_keystate "KEY1" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY1" "STATE_KRRSIG" "omnipresent"
set_keystate "KEY1" "STATE_DS"     "omnipresent"

set_keystate "KEY2" "GOAL"         "omnipresent"
set_keystate "KEY2" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY2" "STATE_ZRRSIG" "omnipresent"
# Initially only two keys.
key_clear "KEY3"
key_clear "KEY4"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the successor KSK needs to be published.  That is
# the KSK lifetime - prepublication time - DS registration delay.  The
# prepublication time is DNSKEY TTL plus publish safety plus the zone
# propagation delay.  For the ksk-doubleksk policy that means:
# 60d - (1d3h) - (1d) = 5000400 seconds.
check_next_key_event 5000400

#
# Zone: step2.ksk-doubleksk.autosign.
#
set_zone "step2.ksk-doubleksk.autosign"
set_policy "ksk-doubleksk" "3" "7200"
set_server "ns3" "10.53.0.3"
# New KSK (KEY3) is prepublished (and signs DNSKEY RRset).
key_clear        "KEY3"
set_keyrole      "KEY3" "ksk"
set_keylifetime  "KEY3" "5184000"
set_keyalgorithm "KEY3" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY3" "yes"
set_zonesigning  "KEY3" "no"
# Key timings.
set_keytime  "KEY3" "PUBLISHED" "yes"
set_keytime  "KEY3" "ACTIVE"    "yes"
set_keytime  "KEY3" "RETIRED"   "yes"
# Key states.
set_keystate "KEY3" "GOAL"         "omnipresent"
set_keystate "KEY3" "STATE_DNSKEY" "rumoured"
set_keystate "KEY3" "STATE_KRRSIG" "rumoured"
set_keystate "KEY3" "STATE_DS"     "hidden"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the successor KSK becomes OMNIPRESENT.  That is the
# DNSKEY TTL plus the zone propagation delay, plus the publish-safety.  For
# the ksk-doubleksk policy, this means: 7200s + 1h + 1d = 97200 seconds.
check_next_key_event 97200

#
# Zone: step3.ksk-doubleksk.autosign.
#
set_zone "step3.ksk-doubleksk.autosign"
set_policy "ksk-doubleksk" "3" "7200"
set_server "ns3" "10.53.0.3"
# KSK (KEY1) DS will be removed, so it is UNRETENTIVE.
set_keystate "KEY1" "GOAL"         "hidden"
set_keystate "KEY1" "STATE_DS"     "unretentive"
# New KSK (KEY3) has its DS submitted.
set_keystate "KEY3" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY3" "STATE_KRRSIG" "omnipresent"
set_keystate "KEY3" "STATE_DS"     "rumoured"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the predecessor DS has been replaced with the
# successor DS and enough time has passed such that the all validators that
# have this DS RRset cached only know about the successor DS.  This is the
# registration delay plus the retire interval, which is the parent
# propagation delay plus the DS TTL plus the retire-safety.  For the
# ksk-double-ksk policy this means: 1d + 1h + 3600s + 2d = 3d2h =
# 266400 seconds.
check_next_key_event 266400

#
# Zone: step4.ksk-doubleksk.autosign.
#
set_zone "step4.ksk-doubleksk.autosign"
set_policy "ksk-doubleksk" "3" "7200"
set_server "ns3" "10.53.0.3"
# KSK (KEY1) DNSKEY can be removed.
set_keysigning "KEY1" "no"
set_keystate   "KEY1" "STATE_DNSKEY" "unretentive"
set_keystate   "KEY1" "STATE_KRRSIG" "unretentive"
set_keystate   "KEY1" "STATE_DS"     "hidden"
# New KSK (KEY3) DS is now OMNIPRESENT.
set_keystate   "KEY3" "STATE_DS"     "omnipresent"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the DNSKEY enters the HIDDEN state.  This is the
# DNSKEY TTL plus zone propagation delay. For the ksk-doubleksk policy this is:
# 7200s + 1h = 10800s
check_next_key_event 10800

#
# Zone: step5.ksk-doubleksk.autosign.
#
set_zone "step5.ksk-doubleksk.autosign"
set_policy "ksk-doubleksk" "3" "7200"
set_server "ns3" "10.53.0.3"
# KSK (KEY1) DNSKEY is now HIDDEN.
set_keystate "KEY1" "STATE_DNSKEY" "hidden"
set_keystate "KEY1" "STATE_KRRSIG" "hidden"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the new successor needs to be published.  This is the
# KSK lifetime minus Ipub minus Dreg minus Iret minus DNSKEY TTL.  For the
# ksk-doubleksk this is: 60d - 1d3h - 1d - 2d2h - 2h =
# 5184000 - 97200 - 86400 - 180000 - 7200 = 4813200 seconds.
check_next_key_event 4813200

#
# Testing CSK key rollover (1).
#

#
# Zone: step1.csk-roll.autosign.
#
set_zone "step1.csk-roll.autosign"
set_policy "csk-roll" "1" "3600"
set_server "ns3" "10.53.0.3"
# Key properties.
key_clear        "KEY1"
set_keyrole      "KEY1" "csk"
set_keylifetime  "KEY1" "16070400"
set_keyalgorithm "KEY1" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "yes"
# Key timings.
set_keytime  "KEY1" "PUBLISHED" "yes"
set_keytime  "KEY1" "ACTIVE"    "yes"
set_keytime  "KEY1" "RETIRED"   "yes"
# The CSK (KEY1) starts in OMNIPRESENT.
set_keystate "KEY1" "GOAL"         "omnipresent"
set_keystate "KEY1" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY1" "STATE_KRRSIG" "omnipresent"
set_keystate "KEY1" "STATE_ZRRSIG" "omnipresent"
set_keystate "KEY1" "STATE_DS"     "omnipresent"
# Initially only one key.
key_clear "KEY2"
key_clear "KEY3"
key_clear "KEY4"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the successor CSK needs to be published.  That is
# the CSK lifetime - prepublication time - DS registration delay.  The
# prepublication time is DNSKEY TTL plus publish safety plus the zone
# propagation delay.  For the csk-roll policy that means:
# 6mo - 1d - 3h = 15973200 seconds.
check_next_key_event 15973200

#
# Zone: step2.csk-roll.autosign.
#
set_zone "step2.csk-roll.autosign"
set_policy "csk-roll" "2" "3600"
set_server "ns3" "10.53.0.3"
# New CSK (KEY2) is prepublished (signs DNSKEY RRset, but not yet other RRsets).
key_clear        "KEY2"
set_keyrole      "KEY2" "csk"
set_keylifetime  "KEY2" "16070400"
set_keyalgorithm "KEY2" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY2" "yes"
set_zonesigning  "KEY2" "no"
# Key timings.
set_keytime  "KEY2" "PUBLISHED" "yes"
set_keytime  "KEY2" "ACTIVE"    "yes"
set_keytime  "KEY2" "RETIRED"   "yes"
# Key states.
set_keystate "KEY2" "GOAL"         "omnipresent"
set_keystate "KEY2" "STATE_DNSKEY" "rumoured"
set_keystate "KEY2" "STATE_KRRSIG" "rumoured"
set_keystate "KEY2" "STATE_ZRRSIG" "hidden"
set_keystate "KEY2" "STATE_DS"     "hidden"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the successor CSK becomes OMNIPRESENT.  That is the
# DNSKEY TTL plus the zone propagation delay, plus the publish-safety. For
# the csk-roll policy, this means 3 hours = 10800 seconds.
check_next_key_event 10800

#
# Zone: step3.csk-roll.autosign.
#
set_zone "step3.csk-roll.autosign"
set_policy "csk-roll" "2" "3600"
set_server "ns3" "10.53.0.3"
# Swap zone signing role.
set_zonesigning  "KEY1" "no"
set_zonesigning  "KEY2" "yes"
# CSK (KEY1) DS and ZRRSIG will be removed, so it is UNRETENTIVE.
set_keystate "KEY1" "GOAL"         "hidden"
set_keystate "KEY1" "STATE_ZRRSIG" "unretentive"
set_keystate "KEY1" "STATE_DS"     "unretentive"
# New CSK (KEY2) has its DS submitted, and is signing, so the DS and ZRRSIG
# are in RUMOURED state.
set_keystate "KEY2" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY2" "STATE_KRRSIG" "omnipresent"
set_keystate "KEY2" "STATE_ZRRSIG" "rumoured"
set_keystate "KEY2" "STATE_DS"     "rumoured"

check_keys
check_apex
# Subdomain still has good signatures of old CSK (KEY1).
# Set expected zone signing on for KEY1 and off for KEY2,
# testing whether signatures which are still valid are being reused.
set_zonesigning  "KEY1" "yes"
set_zonesigning  "KEY2" "no"
check_subdomain
# Restore the expected zone signing properties.
set_zonesigning  "KEY1" "no"
set_zonesigning  "KEY2" "yes"
dnssec_verify

# Next key event is when the predecessor DS has been replaced with the
# successor DS and enough time has passed such that the all validators that
# have this DS RRset cached only know about the successor DS.  This is the
# registration delay plus the retire interval, which is the parent
# propagation delay plus the DS TTL plus the retire-safety.  For the
# csk-roll policy this means: 1d + 1h + 1h + 2h = 1d4h = 100800 seconds.
check_next_key_event 100800

#
# Zone: step4.csk-roll.autosign.
#
set_zone "step4.csk-roll.autosign"
set_policy "csk-roll" "2" "3600"
set_server "ns3" "10.53.0.3"
# The old CSK (KEY1) is no longer signing the DNSKEY RRset.
set_keysigning   "KEY1" "no"
# The old CSK (KEY1) DS is hidden.  We still need to keep the DNSKEY public
# but can remove the KRRSIG records.
set_keystate "KEY1" "STATE_KRRSIG" "unretentive"
set_keystate "KEY1" "STATE_DS"     "hidden"
# The new CSK (KEY2) DS is now OMNIPRESENT.
set_keystate "KEY2" "STATE_DS"     "omnipresent"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the KRRSIG enters the HIDDEN state.  This is the
# DNSKEY TTL plus zone propagation delay. For the csk-roll policy this is:
# 1h + 1h = 7200 seconds.
check_next_key_event 7200

#
# Zone: step5.csk-roll.autosign.
#
set_zone "step5.csk-roll.autosign"
set_policy "csk-roll" "2" "3600"
set_server "ns3" "10.53.0.3"
# The old CSK (KEY1) KRRSIG records are now all hidden.
set_keystate "KEY1" "STATE_KRRSIG" "hidden"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the DNSKEY can be removed.  This is when all ZRRSIG
# records have been replaced with signatures of the new CSK.  We have
# calculated the interval to be 26d3h of which 1d4h (Dreg + Iret(KSK)) plus
# 2h (DNSKEY TTL + Dprp) have already passed.  So next key event is in
# 26d3h - 1d4h - 2h = 597h = 2149200 seconds.
check_next_key_event 2149200

#
# Zone: step6.csk-roll.autosign.
#
set_zone "step6.csk-roll.autosign"
set_policy "csk-roll" "2" "3600"
set_server "ns3" "10.53.0.3"
# The old CSK (KEY1) ZRRSIG records are now all hidden (so the DNSKEY can
# be removed).
set_keystate "KEY1" "STATE_DNSKEY" "unretentive"
set_keystate "KEY1" "STATE_ZRRSIG" "hidden"
# The new CSK (KEY2) is now fully OMNIPRESENT.
set_keystate "KEY2" "STATE_ZRRSIG" "omnipresent"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the DNSKEY enters the HIDDEN state.  This is the
# DNSKEY TTL plus zone propagation delay. For the csk-roll policy this is:
# 1h + 1h = 7200 seconds.
check_next_key_event 7200

#
# Zone: step7.csk-roll.autosign.
#
set_zone "step7.csk-roll.autosign"
set_policy "csk-roll" "2" "3600"
set_server "ns3" "10.53.0.3"
# The old CSK (KEY1) is now completely HIDDEN.
set_keystate "KEY1" "STATE_DNSKEY" "hidden"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the new successor needs to be published.  This is the
# CSK lifetime minus Ipub minus Dreg minus Iret minus DNSKEY TTL minus zone
# propagation delay.  For the csk-roll this is:
# 6mo - 3h - 1d - 26d3h - 1h - 1h = 6mo - 27d8h = 13708800 seconds.
check_next_key_event 13708800

#
# Testing CSK key rollover (2).
#

#
# Zone: step1.csk-roll2.autosign.
#
set_zone "step1.csk-roll2.autosign"
set_policy "csk-roll2" "1" "3600"
set_server "ns3" "10.53.0.3"
# Key properties.
key_clear        "KEY1"
set_keyrole      "KEY1" "csk"
set_keylifetime  "KEY1" "16070400"
set_keyalgorithm "KEY1" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "yes"
# Key timings.
set_keytime  "KEY1" "PUBLISHED"    "yes"
set_keytime  "KEY1" "ACTIVE"       "yes"
set_keytime  "KEY1" "RETIRED"      "yes"
# The CSK (KEY1) starts in OMNIPRESENT.
set_keystate "KEY1" "GOAL"         "omnipresent"
set_keystate "KEY1" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY1" "STATE_KRRSIG" "omnipresent"
set_keystate "KEY1" "STATE_ZRRSIG" "omnipresent"
set_keystate "KEY1" "STATE_DS"     "omnipresent"
# Initially only one key.
key_clear "KEY2"
key_clear "KEY3"
key_clear "KEY4"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the successor CSK needs to be published.  That is
# the CSK lifetime - prepublication time - DS registration delay.  The
# prepublication time is DNSKEY TTL plus publish safety plus the zone
# propagation delay.  For the csk-roll2 policy that means:
# 6mo - 3h - 1w = 15454800 seconds.
check_next_key_event 15454800

#
# Zone: step2.csk-roll2.autosign.
#
set_zone "step2.csk-roll2.autosign"
set_policy "csk-roll2" "2" "3600"
set_server "ns3" "10.53.0.3"
# New CSK (KEY2) is prepublished (signs DNSKEY RRset, but not yet other RRsets).
key_clear        "KEY2"
set_keyrole      "KEY2" "csk"
set_keylifetime  "KEY2" "16070400"
set_keyalgorithm "KEY2" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY2" "yes"
set_zonesigning  "KEY2" "no"
# Key timings.
set_keytime  "KEY2" "PUBLISHED"    "yes"
set_keytime  "KEY2" "ACTIVE"       "yes"
set_keytime  "KEY2" "RETIRED"      "yes"
# Key states.
set_keystate "KEY2" "GOAL"         "omnipresent"
set_keystate "KEY2" "STATE_DNSKEY" "rumoured"
set_keystate "KEY2" "STATE_KRRSIG" "rumoured"
set_keystate "KEY2" "STATE_ZRRSIG" "hidden"
set_keystate "KEY2" "STATE_DS"     "hidden"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the successor CSK becomes OMNIPRESENT.  That is the
# DNSKEY TTL plus the zone propagation delay, plus the publish-safety. For
# the csk-roll2 policy, this means 3 hours = 10800 seconds.
check_next_key_event 10800

#
# Zone: step3.csk-roll2.autosign.
#
set_zone "step3.csk-roll2.autosign"
set_policy "csk-roll2" "2" "3600"
set_server "ns3" "10.53.0.3"
# CSK (KEY1) DS and ZRRSIG will be removed, so it is UNRETENTIVE.
set_zonesigning  "KEY1" "no"
set_keystate     "KEY1" "GOAL"         "hidden"
set_keystate     "KEY1" "STATE_ZRRSIG" "unretentive"
set_keystate     "KEY1" "STATE_DS"     "unretentive"
# New CSK (KEY2) has its DS submitted, and is signing, so the DS and ZRRSIG
# are in RUMOURED state.
set_zonesigning  "KEY2" "yes"
set_keystate     "KEY2" "STATE_DNSKEY" "omnipresent"
set_keystate     "KEY2" "STATE_KRRSIG" "omnipresent"
set_keystate     "KEY2" "STATE_ZRRSIG" "rumoured"
set_keystate     "KEY2" "STATE_DS"     "rumoured"

check_keys
check_apex
# Subdomain still has good signatures of old CSK (KEY1).
# Set expected zone signing on for KEY1 and off for KEY2,
# testing whether signatures which are still valid are being reused.
set_zonesigning  "KEY1" "yes"
set_zonesigning  "KEY2" "no"
check_subdomain
# Restore the expected zone signing properties.
set_zonesigning  "KEY1" "no"
set_zonesigning  "KEY2" "yes"
dnssec_verify

# Next key event is when the predecessor ZRRSIG records have been replaced
# with that of the successor and enough time has passed such that the all
# validators that have such signed RRsets in cache only know about the
# successor signatures.  This is the retire interval: Dsgn plus the
# maximum zone TTL plus the zone propagation delay plus retire-safety. For the
# csk-roll2 policy that means: 12h (because 1d validity and refresh within
# 12 hours) + 1d + 1h + 1h = 38h = 136800 seconds.
check_next_key_event 136800

#
# Zone: step4.csk-roll2.autosign.
#
set_zone "step4.csk-roll2.autosign"
set_policy "csk-roll2" "2" "3600"
set_server "ns3" "10.53.0.3"
# The old CSK (KEY1) ZRRSIG is now HIDDEN.
set_keystate "KEY1" "STATE_ZRRSIG" "hidden"
# The new CSK (KEY2) ZRRSIG is now OMNIPRESENT.
set_keystate "KEY2" "STATE_ZRRSIG" "omnipresent"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the predecessor DS has been replaced with the
# successor DS and enough time has passed such that the all validators that
# have this DS RRset cached only know about the successor DS.  This is the
# registration delay plus the retire interval, which is the parent
# propagation delay plus the DS TTL plus the retire-safety.  For the
# csk-roll2 policy this means: 1w + 1h + 1h + 1h = 171h = 615600 seconds.
# However, 136800 seconds have passed already, so 478800 seconds left.
check_next_key_event 478800

#
# Zone: step5.csk-roll2.autosign.
#
set_zone "step5.csk-roll2.autosign"
set_policy "csk-roll2" "2" "3600"
set_server "ns3" "10.53.0.3"
# The old CSK (KEY1) DNSKEY can be removed.
set_keysigning   "KEY1" "no"
set_keystate     "KEY1" "STATE_DNSKEY" "unretentive"
set_keystate     "KEY1" "STATE_KRRSIG" "unretentive"
set_keystate     "KEY1" "STATE_DS"     "hidden"
# The new CSK (KEY2) is now fully OMNIPRESENT.
set_keystate     "KEY2" "STATE_DS"     "omnipresent"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the DNSKEY enters the HIDDEN state.  This is the
# DNSKEY TTL plus zone propagation delay. For the csk-roll policy this is:
# 1h + 1h = 7200 seconds.
check_next_key_event 7200

#
# Zone: step6.csk-roll2.autosign.
#
set_zone "step6.csk-roll2.autosign"
set_policy "csk-roll2" "2" "3600"
set_server "ns3" "10.53.0.3"
# The old CSK (KEY1) is now completely HIDDEN.
set_keystate "KEY1" "STATE_DNSKEY" "hidden"
set_keystate "KEY1" "STATE_KRRSIG" "hidden"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the new successor needs to be published.
check_next_key_event 14684400

#
# Testing algorithm rollover.
#

#
# Zone: step1.algorithm-roll.kasp
#
set_zone "step1.algorithm-roll.kasp"
set_policy "rsasha1" "2" "3600"
set_server "ns6" "10.53.0.6"
# Key properties.
key_clear        "KEY1"
set_keyrole      "KEY1" "ksk"
set_keylifetime  "KEY1" "0"
set_keyalgorithm "KEY1" "5" "RSASHA1" "2048"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "no"

key_clear        "KEY2"
set_keyrole      "KEY2" "zsk"
set_keylifetime  "KEY2" "0"
set_keyalgorithm "KEY2" "5" "RSASHA1" "2048"
set_keysigning   "KEY2" "no"
set_zonesigning  "KEY2" "yes"
key_clear "KEY3"
key_clear "KEY4"
# Key timings.
set_keytime  "KEY1" "PUBLISHED"    "yes"
set_keytime  "KEY1" "ACTIVE"       "yes"

set_keytime  "KEY2" "PUBLISHED"    "yes"
set_keytime  "KEY2" "ACTIVE"       "yes"
# The KSK (KEY1) and ZSK (KEY2) start in OMNIPRESENT.
set_keystate "KEY1" "GOAL"         "omnipresent"
set_keystate "KEY1" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY1" "STATE_KRRSIG" "omnipresent"
set_keystate "KEY1" "STATE_DS"     "omnipresent"

set_keystate "KEY2" "GOAL"         "omnipresent"
set_keystate "KEY2" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY2" "STATE_ZRRSIG" "omnipresent"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the successor keys need to be published.
# Since the lifetime of the keys are unlimited, so default to loadkeys
# interval.
check_next_key_event 3600

#
# Zone: step1.csk-algorithm-roll.kasp
#
set_zone "step1.csk-algorithm-roll.kasp"
set_policy "csk-algoroll" "1" "3600"
set_server "ns6" "10.53.0.6"
# Key properties.
key_clear        "KEY1"
set_keyrole      "KEY1" "csk"
set_keylifetime  "KEY1" "0"
set_keyalgorithm "KEY1" "5" "RSASHA1" "2048"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "yes"
key_clear "KEY2"
key_clear "KEY3"
key_clear "KEY4"
# Key timings.
set_keytime  "KEY1" "PUBLISHED"    "yes"
set_keytime  "KEY1" "ACTIVE"       "yes"
# The CSK (KEY1) starts in OMNIPRESENT.
set_keystate "KEY1" "GOAL"         "omnipresent"
set_keystate "KEY1" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY1" "STATE_KRRSIG" "omnipresent"
set_keystate "KEY1" "STATE_ZRRSIG" "omnipresent"
set_keystate "KEY1" "STATE_DS"     "omnipresent"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the successor keys need to be published.
# Since the lifetime of the keys are unlimited, so default to loadkeys
# interval.
check_next_key_event 3600

#
# Testing good migration.
#
set_zone "migrate.kasp"
set_policy "none" "2" "7200"
set_server "ns6" "10.53.0.6"

init_migration_match() {
	key_clear        "KEY1"
	key_set          "KEY1" "LEGACY" "yes"
	set_keyrole      "KEY1" "ksk"
	set_keylifetime  "KEY1" "0"
	set_keyalgorithm "KEY1" "13" "ECDSAP256SHA256" "256"
	set_keysigning   "KEY1" "yes"
	set_zonesigning  "KEY1" "no"

	key_clear        "KEY2"
	key_set          "KEY2" "LEGACY" "yes"
	set_keyrole      "KEY2" "zsk"
	set_keylifetime  "KEY2" "5184000"
	set_keyalgorithm "KEY2" "13" "ECDSAP256SHA256" "256"
	set_keysigning   "KEY2" "no"
	set_zonesigning  "KEY2" "yes"

	key_clear        "KEY3"
	key_clear        "KEY4"

	set_keytime  "KEY1" "PUBLISHED"    "yes"
	set_keytime  "KEY1" "ACTIVE"       "yes"
	set_keytime  "KEY1" "RETIRED"      "none"
	set_keystate "KEY1" "GOAL"         "omnipresent"
	set_keystate "KEY1" "STATE_DNSKEY" "rumoured"
	set_keystate "KEY1" "STATE_KRRSIG" "rumoured"
	set_keystate "KEY1" "STATE_DS"     "rumoured"

	set_keytime  "KEY2" "PUBLISHED"    "yes"
	set_keytime  "KEY2" "ACTIVE"       "yes"
	set_keytime  "KEY2" "RETIRED"      "none"
	set_keystate "KEY2" "GOAL"         "omnipresent"
	set_keystate "KEY2" "STATE_DNSKEY" "rumoured"
	set_keystate "KEY2" "STATE_ZRRSIG" "rumoured"
}
init_migration_match

# Make sure the zone is signed with legacy keys.
check_keys
check_apex
check_subdomain
dnssec_verify

# Remember legacy key tags.
_migrate_ksk=$(key_get KEY1 ID)
_migrate_zsk=$(key_get KEY2 ID)

#
# Testing migration with unmatched existing keys (different algorithm).
#
set_zone "migrate-nomatch-algnum.kasp"
set_policy "none" "2" "300"
set_server "ns6" "10.53.0.6"

init_migration_nomatch_algnum() {
	key_clear        "KEY1"
	key_set          "KEY1" "LEGACY" "yes"
	set_keyrole      "KEY1" "ksk"
	set_keyalgorithm "KEY1" "5" "RSASHA1" "2048"
	set_keysigning   "KEY1" "yes"
	set_zonesigning  "KEY1" "no"

	key_clear        "KEY2"
	key_set          "KEY2" "LEGACY" "yes"
	set_keyrole      "KEY2" "zsk"
	set_keyalgorithm "KEY2" "5" "RSASHA1" "1024"
	set_keysigning   "KEY2" "no"
	set_zonesigning  "KEY2" "yes"

	key_clear        "KEY3"
	key_clear        "KEY4"

	set_keytime  "KEY1" "PUBLISHED"    "yes"
	set_keytime  "KEY1" "ACTIVE"       "yes"
	set_keytime  "KEY1" "RETIRED"      "none"
	set_keystate "KEY1" "GOAL"         "omnipresent"
	set_keystate "KEY1" "STATE_DNSKEY" "omnipresent"
	set_keystate "KEY1" "STATE_KRRSIG" "omnipresent"
	set_keystate "KEY1" "STATE_DS"     "omnipresent"

	set_keytime  "KEY2" "PUBLISHED"    "yes"
	set_keytime  "KEY2" "ACTIVE"       "yes"
	set_keytime  "KEY2" "RETIRED"      "none"
	set_keystate "KEY2" "GOAL"         "omnipresent"
	set_keystate "KEY2" "STATE_DNSKEY" "omnipresent"
	set_keystate "KEY2" "STATE_ZRRSIG" "omnipresent"
}
init_migration_nomatch_algnum

# Make sure the zone is signed with legacy keys.
check_keys
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
set_server "ns6" "10.53.0.6"

init_migration_nomatch_alglen() {
	key_clear        "KEY1"
	key_set          "KEY1" "LEGACY" "yes"
	set_keyrole      "KEY1" "ksk"
	set_keyalgorithm "KEY1" "5" "RSASHA1" "1024"
	set_keysigning   "KEY1" "yes"
	set_zonesigning  "KEY1" "no"

	key_clear        "KEY2"
	key_set          "KEY2" "LEGACY" "yes"
	set_keyrole      "KEY2" "zsk"
	set_keyalgorithm "KEY2" "5" "RSASHA1" "1024"
	set_keysigning   "KEY2" "no"
	set_zonesigning  "KEY2" "yes"

	key_clear        "KEY3"
	key_clear        "KEY4"

	set_keytime  "KEY1" "PUBLISHED"    "yes"
	set_keytime  "KEY1" "ACTIVE"       "yes"
	set_keytime  "KEY1" "RETIRED"      "none"
	set_keystate "KEY1" "GOAL"         "omnipresent"
	set_keystate "KEY1" "STATE_DNSKEY" "omnipresent"
	set_keystate "KEY1" "STATE_KRRSIG" "omnipresent"
	set_keystate "KEY1" "STATE_DS"     "omnipresent"

	set_keytime  "KEY2" "PUBLISHED"    "yes"
	set_keytime  "KEY2" "ACTIVE"       "yes"
	set_keytime  "KEY2" "RETIRED"      "none"
	set_keystate "KEY2" "GOAL"         "omnipresent"
	set_keystate "KEY2" "STATE_DNSKEY" "omnipresent"
	set_keystate "KEY2" "STATE_ZRRSIG" "omnipresent"
}
init_migration_nomatch_alglen

# Make sure the zone is signed with legacy keys.
check_keys
check_apex
check_subdomain
dnssec_verify

# Remember legacy key tags.
_migratenomatch_alglen_ksk=$(key_get KEY1 ID)
_migratenomatch_alglen_zsk=$(key_get KEY2 ID)

# Reconfig dnssec-policy (triggering algorithm roll and other dnssec-policy
# changes).
echo_i "reconfig dnssec-policy to trigger algorithm rollover"
copy_setports ns6/named2.conf.in ns6/named.conf
rndc_reconfig ns6 10.53.0.6

# Calculate time passed to correctly check for next key events.
now="$(TZ=UTC date +%s)"
time_passed=$((now-start_time))
echo_i "${time_passed} seconds passed between start of tests and reconfig"

#  The NSEC record at the apex of the zone and its RRSIG records are
#  added as part of the last step in signing a zone.  We wait for the
#  NSEC records to appear before proceeding with a counter to prevent
#  infinite loops if there is a error.
#
n=$((n+1))
echo_i "waiting for reconfig signing changes to take effect ($n)"
i=0
while [ $i -lt 30 ]
do
	ret=0
	while read -r zone
	do
		dig_with_opts "$zone" @10.53.0.6 nsec > "dig.out.ns6.test$n.$zone" || ret=1
		grep "NS SOA" "dig.out.ns6.test$n.$zone" > /dev/null || ret=1
		grep "$zone\..*IN.*RRSIG" "dig.out.ns6.test$n.$zone" > /dev/null || ret=1
	done < ns6/zones.2

	i=$((i+1))
	if [ $ret = 0 ]; then break; fi
	echo_i "waiting ... ($i)"
	sleep 1
done
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

next_key_event_threshold=$((next_key_event_threshold+i))

#
# Testing migration.
#
set_zone "migrate.kasp"
set_policy "migrate" "2" "7200"
set_server "ns6" "10.53.0.6"

# Key properties, timings and metadata should be the same as legacy keys above.
# However, because the zsk has a lifetime, kasp will set the retired time.
init_migration_match

key_set     "KEY1" "LEGACY"  "no"

key_set     "KEY2" "LEGACY"  "no"
set_keytime "KEY2" "RETIRED" "yes"

check_keys
check_apex
check_subdomain
dnssec_verify

# Check key tags, should be the same.
n=$((n+1))
echo_i "check that of zone ${ZONE} migration to dnssec-policy uses the same keys ($n)"
ret=0
[ $_migrate_ksk == $(key_get KEY1 ID) ] || log_error "mismatch ksk tag"
[ $_migrate_zsk == $(key_get KEY2 ID) ] || log_error "mismatch zsk tag"
status=$((status+ret))

# Test migration to dnssec-policy, existing keys do not match key algorithm.
set_zone "migrate-nomatch-algnum.kasp"
set_policy "migrate-nomatch-algnum" "4" "300"
set_server "ns6" "10.53.0.6"

# The legacy keys need to be retired, but otherwise stay present until the
# new keys are omnipresent, and can be used to construct a chain of trust.
init_migration_nomatch_algnum

key_set      "KEY1" "LEGACY"  "no"
set_keytime  "KEY1" "RETIRED" "yes"
set_keystate "KEY1" "GOAL"    "hidden"

key_set      "KEY2" "LEGACY"  "no"
set_keytime  "KEY2" "RETIRED" "yes"
set_keystate "KEY2" "GOAL"    "hidden"

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

set_keytime  "KEY3" "PUBLISHED"    "yes"
set_keytime  "KEY3" "ACTIVE"       "yes"
set_keytime  "KEY3" "RETIRED"      "none"
set_keystate "KEY3" "GOAL"         "omnipresent"
set_keystate "KEY3" "STATE_DNSKEY" "rumoured"
set_keystate "KEY3" "STATE_KRRSIG" "rumoured"
set_keystate "KEY3" "STATE_DS"     "hidden"

set_keytime  "KEY4" "PUBLISHED"    "yes"
set_keytime  "KEY4" "ACTIVE"       "yes"
set_keytime  "KEY4" "RETIRED"      "yes"
set_keystate "KEY4" "GOAL"         "omnipresent"
set_keystate "KEY4" "STATE_DNSKEY" "rumoured"
set_keystate "KEY4" "STATE_ZRRSIG" "rumoured"

check_keys
check_apex
check_subdomain
dnssec_verify

# Check key tags, should be the same.
n=$((n+1))
echo_i "check that of zone ${ZONE} migration to dnssec-policy keeps existing keys ($n)"
ret=0
[ $_migratenomatch_algnum_ksk == $(key_get KEY1 ID) ] || log_error "mismatch ksk tag"
[ $_migratenomatch_algnum_zsk == $(key_get KEY2 ID) ] || log_error "mismatch zsk tag"
status=$((status+ret))

# Test migration to dnssec-policy, existing keys do not match key length.
set_zone "migrate-nomatch-alglen.kasp"
set_policy "migrate-nomatch-alglen" "4" "300"
set_server "ns6" "10.53.0.6"

# The legacy keys need to be retired, but otherwise stay present until the
# new keys are omnipresent, and can be used to construct a chain of trust.
init_migration_nomatch_alglen

key_set      "KEY1" "LEGACY"  "no"
set_keytime  "KEY1" "RETIRED" "yes"
set_keystate "KEY1" "GOAL"    "hidden"

key_set      "KEY2" "LEGACY"  "no"
set_keytime  "KEY2" "RETIRED" "yes"
set_keystate "KEY2" "GOAL"    "hidden"

set_keyrole      "KEY3" "ksk"
set_keylifetime  "KEY3" "0"
set_keyalgorithm "KEY3" "5" "RSASHA1" "2048"
set_keysigning   "KEY3" "yes"
set_zonesigning  "KEY3" "no"

set_keyrole      "KEY4" "zsk"
set_keylifetime  "KEY4" "5184000"
set_keyalgorithm "KEY4" "5" "RSASHA1" "2048"
set_keysigning   "KEY4" "no"
# This key is considered to be prepublished, so it is not yet signing.
set_zonesigning  "KEY4" "no"

set_keytime  "KEY3" "PUBLISHED"    "yes"
set_keytime  "KEY3" "ACTIVE"       "yes"
set_keytime  "KEY3" "RETIRED"      "none"
set_keystate "KEY3" "GOAL"         "omnipresent"
set_keystate "KEY3" "STATE_DNSKEY" "rumoured"
set_keystate "KEY3" "STATE_KRRSIG" "rumoured"
set_keystate "KEY3" "STATE_DS"     "hidden"

set_keytime  "KEY4" "PUBLISHED"    "yes"
set_keytime  "KEY4" "ACTIVE"       "yes"
set_keytime  "KEY4" "RETIRED"      "yes"
set_keystate "KEY4" "GOAL"         "omnipresent"
set_keystate "KEY4" "STATE_DNSKEY" "rumoured"
set_keystate "KEY4" "STATE_ZRRSIG" "hidden"

check_keys
check_apex
check_subdomain
dnssec_verify

# Check key tags, should be the same.
n=$((n+1))
echo_i "check that of zone ${ZONE} migration to dnssec-policy keeps existing keys ($n)"
ret=0
[ $_migratenomatch_alglen_ksk == $(key_get KEY1 ID) ] || log_error "mismatch ksk tag"
[ $_migratenomatch_alglen_zsk == $(key_get KEY2 ID) ] || log_error "mismatch zsk tag"
status=$((status+ret))

#
# Testing KSK/ZSK algorithm rollover.
#

#
# Zone: step1.algorithm-roll.kasp
#
set_zone "step1.algorithm-roll.kasp"
set_policy "ecdsa256" "4" "3600"
set_server "ns6" "10.53.0.6"
# Old RSASHA1 keys.
key_clear        "KEY1"
set_keyrole      "KEY1" "ksk"
set_keylifetime  "KEY1" "0"
set_keyalgorithm "KEY1" "5" "RSASHA1" "2048"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "no"

key_clear        "KEY2"
set_keyrole      "KEY2" "zsk"
set_keylifetime  "KEY2" "0"
set_keyalgorithm "KEY2" "5" "RSASHA1" "2048"
set_keysigning   "KEY2" "no"
set_zonesigning  "KEY2" "yes"
# New ECDSAP256SHA256 keys.
key_clear        "KEY3"
set_keyrole      "KEY3" "ksk"
set_keylifetime  "KEY3" "0"
set_keyalgorithm "KEY3" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY3" "yes"
set_zonesigning  "KEY3" "no"

key_clear        "KEY4"
set_keyrole      "KEY4" "zsk"
set_keylifetime  "KEY4" "0"
set_keyalgorithm "KEY4" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY4" "no"
set_zonesigning  "KEY4" "yes"
# The RSAHSHA1 keys are outroducing.
set_keytime  "KEY1" "PUBLISHED"    "yes"
set_keytime  "KEY1" "ACTIVE"       "yes"
set_keytime  "KEY1" "RETIRED"      "yes"
set_keystate "KEY1" "GOAL"         "hidden"
set_keystate "KEY1" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY1" "STATE_KRRSIG" "omnipresent"
set_keystate "KEY1" "STATE_DS"     "omnipresent"

set_keytime  "KEY2" "PUBLISHED"    "yes"
set_keytime  "KEY2" "ACTIVE"       "yes"
set_keytime  "KEY2" "RETIRED"      "yes"
set_keystate "KEY2" "GOAL"         "hidden"
set_keystate "KEY2" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY2" "STATE_ZRRSIG" "omnipresent"
# The ECDSAP256SHA256 keys are introducing.
set_keytime  "KEY3" "PUBLISHED"    "yes"
set_keytime  "KEY3" "ACTIVE"       "yes"
set_keystate "KEY3" "GOAL"         "omnipresent"
set_keystate "KEY3" "STATE_DNSKEY" "rumoured"
set_keystate "KEY3" "STATE_KRRSIG" "rumoured"
set_keystate "KEY3" "STATE_DS"     "hidden"

set_keytime  "KEY4" "PUBLISHED"    "yes"
set_keytime  "KEY4" "ACTIVE"       "yes"
set_keystate "KEY4" "GOAL"         "omnipresent"
set_keystate "KEY4" "STATE_DNSKEY" "rumoured"
set_keystate "KEY4" "STATE_ZRRSIG" "rumoured"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the ecdsa256 keys have been propagated.
# This is the DNSKEY TTL plus publish safety plus zone propagation delay:
# 3 times an hour: 10800 seconds.
check_next_key_event 10800

#
# Zone: step2.algorithm-roll.kasp
#
set_zone "step2.algorithm-roll.kasp"
set_policy "ecdsa256" "4" "3600"
set_server "ns6" "10.53.0.6"
# The RSAHSHA1 keys are outroducing, but need to stay present until the new
# algorithm chain of trust has been established. Thus the properties, timings
# and states of the KEY1 and KEY2 are the same as above.
#
# The ECDSAP256SHA256 keys are introducing. The DNSKEY RRset is omnipresent,
# but the zone signatures are not.
set_keystate "KEY3" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY3" "STATE_KRRSIG" "omnipresent"
set_keystate "KEY4" "STATE_DNSKEY" "omnipresent"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when all zone signatures are signed with the new
# algorithm.  This is the max-zone-ttl plus zone propagation delay
# plus retire safety: 6h + 1h + 2h.  But three hours have already passed
# (the time it took to make the DNSKEY omnipresent), so the next event
# should be scheduled in 6 hour: 21600 seconds.  Prevent intermittent
# false positives on slow platforms by subtracting the number of seconds
# which passed between key creation and invoking 'rndc reconfig'.
next_time=$((21600-time_passed))
check_next_key_event $next_time

#
# Zone: step3.algorithm-roll.kasp
#
set_zone "step3.algorithm-roll.kasp"
set_policy "ecdsa256" "4" "3600"
set_server "ns6" "10.53.0.6"
# The RSAHSHA1 keys are outroducing, and it is time to swap the DS.
set_keystate "KEY1" "STATE_DS"     "unretentive"
# The ECDSAP256SHA256 keys are introducing. The DNSKEY RRset and all signatures
# are now omnipresent, so the DS can be introduced.
set_keystate "KEY3" "STATE_DS"     "rumoured"
set_keystate "KEY4" "STATE_ZRRSIG" "omnipresent"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the DS becomes OMNIPRESENT. This happens after the
# parent registration delay, parent propagation delay, retire safety delay,
# and DS TTL: 24h + 1h + 2h + 2h = 29h = 104400 seconds.
check_next_key_event 104400

#
# Zone: step4.algorithm-roll.kasp
#
set_zone "step4.algorithm-roll.kasp"
set_policy "ecdsa256" "4" "3600"
set_server "ns6" "10.53.0.6"
# The old DS is HIDDEN, we can remove the old algorithm DNSKEY/RRSIG records.
set_keysigning   "KEY1" "no"
set_keystate     "KEY1" "STATE_DNSKEY" "unretentive"
set_keystate     "KEY1" "STATE_KRRSIG" "unretentive"
set_keystate     "KEY1" "STATE_DS"     "hidden"

set_zonesigning  "KEY2" "no"
set_keystate     "KEY2" "GOAL"         "hidden"
set_keystate     "KEY2" "STATE_DNSKEY" "unretentive"
set_keystate     "KEY2" "STATE_ZRRSIG" "unretentive"
# The ECDSAP256SHA256 DS is now OMNIPRESENT.
set_keystate     "KEY3" "STATE_DS"     "omnipresent"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the old DNSKEY becomes HIDDEN.  This happens after the
# DNSKEY TTL plus zone propagation delay (2h).
check_next_key_event 7200

#
# Zone: step5.algorithm-roll.kasp
#
set_zone "step5.algorithm-roll.kasp"
set_policy "ecdsa256" "4" "3600"
set_server "ns6" "10.53.0.6"
# The DNSKEY becomes HIDDEN.
set_keystate "KEY1" "STATE_DNSKEY" "hidden"
set_keystate "KEY1" "STATE_KRRSIG" "hidden"
set_keystate "KEY2" "STATE_DNSKEY" "hidden"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the RSASHA1 signatures become HIDDEN.  This happens
# after the max-zone-ttl plus zone propagation delay plus retire safety
# (6h + 1h + 2h) minus the time already passed since the UNRETENTIVE state has
# been reached (2h): 9h - 2h = 7h = 25200 seconds. Prevent intermittent
# false positives on slow platforms by subtracting the number of seconds
# which passed between key creation and invoking 'rndc reconfig'.
next_time=$((25200-time_passed))
check_next_key_event $next_time

#
# Zone: step6.algorithm-roll.kasp
#
set_zone "step6.algorithm-roll.kasp"
set_policy "ecdsa256" "4" "3600"
set_server "ns6" "10.53.0.6"
# The old zone signatures (KEY2) should now also be HIDDEN.
set_keystate "KEY2" "STATE_ZRRSIG" "hidden"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is never since we established the policy and the keys have
# an unlimited lifetime.  Fallback to the default loadkeys interval.
check_next_key_event 3600

#
# Testing CSK algorithm rollover.
#

#
# Zone: step1.csk-algorithm-roll.kasp
#
set_zone "step1.csk-algorithm-roll.kasp"
set_policy "csk-algoroll" "2" "3600"
set_server "ns6" "10.53.0.6"
# Old RSASHA1 key.
key_clear	 "KEY1"
set_keyrole      "KEY1" "csk"
set_keylifetime  "KEY1" "0"
set_keyalgorithm "KEY1" "5" "RSASHA1" "2048"
set_keysigning   "KEY1" "yes"
set_zonesigning  "KEY1" "yes"
# New ECDSAP256SHA256 key.
key_clear	 "KEY2"
set_keyrole      "KEY2" "csk"
set_keylifetime  "KEY2" "0"
set_keyalgorithm "KEY2" "13" "ECDSAP256SHA256" "256"
set_keysigning   "KEY2" "yes"
set_zonesigning  "KEY2" "yes"
key_clear "KEY3"
key_clear "KEY4"
# The RSAHSHA1 key is outroducing.
set_keytime  "KEY1" "PUBLISHED"    "yes"
set_keytime  "KEY1" "ACTIVE"       "yes"
set_keytime  "KEY1" "RETIRED"      "yes"
set_keystate "KEY1" "GOAL"         "hidden"
set_keystate "KEY1" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY1" "STATE_KRRSIG" "omnipresent"
set_keystate "KEY1" "STATE_ZRRSIG" "omnipresent"
set_keystate "KEY1" "STATE_DS"     "omnipresent"
# The ECDSAP256SHA256 key is introducing.
set_keytime  "KEY2" "PUBLISHED"    "yes"
set_keytime  "KEY2" "ACTIVE"       "yes"
set_keystate "KEY2" "GOAL"         "omnipresent"
set_keystate "KEY2" "STATE_DNSKEY" "rumoured"
set_keystate "KEY2" "STATE_KRRSIG" "rumoured"
set_keystate "KEY2" "STATE_ZRRSIG" "rumoured"
set_keystate "KEY2" "STATE_DS"     "hidden"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the new key has been propagated.
# This is the DNSKEY TTL plus publish safety plus zone propagation delay:
# 3 times an hour: 10800 seconds.
check_next_key_event 10800

#
# Zone: step2.csk-algorithm-roll.kasp
#
set_zone "step2.csk-algorithm-roll.kasp"
set_policy "csk-algoroll" "2" "3600"
set_server "ns6" "10.53.0.6"
# The RSAHSHA1 key is outroducing, but need to stay present until the new
# algorithm chain of trust has been established. Thus the properties, timings
# and states of KEY1 is the same as above.
#
# The ECDSAP256SHA256 keys are introducing. The DNSKEY RRset is omnipresent,
# but the zone signatures are not.
set_keystate "KEY2" "STATE_DNSKEY" "omnipresent"
set_keystate "KEY2" "STATE_KRRSIG" "omnipresent"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when all zone signatures are signed with the new
# algorithm.  This is the max-zone-ttl plus zone propagation delay
# plus retire safety: 6h + 1h + 2h.  But three hours have already passed
# (the time it took to make the DNSKEY omnipresent), so the next event
# should be scheduled in 6 hour: 21600 seconds.  Prevent intermittent
# false positives on slow platforms by subtracting the number of seconds
# which passed between key creation and invoking 'rndc reconfig'.
next_time=$((21600-time_passed))
check_next_key_event $next_time

#
# Zone: step3.csk-algorithm-roll.kasp
#
set_zone "step3.csk-algorithm-roll.kasp"
set_policy "csk-algoroll" "2" "3600"
set_server "ns6" "10.53.0.6"
# The RSAHSHA1 key is outroducing, and it is time to swap the DS.
set_keystate "KEY1" "STATE_DS"     "unretentive"
# The ECDSAP256SHA256 key is introducing. The DNSKEY RRset and all signatures
# are now omnipresent, so the DS can be introduced.
set_keystate "KEY2" "STATE_ZRRSIG" "omnipresent"
set_keystate "KEY2" "STATE_DS"     "rumoured"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the DS becomes OMNIPRESENT. This happens after the
# parent registration delay, parent propagation delay, retire safety delay,
# and DS TTL: 24h + 1h + 2h + 2h = 29h = 104400 seconds.
check_next_key_event 104400

#
# Zone: step4.csk-algorithm-roll.kasp
#
set_zone "step4.csk-algorithm-roll.kasp"
set_policy "csk-algoroll" "2" "3600"
set_server "ns6" "10.53.0.6"
# The old DS is HIDDEN, we can remove the old algorithm DNSKEY/RRSIG records.
set_keysigning   "KEY1" "no"
set_zonesigning  "KEY1" "no"
set_keystate     "KEY1" "STATE_DNSKEY" "unretentive"
set_keystate     "KEY1" "STATE_KRRSIG" "unretentive"
set_keystate     "KEY1" "STATE_ZRRSIG" "unretentive"
set_keystate     "KEY1" "STATE_DS"     "hidden"
# The ECDSAP256SHA256 DS is now OMNIPRESENT.
set_keystate     "KEY2" "STATE_DS"     "omnipresent"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the old DNSKEY becomes HIDDEN.  This happens after the
# DNSKEY TTL plus zone propagation delay (2h).
check_next_key_event 7200

#
# Zone: step5.csk-algorithm-roll.kasp
#
set_zone "step5.csk-algorithm-roll.kasp"
set_policy "csk-algoroll" "2" "3600"
set_server "ns6" "10.53.0.6"
# The DNSKEY becomes HIDDEN.
set_keystate "KEY1" "STATE_DNSKEY" "hidden"
set_keystate "KEY1" "STATE_KRRSIG" "hidden"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is when the RSASHA1 signatures become HIDDEN.  This happens
# after the max-zone-ttl plus zone propagation delay plus retire safety
# (6h + 1h + 2h) minus the time already passed since the UNRETENTIVE state has
# been reached (2h): 9h - 2h = 7h = 25200 seconds.  Prevent intermittent
# false positives on slow platforms by subtracting the number of seconds
# which passed between key creation and invoking 'rndc reconfig'.
next_time=$((25200-time_passed))
check_next_key_event $next_time

#
# Zone: step6.csk-algorithm-roll.kasp
#
set_zone "step6.csk-algorithm-roll.kasp"
set_policy "csk-algoroll" "2" "3600"
set_server "ns6" "10.53.0.6"
# The zone signatures should now also be HIDDEN.
set_keystate "KEY1" "STATE_ZRRSIG" "hidden"

check_keys
check_apex
check_subdomain
dnssec_verify

# Next key event is never since we established the policy and the keys have
# an unlimited lifetime.  Fallback to the default loadkeys interval.
check_next_key_event 3600

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
