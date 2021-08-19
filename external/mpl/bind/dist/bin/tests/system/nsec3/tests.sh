#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# shellcheck source=conf.sh
. ../conf.sh
# shellcheck source=kasp.sh
. ../kasp.sh

# Log errors and increment $ret.
log_error() {
	echo_i "error: $1"
	ret=$((ret+1))
}

# Call dig with default options.
dig_with_opts() {
	$DIG +tcp +noadd +nosea +nostat +nocmd +dnssec -p "$PORT" "$@"
}

# Call rndc.
rndccmd() {
    "$RNDC" -c ../common/rndc.conf -p "$CONTROLPORT" -s "$@"
}

# Set zone name ($1) and policy ($2) for testing nsec3.
set_zone_policy() {
	ZONE=$1
	POLICY=$2
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

# The apex NSEC3PARAM record indicates that it is signed.
_wait_for_nsec3param() {
	dig_with_opts +noquestion "@${SERVER}" "$ZONE" NSEC3PARAM > "dig.out.test$n.wait" || return 1
	grep "${ZONE}\..*IN.*NSEC3PARAM.*1.*0.*${ITERATIONS}.*${SALT}" "dig.out.test$n.wait" > /dev/null || return 1
	grep "${ZONE}\..*IN.*RRSIG" "dig.out.test$n.wait" > /dev/null || return 1
        return 0
}
# The apex NSEC record indicates that it is signed.
_wait_for_nsec() {
	dig_with_opts +noquestion "@${SERVER}" "$ZONE" NSEC > "dig.out.test$n.wait" || return 1
	grep "NS SOA" "dig.out.test$n.wait" > /dev/null || return 1
	grep "${ZONE}\..*IN.*RRSIG" "dig.out.test$n.wait" > /dev/null || return 1
	grep "${ZONE}\..*IN.*NSEC3PARAM" "dig.out.test$n.wait" > /dev/null && return 1
        return 0
}

# Wait for the zone to be signed.
wait_for_zone_is_signed() {
	n=$((n+1))
	ret=0
	echo_i "wait for ${ZONE} to be signed ($n)"

	if [ "$1" = "nsec3" ]; then
		retry_quiet 10 _wait_for_nsec3param || log_error "wait for ${ZONE} to be signed failed"
	else
		retry_quiet 10 _wait_for_nsec || log_error "wait for ${ZONE} to be signed failed"
	fi

	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}

# Test: check NSEC in answers
_check_nsec_nsec3param()
{
	dig_with_opts +noquestion @$SERVER "${ZONE}" NSEC3PARAM > "dig.out.test$n.nsec3param.$ZONE" || return 1
	grep "NSEC3PARAM" "dig.out.test$n.nsec3param.$ZONE" > /dev/null && return 1
	return 0
}

_check_nsec_nxdomain()
{
	dig_with_opts @$SERVER "nosuchname.${ZONE}" > "dig.out.test$n.nxdomain.$ZONE" || return 1
	grep "${ZONE}.*IN.*NSEC.*NS.*SOA.*RRSIG.*NSEC.*DNSKEY" "dig.out.test$n.nxdomain.$ZONE" > /dev/null || return 1
	grep "NSEC3" "dig.out.test$n.nxdomain.$ZONE" > /dev/null && return 1
	return 0
}

check_nsec()
{
	n=$((n+1))
	echo_i "check NSEC3PARAM response for zone ${ZONE} ($n)"
	ret=0
	retry_quiet 10 _check_nsec_nsec3param || log_error "unexpected NSEC3PARAM in response for zone ${ZONE}"
	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))

	n=$((n+1))
	echo_i "check NXDOMAIN response for zone ${ZONE} ($n)"
	ret=0
	retry_quiet 10 _check_nsec_nxdomain || log_error "bad NXDOMAIN response for zone ${ZONE}"
	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}

# Test: check NSEC3 parameters in answers
_check_nsec3_nsec3param()
{
	dig_with_opts +noquestion @$SERVER "${ZONE}" NSEC3PARAM > "dig.out.test$n.nsec3param.$ZONE" || return 1
	grep "${ZONE}.*0.*IN.*NSEC3PARAM.*1.*0.*${ITERATIONS}.*${SALT}" "dig.out.test$n.nsec3param.$ZONE" > /dev/null || return 1

	if [ -z "$SALT" ]; then
		SALT=`awk '$4 == "NSEC3PARAM" { print $8 }' dig.out.test$n.nsec3param.$ZONE`
	fi
	return 0
}

_check_nsec3_nxdomain()
{
	dig_with_opts @$SERVER "nosuchname.${ZONE}" > "dig.out.test$n.nxdomain.$ZONE" || return 1
	grep ".*\.${ZONE}.*IN.*NSEC3.*1.${FLAGS}.*${ITERATIONS}.*${SALT}" "dig.out.test$n.nxdomain.$ZONE" > /dev/null || return 1
	return 0
}

check_nsec3()
{
	n=$((n+1))
	echo_i "check that NSEC3PARAM 1 0 ${ITERATIONS} is published zone ${ZONE} ($n)"
	ret=0
	retry_quiet 10 _check_nsec3_nsec3param || log_error "bad NSEC3PARAM response for ${ZONE}"
	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))

	n=$((n+1))
	echo_i "check NXDOMAIN response has correct NSEC3 1 ${FLAGS} ${ITERATIONS} ${SALT} for zone ${ZONE} ($n)"
	ret=0
	retry_quiet 10 _check_nsec3_nxdomain || log_error "bad NXDOMAIN response for zone ${ZONE}"
	test "$ret" -eq 0 || echo_i "failed"
	status=$((status+ret))
}

start_time="$(TZ=UTC date +%s)"
status=0
n=0

# Zone: nsec-to-nsec3.kasp.
set_zone_policy "nsec-to-nsec3.kasp" "nsec"
set_server "ns3" "10.53.0.3"
echo_i "initial check zone ${ZONE}"
check_nsec
dnssec_verify

# Zone: nsec3.kasp.
set_zone_policy "nsec3.kasp" "nsec3"
set_nsec3param "0" "5" "8"
echo_i "initial check zone ${ZONE}"
check_nsec3
dnssec_verify

# Zone: nsec3-dynamic.kasp.
set_zone_policy "nsec3-dynamic.kasp" "nsec3"
set_nsec3param "0" "5" "8"
echo_i "initial check zone ${ZONE}"
check_nsec3
dnssec_verify

# Zone: nsec3-change.kasp.
set_zone_policy "nsec3-change.kasp" "nsec3"
set_nsec3param "0" "5" "8"
echo_i "initial check zone ${ZONE}"
check_nsec3
dnssec_verify

# Zone: nsec3-dynamic-change.kasp.
set_zone_policy "nsec3-dynamic-change.kasp" "nsec3"
set_nsec3param "0" "5" "8"
echo_i "initial check zone ${ZONE}"
check_nsec3
dnssec_verify

# Zone: nsec3-to-nsec.kasp.
set_zone_policy "nsec3-to-nsec.kasp" "nsec3"
set_nsec3param "0" "5" "8"
echo_i "initial check zone ${ZONE}"
check_nsec3
dnssec_verify

# Zone: nsec3-to-optout.kasp.
set_zone_policy "nsec3-to-optout.kasp" "nsec3"
set_nsec3param "0" "5" "8"
echo_i "initial check zone ${ZONE}"
check_nsec3
dnssec_verify

# Zone: nsec3-from-optout.kasp.
set_zone_policy "nsec3-from-optout.kasp" "optout"
set_nsec3param "1" "5" "8"
echo_i "initial check zone ${ZONE}"
check_nsec3
dnssec_verify

# Zone: nsec3-other.kasp.
set_zone_policy "nsec3-other.kasp" "nsec3-other"
set_nsec3param "1" "11" "0"
echo_i "initial check zone ${ZONE}"
check_nsec3
dnssec_verify

# Reconfig named.
echo_i "reconfig dnssec-policy to trigger nsec3 rollovers"
copy_setports ns3/named2.conf.in ns3/named.conf
rndc_reconfig ns3 10.53.0.3

# Zone: nsec-to-nsec3.kasp. (reconfigured)
set_zone_policy "nsec-to-nsec3.kasp" "nsec3"
set_nsec3param "0" "5" "8"
echo_i "check zone ${ZONE} after reconfig"
check_nsec3
dnssec_verify

# Zone: nsec3.kasp. (same)
set_zone_policy "nsec3.kasp" "nsec3"
set_nsec3param "0" "5" "8"
echo_i "check zone ${ZONE} after reconfig"
check_nsec3
dnssec_verify

# Zone: nsec3-dyamic.kasp. (same)
set_zone_policy "nsec3-dynamic.kasp" "nsec3"
set_nsec3param "0" "5" "8"
echo_i "check zone ${ZONE} after reconfig"
check_nsec3
dnssec_verify

# Zone: nsec3-change.kasp. (reconfigured)
set_zone_policy "nsec3-change.kasp" "nsec3-other"
set_nsec3param "1" "11" "0"
echo_i "check zone ${ZONE} after reconfig"
check_nsec3
dnssec_verify

# Zone: nsec3-dynamic-change.kasp. (reconfigured)
set_zone_policy "nsec3-dynamic-change.kasp" "nsec3-other"
set_nsec3param "1" "11" "0"
echo_i "check zone ${ZONE} after reconfig"
check_nsec3
dnssec_verify

# Zone: nsec3-to-nsec.kasp. (reconfigured)
set_zone_policy "nsec3-to-nsec.kasp" "nsec"
set_nsec3param "1" "11" "0"
echo_i "check zone ${ZONE} after reconfig"
check_nsec
dnssec_verify

# Zone: nsec3-to-optout.kasp. (reconfigured)
# DISABLED:
# There is a bug in the nsec3param building code that thinks when the
# optout bit is changed, the chain already exists. [GL #2216]
#set_zone_policy "nsec3-to-optout.kasp" "optout"
#set_nsec3param "1" "5" "8"
#echo_i "check zone ${ZONE} after reconfig"
#check_nsec3
#dnssec_verify

# Zone: nsec3-from-optout.kasp. (reconfigured)
# DISABLED:
# There is a bug in the nsec3param building code that thinks when the
# optout bit is changed, the chain already exists. [GL #2216]
#set_zone_policy "nsec3-from-optout.kasp" "nsec3"
#set_nsec3param "0" "5" "8"
#echo_i "check zone ${ZONE} after reconfig"
#check_nsec3
#dnssec_verify

# Zone: nsec3-other.kasp. (same)
set_zone_policy "nsec3-other.kasp" "nsec3-other"
set_nsec3param "1" "11" "0"
echo_i "check zone ${ZONE} after reconfig"
check_nsec3
dnssec_verify

# Using rndc signing -nsec3param (should fail)
set_zone_policy "nsec3-change.kasp" "nsec3-other"
echo_i "use rndc signing -nsec3param ${ZONE} to change NSEC3 settings"
rndccmd $SERVER signing -nsec3param 1 1 12 ffff $ZONE > rndc.signing.test$n.$ZONE || log_error "failed to call rndc signing -nsec3param $ZONE"
grep "zone uses dnssec-policy, use rndc dnssec command instead" rndc.signing.test$n.$ZONE > /dev/null || log_error "rndc signing -nsec3param should fail"
check_nsec3
dnssec_verify

# Test NSEC3 and NSEC3PARAM is the same after restart
set_zone_policy "nsec3.kasp" "nsec3"
set_nsec3param "0" "5" "8"
echo_i "check zone ${ZONE} before restart"
check_nsec3
dnssec_verify

# Restart named, NSEC3 should stay the same.
ret=0
echo "stop ns3"
$PERL ../stop.pl --use-rndc --port ${CONTROLPORT} nsec3 ${DIR} || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

ret=0
echo "start ns3"
start_server --noclean --restart --port ${PORT} nsec3 ${DIR}
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

prevsalt="${SALT}"
set_zone_policy "nsec3.kasp" "nsec3"
set_nsec3param "0" "5" "8"
SALT="${prevsalt}"
echo_i "check zone ${ZONE} after restart has salt ${SALT}"
check_nsec3
dnssec_verify

# Zone: nsec3-fails-to-load.kasp. (should be fixed after reload)
cp ns3/template.db.in ns3/nsec3-fails-to-load.kasp.db
rndc_reload ns3 10.53.0.3

set_zone_policy "nsec3-fails-to-load.kasp" "nsec3"
set_nsec3param "0" "5" "8"
echo_i "check zone ${ZONE} after reload"
check_nsec3
dnssec_verify

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
