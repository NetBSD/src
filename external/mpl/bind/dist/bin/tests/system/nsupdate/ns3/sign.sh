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

zone=nsec3param.test.
infile=nsec3param.test.db.in
zonefile=nsec3param.test.db

keyname1=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
keyname2=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -P -3 - -H 1 -o $zone -k $keyname1 $zonefile $keyname2 > /dev/null

zone=dnskey.test.
infile=dnskey.test.db.in
zonefile=dnskey.test.db

keyname1=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone)
keyname2=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -P -o $zone -k $keyname1 $zonefile $keyname2 > /dev/null

zone=delegation.test.
infile=delegation.test.db.in
zonefile=delegation.test.db

keyname1=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -3 -f KSK $zone)
keyname2=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -3 $zone)

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -A -3 - -P -o $zone -k $keyname1 $zonefile $keyname2 > /dev/null

# Just copy multisigner.db.in because it is signed with dnssec-policy.
cp multisigner.test.db.in multisigner.test.db
