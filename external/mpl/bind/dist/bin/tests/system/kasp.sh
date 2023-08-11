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

#
# Common configuration data for kasp system tests, to be sourced into
# other shell scripts.
#

# shellcheck source=conf.sh
. ../conf.sh

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
VIEW3="C1Azf+gGPMmxrUg/WQINP6eV9Y0="

###############################################################################
# Key properties                                                              #
###############################################################################
# ID
# BASEFILE
# EXPECT
# ROLE
# KSK
# ZSK
# FLAGS
# LIFETIME
# ALG_NUM
# ALG_STR
# ALG_LEN
# CREATED
# PUBLISHED
# ACTIVE
# RETIRED
# REVOKED
# REMOVED
# GOAL
# STATE_DNSKEY
# STATE_ZRRSIG
# STATE_KRRSIG
# STATE_DS
# EXPECT_ZRRSIG
# EXPECT_KRRSIG
# LEGACY
# PRIVATE
# PRIVKEY_STAT
# PUBKEY_STAT
# STATE_STAT

key_key() {
	echo "${1}__${2}"
}

key_get() {
	eval "echo \${$(key_key "$1" "$2")}"
}

key_set() {
	eval "$(key_key "$1" "$2")='$3'"
}

key_stat() {
	$PERL -e 'print((stat @ARGV[0])[9] . "\n");' "$1"
}

# Save certain values in the KEY array.
key_save()
{
	# Save key id.
	key_set "$1" ID "$KEY_ID"
	# Save base filename.
	key_set "$1" BASEFILE "$BASE_FILE"
	# Save creation date.
	key_set "$1" CREATED "${KEY_CREATED}"
	# Save key change time.
	key_set "$1" PRIVKEY_STAT $(key_stat "${BASE_FILE}.private")
	key_set "$1" PUBKEY_STAT $(key_stat "${BASE_FILE}.key")
	key_set "$1" STATE_STAT $(key_stat "${BASE_FILE}.state")
}

# Clear key state.
#
# This will update either the KEY1, KEY2, or KEY3 array.
key_clear() {
	key_set "$1" "ID" 'no'
	key_set "$1" "IDPAD" 'no'
	key_set "$1" "EXPECT" 'no'
	key_set "$1" "ROLE" 'none'
	key_set "$1" "KSK" 'no'
	key_set "$1" "ZSK" 'no'
	key_set "$1" "FLAGS" '0'
	key_set "$1" "LIFETIME" 'none'
	key_set "$1" "ALG_NUM" '0'
	key_set "$1" "ALG_STR" 'none'
	key_set "$1" "ALG_LEN" '0'
	key_set "$1" "CREATED" '0'
	key_set "$1" "PUBLISHED" 'none'
	key_set "$1" "SYNCPUBLISH" 'none'
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
	key_set "$1" "PRIVATE" 'yes'
	key_set "$1" "PRIVKEY_STAT" '0'
	key_set "$1" "PUBKEY_STAT" '0'
	key_set "$1" "STATE_STAT" '0'
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
_dig_with_opts() {

	if [ -n "$TSIG" ]; then
		"$DIG" +tcp +noadd +nosea +nostat +nocmd +dnssec -p "$PORT" -y "$TSIG" "$@"
	else
		"$DIG" +tcp +noadd +nosea +nostat +nocmd +dnssec -p "$PORT" "$@"
	fi
}

# RNDC.
_rndccmd() {
	"$RNDC" -c ../common/rndc.conf -p "$CONTROLPORT" -s "$@"
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
_log_error() {
	test $_log -eq 1 && echo_i "error: $1"
	ret=$((ret+1))
}
disable_logerror() {
	_log=0
}
enable_logerror() {
	_log=1
}

# Set server key-directory ($1) and address ($2) for testing keys.
set_server() {
	DIR=$1
	SERVER=$2
}
# Set zone name for testing keys.
set_zone() {
	ZONE=$1
	DYNAMIC="no"
}
# By default zones are considered static.
# When testing dynamic zones, call 'set_dynamic' after 'set_zone'.
set_dynamic() {
	DYNAMIC="yes"
}

# Set policy settings (name $1, number of keys $2, dnskey ttl $3) for testing keys.
set_policy() {
	POLICY=$1
	NUM_KEYS=$2
	DNSKEY_TTL=$3
	CDS_DELETE="no"
}
# By default policies are considered to be secure.
# If a zone sets its policy to "insecure", call 'set_cdsdelete' to tell the
# system test to expect a CDS and CDNSKEY Delete record.
set_cdsdelete() {
	CDS_DELETE="yes"
}

# Set key properties for testing keys.
# $1: Key to update (KEY1, KEY2, ...)
# $2: Value
set_keyrole() {
	key_set "$1" "EXPECT" "yes"
	key_set "$1" "ROLE" "$2"
	key_set "$1" "KSK" "no"
	key_set "$1" "ZSK" "no"
	key_set "$1" "FLAGS" "0"

	test "$2" = "ksk" && key_set "$1" "KSK" "yes"
	test "$2" = "ksk" && key_set "$1" "FLAGS" "257"

	test "$2" = "zsk" && key_set "$1" "ZSK" "yes"
	test "$2" = "zsk" && key_set "$1" "FLAGS" "256"

	test "$2" = "csk" && key_set "$1" "KSK" "yes"
	test "$2" = "csk" && key_set "$1" "ZSK" "yes"
	test "$2" = "csk" && key_set "$1" "FLAGS" "257"
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
# $1: Key to update (KEY1, KEY2, ...)
# $2: Time to update (PUBLISHED, SYNCPUBLISH, ACTIVE, RETIRED, REVOKED, or REMOVED).
# $3: Value
set_keytime() {
	key_set "$1" "EXPECT" "yes"
	key_set "$1" "$2" "$3"
}

# Set key timing metadata to a value plus additional time.
# $1: Key to update (KEY1, KEY2, ...)
# $2: Time to update (PUBLISHED, SYNCPUBLISH, ACTIVE, RETIRED, REVOKED, or REMOVED).
# $3: Value
# $4: Additional time.
set_addkeytime() {
	if [ -x "$PYTHON" ]; then
		# Convert "%Y%m%d%H%M%S" format to epoch seconds.
		# Then, add the additional time (can be negative).
		_value=$3
		_plus=$4
		$PYTHON > python.out.$ZONE.$1.$2 <<EOF
from datetime import datetime
from datetime import timedelta
_now = datetime.strptime("$_value", "%Y%m%d%H%M%S")
_delta = timedelta(seconds=$_plus)
_then = _now + _delta
print(_then.strftime("%Y%m%d%H%M%S"));
EOF
		# Set the expected timing metadata.
		key_set "$1" "$2" $(cat python.out.$ZONE.$1.$2)
	fi
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
# KEY_CREATED (from the KEY_FILE)
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
	_private=$(key_get "$1" PRIVATE)
	_flags=$(key_get "$1" FLAGS)

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
		_ksk="yes"
	elif [ "$_role" = "zsk" ]; then
		_zsk="yes"
	elif [ "$_role" = "csk" ]; then
		_zsk="yes"
		_ksk="yes"
	fi

	_role2="none"
	if [ "$_flags" = "257" ]; then
		_role2="key-signing"
	elif [ "$_flags" = "256" ]; then
		_role2="zone-signing"
	fi

	BASE_FILE="${_dir}/K${_zone}.+${_alg_numpad}+${_key_idpad}"
	KEY_FILE="${BASE_FILE}.key"
	PRIVATE_FILE="${BASE_FILE}.private"
	STATE_FILE="${BASE_FILE}.state"
	KEY_ID="${_key_id}"

	# Check file existence.
	[ -s "$KEY_FILE" ] || ret=1
	if [ "$_private" = "yes" ]; then
		[ -s "$PRIVATE_FILE" ] || ret=1
	fi
	if [ "$_legacy" = "no" ]; then
		[ -s "$STATE_FILE" ] || ret=1
	fi
	[ "$ret" -eq 0 ] || _log_error "${BASE_FILE} files missing"
	[ "$ret" -eq 0 ] || return

	# Retrieve creation date.
	grep "; Created:" "$KEY_FILE" > "${ZONE}.${KEY_ID}.${_alg_num}.created" || _log_error "mismatch created comment in $KEY_FILE"
	KEY_CREATED=$(awk '{print $3}' < "${ZONE}.${KEY_ID}.${_alg_num}.created")

	if [ "$_private" = "yes" ]; then
		grep "Created: ${KEY_CREATED}" "$PRIVATE_FILE" > /dev/null || _log_error "mismatch created in $PRIVATE_FILE"
	fi
	if [ "$_legacy" = "no" ]; then
		grep "Generated: ${KEY_CREATED}" "$STATE_FILE" > /dev/null || _log_error "mismatch generated in $STATE_FILE"
	fi

	test $_log -eq 1 && echo_i "check key file $BASE_FILE"

	# Check the public key file.
	grep "This is a ${_role2} key, keyid ${_key_id}, for ${_zone}." "$KEY_FILE" > /dev/null || _log_error "mismatch top comment in $KEY_FILE"
	grep "${_zone}\. ${_dnskey_ttl} IN DNSKEY ${_flags} 3 ${_alg_num}" "$KEY_FILE" > /dev/null || _log_error "mismatch DNSKEY record in $KEY_FILE"
	# Now check the private key file.
	if [ "$_private" = "yes" ]; then
		grep "Private-key-format: v1.3" "$PRIVATE_FILE" > /dev/null || _log_error "mismatch private key format in $PRIVATE_FILE"
		grep "Algorithm: ${_alg_num} (${_alg_string})" "$PRIVATE_FILE" > /dev/null || _log_error "mismatch algorithm in $PRIVATE_FILE"
	fi
	# Now check the key state file.
	if [ "$_legacy" = "no" ]; then
		grep "This is the state of key ${_key_id}, for ${_zone}." "$STATE_FILE" > /dev/null || _log_error "mismatch top comment in $STATE_FILE"
		if [ "$_lifetime" = "none" ]; then
			grep "Lifetime: " "$STATE_FILE" > /dev/null && _log_error "unexpected lifetime in $STATE_FILE"
		else
			grep "Lifetime: ${_lifetime}" "$STATE_FILE" > /dev/null || _log_error "mismatch lifetime in $STATE_FILE"
		fi
		grep "Algorithm: ${_alg_num}" "$STATE_FILE" > /dev/null || _log_error "mismatch algorithm in $STATE_FILE"
		grep "Length: ${_length}" "$STATE_FILE" > /dev/null || _log_error "mismatch length in $STATE_FILE"
		grep "KSK: ${_ksk}" "$STATE_FILE" > /dev/null || _log_error "mismatch ksk in $STATE_FILE"
		grep "ZSK: ${_zsk}" "$STATE_FILE" > /dev/null || _log_error "mismatch zsk in $STATE_FILE"

		# Check key states.
		if [ "$_goal" = "none" ]; then
			grep "GoalState: " "$STATE_FILE" > /dev/null && _log_error "unexpected goal state in $STATE_FILE"
		else
			grep "GoalState: ${_goal}" "$STATE_FILE" > /dev/null || _log_error "mismatch goal state in $STATE_FILE"
		fi

		if [ "$_state_dnskey" = "none" ]; then
			grep "DNSKEYState: " "$STATE_FILE" > /dev/null && _log_error "unexpected dnskey state in $STATE_FILE"
			grep "DNSKEYChange: " "$STATE_FILE" > /dev/null && _log_error "unexpected dnskey change in $STATE_FILE"
		else
			grep "DNSKEYState: ${_state_dnskey}" "$STATE_FILE" > /dev/null || _log_error "mismatch dnskey state in $STATE_FILE"
			grep "DNSKEYChange: " "$STATE_FILE" > /dev/null || _log_error "mismatch dnskey change in $STATE_FILE"
		fi

		if [ "$_state_zrrsig" = "none" ]; then
			grep "ZRRSIGState: " "$STATE_FILE" > /dev/null && _log_error "unexpected zrrsig state in $STATE_FILE"
			grep "ZRRSIGChange: " "$STATE_FILE" > /dev/null && _log_error "unexpected zrrsig change in $STATE_FILE"
		else
			grep "ZRRSIGState: ${_state_zrrsig}" "$STATE_FILE" > /dev/null || _log_error "mismatch zrrsig state in $STATE_FILE"
			grep "ZRRSIGChange: " "$STATE_FILE" > /dev/null || _log_error "mismatch zrrsig change in $STATE_FILE"
		fi

		if [ "$_state_krrsig" = "none" ]; then
			grep "KRRSIGState: " "$STATE_FILE" > /dev/null && _log_error "unexpected krrsig state in $STATE_FILE"
			grep "KRRSIGChange: " "$STATE_FILE" > /dev/null && _log_error "unexpected krrsig change in $STATE_FILE"
		else
			grep "KRRSIGState: ${_state_krrsig}" "$STATE_FILE" > /dev/null || _log_error "mismatch krrsig state in $STATE_FILE"
			grep "KRRSIGChange: " "$STATE_FILE" > /dev/null || _log_error "mismatch krrsig change in $STATE_FILE"
		fi

		if [ "$_state_ds" = "none" ]; then
			grep "DSState: " "$STATE_FILE" > /dev/null && _log_error "unexpected ds state in $STATE_FILE"
			grep "DSChange: " "$STATE_FILE" > /dev/null && _log_error "unexpected ds change in $STATE_FILE"
		else
			grep "DSState: ${_state_ds}" "$STATE_FILE" > /dev/null || _log_error "mismatch ds state in $STATE_FILE"
			grep "DSChange: " "$STATE_FILE" > /dev/null || _log_error "mismatch ds change in $STATE_FILE"
		fi
	fi
}

# Check the key timing metadata for key $1.
check_timingmetadata() {
	_dir="$DIR"
	_zone="$ZONE"
	_key_idpad=$(key_get "$1" ID)
	_key_id=$(echo "$_key_idpad" | sed 's/^0\{0,4\}//')
	_alg_num=$(key_get "$1" ALG_NUM)
	_alg_numpad=$(printf "%03d" "$_alg_num")

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

	_base_file=$(key_get "$1" BASEFILE)
	_key_file="${_base_file}.key"
	_private_file="${_base_file}.private"
	_state_file="${_base_file}.state"
	_legacy=$(key_get "$1" LEGACY)
	_private=$(key_get "$1" PRIVATE)

	_published=$(key_get "$1" PUBLISHED)
	_syncpublish=$(key_get "$1" SYNCPUBLISH)
	_active=$(key_get "$1" ACTIVE)
	_retired=$(key_get "$1" RETIRED)
	_revoked=$(key_get "$1" REVOKED)
	_removed=$(key_get "$1" REMOVED)

	# Check timing metadata.
	n=$((n+1))
	echo_i "check key timing metadata for key $1 id ${_key_id} zone ${ZONE} ($n)"
	ret=0

	if [ "$_published" = "none" ]; then
		grep "; Publish:" "${_key_file}" > /dev/null && _log_error "unexpected publish comment in ${_key_file}"
		if [ "$_private" = "yes" ]; then
			grep "Publish:" "${_private_file}" > /dev/null && _log_error "unexpected publish in ${_private_file}"
		fi
		if [ "$_legacy" = "no" ]; then
			grep "Published: " "${_state_file}" > /dev/null && _log_error "unexpected publish in ${_state_file}"
		fi
	else
		grep "; Publish: $_published" "${_key_file}" > /dev/null || _log_error "mismatch publish comment in ${_key_file} (expected ${_published})"
		if [ "$_private" = "yes" ]; then
			grep "Publish: $_published" "${_private_file}" > /dev/null || _log_error "mismatch publish in ${_private_file} (expected ${_published})"
		fi
		if [ "$_legacy" = "no" ]; then
			grep "Published: $_published" "${_state_file}" > /dev/null || _log_error "mismatch publish in ${_state_file} (expected ${_published})"
		fi
	fi

	if [ "$_syncpublish" = "none" ]; then
		grep "; SyncPublish:" "${_key_file}" > /dev/null && _log_error "unexpected syncpublish comment in ${_key_file}"
		if [ "$_private" = "yes" ]; then
			grep "SyncPublish:" "${_private_file}" > /dev/null && _log_error "unexpected syncpublish in ${_private_file}"
		fi
		if [ "$_legacy" = "no" ]; then
			grep "PublishCDS: " "${_state_file}" > /dev/null && _log_error "unexpected syncpublish in ${_state_file}"
		fi
	else
		grep "; SyncPublish: $_syncpublish" "${_key_file}" > /dev/null || _log_error "mismatch syncpublish comment in ${_key_file} (expected ${_syncpublish})"
		if [ "$_private" = "yes" ]; then
			grep "SyncPublish: $_syncpublish" "${_private_file}" > /dev/null || _log_error "mismatch syncpublish in ${_private_file} (expected ${_syncpublish})"
		fi
		if [ "$_legacy" = "no" ]; then
			grep "PublishCDS: $_syncpublish" "${_state_file}" > /dev/null || _log_error "mismatch syncpublish in ${_state_file} (expected ${_syncpublish})"
		fi
	fi

	if [ "$_active" = "none" ]; then
		grep "; Activate:" "${_key_file}" > /dev/null && _log_error "unexpected active comment in ${_key_file}"
		if [ "$_private" = "yes" ]; then
			grep "Activate:" "${_private_file}" > /dev/null && _log_error "unexpected active in ${_private_file}"
		fi
		if [ "$_legacy" = "no" ]; then
			grep "Active: " "${_state_file}" > /dev/null && _log_error "unexpected active in ${_state_file}"
		fi
	else
		grep "; Activate: $_active" "${_key_file}" > /dev/null || _log_error "mismatch active comment in ${_key_file} (expected ${_active})"
		if [ "$_private" = "yes" ]; then
			grep "Activate: $_active" "${_private_file}" > /dev/null || _log_error "mismatch active in ${_private_file} (expected ${_active})"
		fi
		if [ "$_legacy" = "no" ]; then
			grep "Active: $_active" "${_state_file}" > /dev/null || _log_error "mismatch active in ${_state_file} (expected ${_active})"
		fi
	fi

	if [ "$_retired" = "none" ]; then
		grep "; Inactive:" "${_key_file}" > /dev/null && _log_error "unexpected retired comment in ${_key_file}"
		if [ "$_private" = "yes" ]; then
			grep "Inactive:" "${_private_file}" > /dev/null && _log_error "unexpected retired in ${_private_file}"
		fi
		if [ "$_legacy" = "no" ]; then
			grep "Retired: " "${_state_file}" > /dev/null && _log_error "unexpected retired in ${_state_file}"
		fi
	else
		grep "; Inactive: $_retired" "${_key_file}" > /dev/null || _log_error "mismatch retired comment in ${_key_file} (expected ${_retired})"
		if [ "$_private" = "yes" ]; then
			grep "Inactive: $_retired" "${_private_file}" > /dev/null || _log_error "mismatch retired in ${_private_file} (expected ${_retired})"
		fi
		if [ "$_legacy" = "no" ]; then
			grep "Retired: $_retired" "${_state_file}" > /dev/null || _log_error "mismatch retired in ${_state_file} (expected ${_retired})"
		fi
	fi

	if [ "$_revoked" = "none" ]; then
		grep "; Revoke:" "${_key_file}" > /dev/null && _log_error "unexpected revoked comment in ${_key_file}"
		if [ "$_private" = "yes" ]; then
			grep "Revoke:" "${_private_file}" > /dev/null && _log_error "unexpected revoked in ${_private_file}"
		fi
		if [ "$_legacy" = "no" ]; then
			grep "Revoked: " "${_state_file}" > /dev/null && _log_error "unexpected revoked in ${_state_file}"
		fi
	else
		grep "; Revoke: $_revoked" "${_key_file}" > /dev/null || _log_error "mismatch revoked comment in ${_key_file} (expected ${_revoked})"
		if [ "$_private" = "yes" ]; then
			grep "Revoke: $_revoked" "${_private_file}" > /dev/null || _log_error "mismatch revoked in ${_private_file} (expected ${_revoked})"
		fi
		if [ "$_legacy" = "no" ]; then
			grep "Revoked: $_revoked" "${_state_file}" > /dev/null || _log_error "mismatch revoked in ${_state_file} (expected ${_revoked})"
		fi
	fi

	if [ "$_removed" = "none" ]; then
		grep "; Delete:" "${_key_file}" > /dev/null && _log_error "unexpected removed comment in ${_key_file}"
		if [ "$_private" = "yes" ]; then
			grep "Delete:" "${_private_file}" > /dev/null && _log_error "unexpected removed in ${_private_file}"
		fi
		if [ "$_legacy" = "no" ]; then
			grep "Removed: " "${_state_file}" > /dev/null && _log_error "unexpected removed in ${_state_file}"
		fi
	else
		grep "; Delete: $_removed" "${_key_file}" > /dev/null || _log_error "mismatch removed comment in ${_key_file} (expected ${_removed})"
		if [ "$_private" = "yes" ]; then
			grep "Delete: $_removed" "${_private_file}" > /dev/null || _log_error "mismatch removed in ${_private_file} (expected ${_removed})"
		fi
		if [ "$_legacy" = "no" ]; then
			grep "Removed: $_removed" "${_state_file}" > /dev/null || _log_error "mismatch removed in ${_state_file} (expected ${_removed})"
		fi
	fi

	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}

check_keytimes() {
	# The script relies on Python to set keytimes.
	if [ -x "$PYTHON" ]; then

		if [ "$(key_get KEY1 EXPECT)" = "yes" ]; then
			check_timingmetadata "KEY1"
		fi
		if [ "$(key_get KEY2 EXPECT)" = "yes" ]; then
			check_timingmetadata "KEY2"
		fi
		if [ "$(key_get KEY3 EXPECT)" = "yes" ]; then
			check_timingmetadata "KEY3"
		fi
		if [ "$(key_get KEY4 EXPECT)" = "yes" ]; then
			check_timingmetadata "KEY4"
		fi
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

	# Treat keys that have been removed from the zone as unused.
	_check_removed=1
	grep "; Created:" "$KEY_FILE" > created.key-${KEY_ID}.test${n} || _check_removed=0
	grep "; Delete:" "$KEY_FILE" > unused.key-${KEY_ID}.test${n} || _check_removed=0
	if [ "$_check_removed" -eq 1 ]; then
		_created=$(awk '{print $3}' < created.key-${KEY_ID}.test${n})
		_removed=$(awk '{print $3}' < unused.key-${KEY_ID}.test${n})
		[ "$_removed" -le "$_created" ] && return
	fi

	# If no timing metadata is set, this key is unused.
	grep "; Publish:" "$KEY_FILE" > /dev/null && _log_error "unexpected publish comment in $KEY_FILE"
	grep "; Activate:" "$KEY_FILE" > /dev/null && _log_error "unexpected active comment in $KEY_FILE"
	grep "; Inactive:" "$KEY_FILE" > /dev/null && _log_error "unexpected retired comment in $KEY_FILE"
	grep "; Revoke:" "$KEY_FILE" > /dev/null && _log_error "unexpected revoked comment in $KEY_FILE"
	grep "; Delete:" "$KEY_FILE" > /dev/null && _log_error "unexpected removed comment in $KEY_FILE"

	grep "Publish:" "$PRIVATE_FILE" > /dev/null && _log_error "unexpected publish in $PRIVATE_FILE"
	grep "Activate:" "$PRIVATE_FILE" > /dev/null && _log_error "unexpected active in $PRIVATE_FILE"
	grep "Inactive:" "$PRIVATE_FILE" > /dev/null && _log_error "unexpected retired in $PRIVATE_FILE"
	grep "Revoke:" "$PRIVATE_FILE" > /dev/null && _log_error "unexpected revoked in $PRIVATE_FILE"
	grep "Delete:" "$PRIVATE_FILE" > /dev/null && _log_error "unexpected removed in $PRIVATE_FILE"

	grep "Published: " "$STATE_FILE" > /dev/null && _log_error "unexpected publish in $STATE_FILE"
	grep "Active: " "$STATE_FILE" > /dev/null && _log_error "unexpected active in $STATE_FILE"
	grep "Retired: " "$STATE_FILE" > /dev/null && _log_error "unexpected retired in $STATE_FILE"
	grep "Revoked: " "$STATE_FILE" > /dev/null && _log_error "unexpected revoked in $STATE_FILE"
	grep "Removed: " "$STATE_FILE" > /dev/null && _log_error "unexpected removed in $STATE_FILE"
}

# Test: dnssec-verify zone $1.
dnssec_verify()
{
	n=$((n+1))
	echo_i "dnssec-verify zone ${ZONE} ($n)"
	ret=0
	_dig_with_opts "$ZONE" "@${SERVER}" AXFR > dig.out.axfr.test$n || _log_error "dig ${ZONE} AXFR failed"
	$VERIFY -z -o "$ZONE" dig.out.axfr.test$n > verify.out.$ZONE.test$n || _log_error "dnssec verify zone $ZONE failed"
	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}

# Wait for the zone to be signed.
# The apex NSEC record indicates that it is signed.
_wait_for_nsec() {
	_dig_with_opts "@${SERVER}" "$ZONE" NSEC > "dig.out.nsec.test$n" || return 1
	grep "NS SOA" "dig.out.nsec.test$n" > /dev/null || return 1
	grep "${ZONE}\..*IN.*RRSIG" "dig.out.nsec.test$n" > /dev/null || return 1
	return 0
}
wait_for_nsec() {
	n=$((n+1))
	ret=0
	echo_i "wait for ${ZONE} to be signed ($n)"
	retry_quiet 10 _wait_for_nsec  || _log_error "wait for ${ZONE} to be signed failed"
	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}

check_numkeys() {
	_numkeys=$(get_keyids "$DIR" "$ZONE" | wc -l)
	test "$_numkeys" -eq "$NUM_KEYS" || return 1
	return 0
}

_check_keys() {
	ret=0

	# Clear key ids.
	key_set KEY1 ID "no"
	key_set KEY2 ID "no"
	key_set KEY3 ID "no"
	key_set KEY4 ID "no"

	# Check key files.
	_ids=$(get_keyids "$DIR" "$ZONE")
	for _id in $_ids; do
		# There are multiple key files with the same algorithm.
		# Check them until a match is found.
		ret=0
		echo_i "check key id $_id"

		if [ "no" = "$(key_get KEY1 ID)" ] && [ "$(key_get KEY1 EXPECT)" = "yes" ]; then
			ret=0
			check_key "KEY1" "$_id"
			test "$ret" -eq 0 && key_save KEY1 && continue
		fi
		if [ "no" = "$(key_get KEY2 ID)" ] && [ "$(key_get KEY2 EXPECT)" = "yes" ]; then
			ret=0
			check_key "KEY2" "$_id"
			test "$ret" -eq 0 && key_save KEY2 && continue
		fi
		if [ "no" = "$(key_get KEY3 ID)" ] && [ "$(key_get KEY3 EXPECT)" = "yes"  ]; then
			ret=0
			check_key "KEY3" "$_id"
			test "$ret" -eq 0 && key_save KEY3 && continue
		fi
		if [ "no" = "$(key_get KEY4 ID)" ] && [ "$(key_get KEY4 EXPECT)" = "yes"  ]; then
			ret=0
			check_key "KEY4" "$_id"
			test "$ret" -eq 0 && key_save KEY4 && continue
		fi

		# This may be an unused key. Assume algorithm of KEY1.
		ret=0 && key_unused "$_id" "$(key_get KEY1 ALG_NUM)"
		test "$ret" -eq 0 && continue

		# If ret is still non-zero, none of the files matched.
		echo_i "failed"
		return 1
	done

	return 0
}

# Check keys for a configured zone. This verifies:
# 1. The right number of keys exist in the key pool ($1).
# 2. The right number of keys is active. Checks KEY1, KEY2, KEY3, and KEY4.
#
# It is expected that KEY1, KEY2, KEY3, and KEY4 arrays are set correctly.
# Found key identifiers are stored in the right key array.
check_keys() {
	n=$((n+1))
	echo_i "check keys are created for zone ${ZONE} ($n)"
	ret=0

	echo_i "check number of keys for zone ${ZONE} in dir ${DIR} ($n)"
	retry_quiet 10 check_numkeys || ret=1
	if [ $ret -ne 0 ]; then
		_numkeys=$(get_keyids "$DIR" "$ZONE" | wc -l)
		_log_error "bad number of key files ($_numkeys) for zone $ZONE (expected $NUM_KEYS)"
		status=$((status+ret))
	fi

	# Temporarily don't log errors because we are searching multiple files.
	disable_logerror

	retry_quiet 3 _check_keys || ret=1
	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))

	# Turn error logs on again.
	enable_logerror

	ret=0
	if [ "$(key_get KEY1 EXPECT)" = "yes" ]; then
		echo_i "KEY1 ID $(key_get KEY1 ID)"
		test "no" = "$(key_get KEY1 ID)" && _log_error "No KEY1 found for zone ${ZONE}"
	fi
	if [ "$(key_get KEY2 EXPECT)" = "yes" ]; then
		echo_i "KEY2 ID $(key_get KEY2 ID)"
		test "no" = "$(key_get KEY2 ID)" && _log_error "No KEY2 found for zone ${ZONE}"
	fi
	if [ "$(key_get KEY3 EXPECT)" = "yes" ]; then
		echo_i "KEY3 ID $(key_get KEY3 ID)"
		test "no" = "$(key_get KEY3 ID)" && _log_error "No KEY3 found for zone ${ZONE}"
	fi
	if [ "$(key_get KEY4 EXPECT)" = "yes" ]; then
		echo_i "KEY4 ID $(key_get KEY4 ID)"
		test "no" = "$(key_get KEY4 ID)" && _log_error "No KEY4 found for zone ${ZONE}"
	fi
	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}

# Call rndc dnssec -status on server $1 for zone $3 in view $4 with policy $2
# and check output. This is a loose verification, it just tests if the right
# policy name is returned, and if all expected keys are listed.  The rndc
# dnssec -status output also lists whether a key is published,
# used for signing, is retired, or is removed, and if not when
# it is scheduled to do so, and it shows the states for the various
# DNSSEC records.
check_dnssecstatus() {
	_server=$1
	_policy=$2
	_zone=$3
	_view=$4

	n=$((n+1))
	echo_i "check rndc dnssec -status output for ${_zone} (policy: $_policy) ($n)"
	ret=0

	_rndccmd $_server dnssec -status $_zone in $_view > rndc.dnssec.status.out.$_zone.$n || _log_error "rndc dnssec -status zone ${_zone} failed"

	if [ "$_policy" = "none" ]; then
		grep "Zone does not have dnssec-policy" rndc.dnssec.status.out.$_zone.$n > /dev/null || log_error "bad dnssec status for unsigned zone ${_zone}"
	else
		grep "dnssec-policy: ${_policy}" rndc.dnssec.status.out.$_zone.$n > /dev/null || _log_error "bad dnssec status for signed zone ${_zone}"
		if [ "$(key_get KEY1 EXPECT)" = "yes" ]; then
			grep "key: $(key_get KEY1 ID)" rndc.dnssec.status.out.$_zone.$n > /dev/null || _log_error "missing key $(key_get KEY1 ID) from dnssec status"
		fi
		if [ "$(key_get KEY2 EXPECT)" = "yes" ]; then
			grep "key: $(key_get KEY2 ID)" rndc.dnssec.status.out.$_zone.$n > /dev/null || _log_error "missing key $(key_get KEY2 ID) from dnssec status"
		fi
		if [ "$(key_get KEY3 EXPECT)" = "yes" ]; then
			grep "key: $(key_get KEY3 ID)" rndc.dnssec.status.out.$_zone.$n > /dev/null || _log_error "missing key $(key_get KEY3 ID) from dnssec status"
		fi
		if [ "$(key_get KEY4 EXPECT)" = "yes" ]; then
			grep "key: $(key_get KEY4 ID)" rndc.dnssec.status.out.$_zone.$n > /dev/null || _log_error "missing key $(key_get KEY4 ID) from dnssec status"
		fi
	fi

	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}

# Call rndc zonestatus on server $1 for zone $2 in view $3 and check output if
# inline-signing is enabled.
check_inlinesigning() {
	_server=$1
	_zone=$2
	_view=$3

	_rndccmd $_server zonestatus $_zone in $_view > rndc.zonestatus.out.$_zone.$n || return 1
	grep "inline signing: yes" rndc.zonestatus.out.$_zone.$n > /dev/null || return 1
}

# Call rndc zonestatus on server $1 for zone $2 in view $3 and check output if
# the zone is dynamic.
check_isdynamic() {
	_server=$1
	_zone=$2
	_view=$3

	_rndccmd $_server zonestatus $_zone in $_view > rndc.zonestatus.out.$_zone.$n || return 1
	grep "dynamic: yes" rndc.zonestatus.out.$_zone.$n > /dev/null || return 1
}

# Check if RRset of type $1 in file $2 is signed with the right keys.
# The right keys are the ones that expect a signature and matches the role $3.
_check_signatures() {
	_qtype=$1
	_file=$2
	_role=$3

	numsigs=0

	if [ "$_role" = "KSK" ]; then
		_expect_type=EXPECT_KRRSIG
	elif [ "$_role" = "ZSK" ]; then
		_expect_type=EXPECT_ZRRSIG
	fi

	if [ "$(key_get KEY1 "$_expect_type")" = "yes" ] && [ "$(key_get KEY1 "$_role")" = "yes" ]; then
		get_keys_which_signed "$_qtype" "$_file" | grep "^$(key_get KEY1 ID)$" > /dev/null || return 1
		numsigs=$((numsigs+1))
	elif [ "$(key_get KEY1 EXPECT)" = "yes" ]; then
		get_keys_which_signed "$_qtype" "$_file" | grep "^$(key_get KEY1 ID)$" > /dev/null && return 1
	fi

	if [ "$(key_get KEY2 "$_expect_type")" = "yes" ] && [ "$(key_get KEY2 "$_role")" = "yes" ]; then
		get_keys_which_signed "$_qtype" "$_file" | grep "^$(key_get KEY2 ID)$" > /dev/null || return 1
		numsigs=$((numsigs+1))
	elif [ "$(key_get KEY2 EXPECT)" = "yes" ]; then
		get_keys_which_signed "$_qtype" "$_file" | grep "^$(key_get KEY2 ID)$" > /dev/null && return 1
	fi

	if [ "$(key_get KEY3 "$_expect_type")" = "yes" ] && [ "$(key_get KEY3 "$_role")" = "yes" ]; then
		get_keys_which_signed "$_qtype" "$_file" | grep "^$(key_get KEY3 ID)$" > /dev/null || return 1
		numsigs=$((numsigs+1))
	elif [ "$(key_get KEY3 EXPECT)" = "yes" ]; then
		get_keys_which_signed "$_qtype" "$_file" | grep "^$(key_get KEY3 ID)$" > /dev/null && return 1
	fi

	if [ "$(key_get KEY4 "$_expect_type")" = "yes" ] && [ "$(key_get KEY4 "$_role")" = "yes" ]; then
		get_keys_which_signed "$_qtype" "$_file" | grep "^$(key_get KEY4 ID)$" > /dev/null || return 1
		numsigs=$((numsigs+1))
	elif [ "$(key_get KEY4 EXPECT)" = "yes" ]; then
		get_keys_which_signed "$_qtype" "$_file" | grep "^$(key_get KEY4 ID)$" > /dev/null && return 1
	fi

	lines=$(get_keys_which_signed "${_qtype}" "${_file}" | wc -l)
	test "$lines" -eq "$numsigs" || echo_i "bad number of signatures for $_qtype (got $lines, expected $numsigs)"
	test "$lines" -eq "$numsigs" || return 1

	return 0
}
check_signatures() {
	retry_quiet 3 _check_signatures $1 $2 $3 || _log_error "RRset $1 in zone $ZONE incorrectly signed"
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
	    -v flags="$(key_get "${1}" FLAGS)" \
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

	_checksig=0

	_dig_with_opts "$ZONE" "@${SERVER}" "CDS" > "dig.out.$DIR.test$n.cds" || _log_error "dig ${ZONE} CDS failed"
	grep "status: NOERROR" "dig.out.$DIR.test$n.cds" > /dev/null || _log_error "mismatch status in DNS response"

	_dig_with_opts "$ZONE" "@${SERVER}" "CDNSKEY" > "dig.out.$DIR.test$n.cdnskey" || _log_error "dig ${ZONE} CDNSKEY failed"
	grep "status: NOERROR" "dig.out.$DIR.test$n.cdnskey" > /dev/null || _log_error "mismatch status in DNS response"

	if [ "$CDS_DELETE" = "no" ]; then
		grep "CDS.*0 0 0 00" "dig.out.$DIR.test$n.cds" > /dev/null && _log_error "unexpected CDS DELETE record in DNS response"
		grep "CDNSKEY.*0 3 0 AA==" "dig.out.$DIR.test$n.cdnskey" > /dev/null && _log_error "unexpected CDNSKEY DELETE record in DNS response"
	else
		grep "CDS.*0 0 0 00" "dig.out.$DIR.test$n.cds" > /dev/null || _log_error "missing CDS DELETE record in DNS response"
		grep "CDNSKEY.*0 3 0 AA==" "dig.out.$DIR.test$n.cdnskey" > /dev/null || _log_error "missing CDNSKEY DELETE record in DNS response"
		_checksig=1
	fi

	if [ "$(key_get KEY1 STATE_DS)" = "rumoured" ] || [ "$(key_get KEY1 STATE_DS)" = "omnipresent" ]; then
		response_has_cds_for_key KEY1 "dig.out.$DIR.test$n.cds" || _log_error "missing CDS record in response for key $(key_get KEY1 ID)"
		response_has_cdnskey_for_key KEY1 "dig.out.$DIR.test$n.cdnskey" || _log_error "missing CDNSKEY record in response for key $(key_get KEY1 ID)"
		_checksig=1
	elif [ "$(key_get KEY1 EXPECT)" = "yes" ]; then
		response_has_cds_for_key KEY1 "dig.out.$DIR.test$n.cds" && _log_error "unexpected CDS record in response for key $(key_get KEY1 ID)"
		# KEY1 should not have an associated CDNSKEY, but there may be
		# one for another key.  Since the CDNSKEY has no field for key
		# id, it is hard to check what key the CDNSKEY may belong to
		# so let's skip this check for now.
	fi

	if [ "$(key_get KEY2 STATE_DS)" = "rumoured" ] || [ "$(key_get KEY2 STATE_DS)" = "omnipresent" ]; then
		response_has_cds_for_key KEY2 "dig.out.$DIR.test$n.cds" || _log_error "missing CDS record in response for key $(key_get KEY2 ID)"
		response_has_cdnskey_for_key KEY2 "dig.out.$DIR.test$n.cdnskey" || _log_error "missing CDNSKEY record in response for key $(key_get KEY2 ID)"
		_checksig=1
	elif [ "$(key_get KEY2 EXPECT)" = "yes" ]; then
		response_has_cds_for_key KEY2 "dig.out.$DIR.test$n.cds" && _log_error "unexpected CDS record in response for key $(key_get KEY2 ID)"
		# KEY2 should not have an associated CDNSKEY, but there may be
		# one for another key.  Since the CDNSKEY has no field for key
		# id, it is hard to check what key the CDNSKEY may belong to
		# so let's skip this check for now.
	fi

	if [ "$(key_get KEY3 STATE_DS)" = "rumoured" ] || [ "$(key_get KEY3 STATE_DS)" = "omnipresent" ]; then
		response_has_cds_for_key KEY3 "dig.out.$DIR.test$n.cds" || _log_error "missing CDS record in response for key $(key_get KEY3 ID)"
		response_has_cdnskey_for_key KEY3 "dig.out.$DIR.test$n.cdnskey" || _log_error "missing CDNSKEY record in response for key $(key_get KEY3 ID)"
		_checksig=1
	elif [ "$(key_get KEY3 EXPECT)" = "yes" ]; then
		response_has_cds_for_key KEY3 "dig.out.$DIR.test$n.cds" && _log_error "unexpected CDS record in response for key $(key_get KEY3 ID)"
		# KEY3 should not have an associated CDNSKEY, but there may be
		# one for another key.  Since the CDNSKEY has no field for key
		# id, it is hard to check what key the CDNSKEY may belong to
		# so let's skip this check for now.
	fi

	if [ "$(key_get KEY4 STATE_DS)" = "rumoured" ] || [ "$(key_get KEY4 STATE_DS)" = "omnipresent" ]; then
		response_has_cds_for_key KEY4 "dig.out.$DIR.test$n.cds" || _log_error "missing CDS record in response for key $(key_get KEY4 ID)"
		response_has_cdnskey_for_key KEY4 "dig.out.$DIR.test$n.cdnskey" || _log_error "missing CDNSKEY record in response for key $(key_get KEY4 ID)"
		_checksig=1
	elif [ "$(key_get KEY4 EXPECT)" = "yes" ]; then
		response_has_cds_for_key KEY4 "dig.out.$DIR.test$n.cds" && _log_error "unexpected CDS record in response for key $(key_get KEY4 ID)"
		# KEY4 should not have an associated CDNSKEY, but there may be
		# one for another key.  Since the CDNSKEY has no field for key
		# id, it is hard to check what key the CDNSKEY may belong to
		# so let's skip this check for now.
	fi

	test "$_checksig" -eq 0 || check_signatures "CDS" "dig.out.$DIR.test$n.cds" "KSK"
	test "$_checksig" -eq 0 || check_signatures "CDNSKEY" "dig.out.$DIR.test$n.cdnskey" "KSK"

	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}

_find_dnskey() {
	_owner="${ZONE}."
	_alg="$(key_get $1 ALG_NUM)"
	_flags="$(key_get $1 FLAGS)"
	_key_file="$(key_get $1 BASEFILE).key"

	awk '$1 == "'"$_owner"'" && $2 == "'"$DNSKEY_TTL"'" && $3 == "IN" && $4 == "DNSKEY" && $5 == "'"$_flags"'" && $6 == "3" && $7 == "'"$_alg"'" { print $8 }' < "$_key_file"
}


# Test DNSKEY query.
_check_apex_dnskey() {
	_dig_with_opts "$ZONE" "@${SERVER}" "DNSKEY" > "dig.out.$DIR.test$n" || return 1
	grep "status: NOERROR" "dig.out.$DIR.test$n" > /dev/null || return 1

	_checksig=0

	if [ "$(key_get KEY1 STATE_DNSKEY)" = "rumoured" ] || [ "$(key_get KEY1 STATE_DNSKEY)" = "omnipresent" ]; then
		_pubkey=$(_find_dnskey KEY1)
		test -z "$_pubkey" && return 1
		grep -F "$_pubkey" "dig.out.$DIR.test$n" > /dev/null || return 1
		_checksig=1
	elif [ "$(key_get KEY1 EXPECT)" = "yes" ]; then
		_pubkey=$(_find_dnskey KEY1)
		test -z "$_pubkey" && return 1
		grep -F "$_pubkey" "dig.out.$DIR.test$n" > /dev/null && return 1
	fi

	if [ "$(key_get KEY2 STATE_DNSKEY)" = "rumoured" ] || [ "$(key_get KEY2 STATE_DNSKEY)" = "omnipresent" ]; then
		_pubkey=$(_find_dnskey KEY2)
		test -z "$_pubkey" && return 1
		grep -F "$_pubkey" "dig.out.$DIR.test$n" > /dev/null || return 1
		_checksig=1
	elif [ "$(key_get KEY2 EXPECT)" = "yes" ]; then
		_pubkey=$(_find_dnskey KEY2)
		test -z "$_pubkey" && return 1
		grep -F "$_pubkey" "dig.out.$DIR.test$n" > /dev/null && return 1
	fi

	if [ "$(key_get KEY3 STATE_DNSKEY)" = "rumoured" ] || [ "$(key_get KEY3 STATE_DNSKEY)" = "omnipresent" ]; then
		_pubkey=$(_find_dnskey KEY3)
		test -z "$_pubkey" && return 1
		grep -F "$_pubkey" "dig.out.$DIR.test$n" > /dev/null || return 1
		_checksig=1
	elif [ "$(key_get KEY3 EXPECT)" = "yes" ]; then
		_pubkey=$(_find_dnskey KEY3)
		test -z "$_pubkey" && return 1
		grep -F "$_pubkey" "dig.out.$DIR.test$n" > /dev/null && return 1
	fi

	if [ "$(key_get KEY4 STATE_DNSKEY)" = "rumoured" ] || [ "$(key_get KEY4 STATE_DNSKEY)" = "omnipresent" ]; then
		_pubkey=$(_find_dnskey KEY4)
		test -z "$_pubkey" && return 1
		grep -F "$_pubkey" "dig.out.$DIR.test$n" > /dev/null || return 1
		_checksig=1
	elif [ "$(key_get KEY4 EXPECT)" = "yes" ]; then
		_pubkey=$(_find_dnskey KEY4)
		test -z "$_pubkey" && return 1
		grep -F "$_pubkey" "dig.out.$DIR.test$n" > /dev/null && return 1
	fi

	test "$_checksig" -eq 0 && return 0

	_check_signatures "DNSKEY" "dig.out.$DIR.test$n" "KSK" || return 1

	return 0
}

# Test the apex of a configured zone. This checks that the SOA and DNSKEY
# RRsets are signed correctly and with the appropriate keys.
check_apex() {

	# Test DNSKEY query.
	n=$((n+1))
	echo_i "check DNSKEY rrset is signed correctly for zone ${ZONE} ($n)"
	ret=0
	retry_quiet 10 _check_apex_dnskey || ret=1
	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))

	# We retry the DNSKEY query for at most ten seconds to avoid test
	# failures due to timing issues. If the DNSKEY query check passes this
	# means the zone is resigned and further apex checks (SOA, CDS, CDNSKEY)
	# don't need to be retried quietly.

	# Test SOA query.
	n=$((n+1))
	echo_i "check SOA rrset is signed correctly for zone ${ZONE} ($n)"
	ret=0
	_dig_with_opts "$ZONE" "@${SERVER}" "SOA" > "dig.out.$DIR.test$n" || _log_error "dig ${ZONE} SOA failed"
	grep "status: NOERROR" "dig.out.$DIR.test$n" > /dev/null || _log_error "mismatch status in DNS response"
	grep "${ZONE}\..*${DEFAULT_TTL}.*IN.*SOA.*" "dig.out.$DIR.test$n" > /dev/null || _log_error "missing SOA record in response"
	check_signatures "SOA" "dig.out.$DIR.test$n" "ZSK"
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
	_dig_with_opts "a.$ZONE" "@${SERVER}" $_qtype > "dig.out.$DIR.test$n" || _log_error "dig a.${ZONE} ${_qtype} failed"
	grep "status: NOERROR" "dig.out.$DIR.test$n" > /dev/null || _log_error "mismatch status in DNS response"
	grep "a.${ZONE}\..*${DEFAULT_TTL}.*IN.*${_qtype}.*10\.0\.0\.1" "dig.out.$DIR.test$n" > /dev/null || _log_error "missing a.${ZONE} ${_qtype} record in response"
	lines=$(get_keys_which_signed $_qtype "dig.out.$DIR.test$n" | wc -l)
	check_signatures $_qtype "dig.out.$DIR.test$n" "ZSK"
	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}

# Check if "CDS/CDNSKEY Published" is logged.
check_cdslog() {
	_dir=$1
	_zone=$2
	_key=$3

	_alg=$(key_get $_key ALG_STR)
	_id=$(key_get $_key ID)

	n=$((n+1))
	echo_i "check CDS/CDNSKEY publication is logged in ${_dir}/named.run for key ${_zone}/${_alg}/${_id} ($n)"
	ret=0

	grep "CDS for key ${_zone}/${_alg}/${_id} is now published" "${_dir}/named.run" > /dev/null || ret=1
	grep "CDNSKEY for key ${_zone}/${_alg}/${_id} is now published" "${_dir}/named.run" > /dev/null || ret=1

	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}

# Tell named that the DS for the key in given zone has been seen in the
# parent (this does not actually has to be true, we just issue the command
# to make named believe it can continue with the rollover).
rndc_checkds() {
	_server=$1
	_dir=$2
	_key=$3
	_when=$4
	_what=$5
	_zone=$6
	_view=$7

	_keycmd=""
	if [ "${_key}" != "-" ]; then
		_keyid=$(key_get $_key ID)
		_keycmd=" -key ${_keyid}"
	fi

	_whencmd=""
	if [ "${_when}" != "now" ]; then
		_whencmd=" -when ${_when}"
	fi

	n=$((n+1))
	echo_i "calling rndc dnssec -checkds${_keycmd}${_whencmd} ${_what} zone ${_zone} in ${_view} ($n)"
	ret=0

	_rndccmd $_server dnssec -checkds $_keycmd $_whencmd $_what $_zone in $_view > rndc.dnssec.checkds.out.$_zone.$n || _log_error "rndc dnssec -checkds${_keycmd}${_whencmd} ${_what} zone ${_zone} failed"

	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}

# Tell named to schedule a key rollover.
rndc_rollover() {
	_server=$1
	_dir=$2
	_keyid=$3
	_when=$4
	_zone=$5
	_view=$6

	_whencmd=""
	if [ "${_when}" != "now" ]; then
		_whencmd="-when ${_when}"
	fi

	n=$((n+1))
	echo_i "calling rndc dnssec -rollover key ${_keyid} ${_whencmd} zone ${_zone} ($n)"
	ret=0

	_rndccmd $_server dnssec -rollover -key $_keyid $_whencmd $_zone in $_view > rndc.dnssec.rollover.out.$_zone.$n || _log_error "rndc dnssec -rollover (key ${_keyid} when ${_when}) zone ${_zone} failed"

	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}
