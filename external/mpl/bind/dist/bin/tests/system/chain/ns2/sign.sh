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

zone=example.
zonefile=example.db
signedfile=example.db.signed

ksk=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -b ${DEFAULT_BITS} -fk $zone)
zsk=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -b ${DEFAULT_BITS} $zone)
$SIGNER -S -o $zone -f $signedfile $zonefile > /dev/null

zone=wildcard-secure.example.
zonefile=wildcard-secure.db
signedfile=wildcard-secure.example.db.signed

ksk=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -b ${DEFAULT_BITS} -fk $zone)
zsk=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -b ${DEFAULT_BITS} $zone)
$SIGNER -S -o $zone -f $signedfile $zonefile > /dev/null

zone=wildcard-nsec.example.
zonefile=wildcard.db
signedfile=wildcard-nsec.example.db.signed

ksk=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -b ${DEFAULT_BITS} -fk $zone)
zsk=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -b ${DEFAULT_BITS} $zone)
$SIGNER -S -o $zone -f $signedfile $zonefile > /dev/null

zone=wildcard-nsec3.example.
zonefile=wildcard.db
signedfile=wildcard-nsec3.example.db.signed

ksk=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -b ${DEFAULT_BITS} -fk $zone)
zsk=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -b ${DEFAULT_BITS} $zone)
$SIGNER -S -3 - -H 0 -o $zone -f $signedfile $zonefile > /dev/null

zone=wildcard-nsec3-optout.example.
zonefile=wildcard.db
signedfile=wildcard-nsec3-optout.example.db.signed

ksk=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -b ${DEFAULT_BITS} -fk $zone)
zsk=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -b ${DEFAULT_BITS} $zone)
$SIGNER -S -3 - -H 0 -A -o $zone -f $signedfile $zonefile > /dev/null
