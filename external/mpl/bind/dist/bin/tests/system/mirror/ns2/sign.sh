#!/bin/sh -e

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

SYSTEMTESTTOP=../..
. $SYSTEMTESTTOP/conf.sh

keys_to_trust=""

for zonename in sub.example example initially-unavailable; do
	zone=$zonename
	infile=$zonename.db.in
	zonefile=$zonename.db

	keyname1=$($KEYGEN -a ${DEFAULT_ALGORITHM} -f KSK $zone 2> /dev/null)
	keyname2=$($KEYGEN -a ${DEFAULT_ALGORITHM} $zone 2> /dev/null)

	cat $infile $keyname1.key $keyname2.key > $zonefile

	$SIGNER -P -g -o $zone $zonefile > /dev/null
done

# Only add the key for "initially-unavailable" to the list of keys trusted by
# ns3.  "example" is expected to be validated using a chain of trust starting in
# the "root" zone on ns1.
keys_to_trust="$keys_to_trust $keyname1"

# Prepare a zone signed using a Combined Signing Key (CSK) without the SEP bit
# set and add that key to the list of keys to trust.
zone=verify-csk
infile=verify.db.in
zonefile=verify-csk.db

keyname=$($KEYGEN -a ${DEFAULT_ALGORITHM} $zone 2> /dev/null)
cat $infile $keyname.key > $zonefile
$SIGNER -P -o $zone $zonefile > /dev/null
keys_to_trust="$keys_to_trust $keyname"

# Prepare remaining zones used in the test.
ORIGINAL_SERIAL=$(awk '$2 == "SOA" {print $5}' verify.db.in)
UPDATED_SERIAL_BAD=$((ORIGINAL_SERIAL + 1))
UPDATED_SERIAL_GOOD=$((ORIGINAL_SERIAL + 2))

for variant in addzone axfr ixfr load reconfig untrusted; do
	zone=verify-$variant
	infile=verify.db.in
	zonefile=verify-$variant.db

	keyname1=$($KEYGEN -a ${DEFAULT_ALGORITHM} -f KSK $zone 2> /dev/null)
	keyname2=$($KEYGEN -a ${DEFAULT_ALGORITHM} $zone 2> /dev/null)

	cat $infile $keyname1.key $keyname2.key > $zonefile

	# Prepare a properly signed version of the zone ("*.original.signed").
	$SIGNER -P -o $zone $zonefile > /dev/null
	cp $zonefile.signed $zonefile.original.signed
	# Prepare a version of the zone with a bogus SOA RRSIG ("*.bad.signed").
	sed "s/${ORIGINAL_SERIAL}/${UPDATED_SERIAL_BAD}/;" $zonefile.signed > $zonefile.bad.signed
	# Prepare another properly signed version of the zone ("*.good.signed").
	sed "s/${ORIGINAL_SERIAL}/${UPDATED_SERIAL_GOOD}/;" $zonefile > $zonefile.good
	$SIGNER -P -o $zone $zonefile.good > /dev/null
	rm -f $zonefile.good

	# Except for the "verify-untrusted" zone, declare the KSK used for
	# signing the zone to be a trust anchor for ns3.
	if [ "$variant" != "untrusted" ]; then
		keys_to_trust="$keys_to_trust $keyname1"
	fi
done

keyfile_to_static_ds $keys_to_trust > trusted-mirror.conf
