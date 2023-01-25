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

# We sign the zone here and move the signed zone to ns6.
# The ns5 server actually does not serve this zone but
# the DS and NS records are in the test root zone, and
# delegate to ns6.
zone=signed.
infile=signed.db.in
zonefile=signed.db

key1=$($KEYGEN -q -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS $zone 2> /dev/null)
key2=$($KEYGEN -q -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -fk $zone 2> /dev/null)

cat $infile $key1.key $key2.key > $zonefile

$SIGNER -P -g -O full -o $zone $zonefile > sign.ns5.signed.out

cp signed.db.signed ../ns6

# Root zone.
zone=.
infile=root.db.in
zonefile=root.db

key1=$($KEYGEN -q -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS $zone 2> /dev/null)
key2=$($KEYGEN -q -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -fk $zone 2> /dev/null)

# cat $infile $key1.key $key2.key > $zonefile
cat $infile dsset-signed. $key1.key $key2.key > $zonefile

$SIGNER -P -g -O full -o $zone $zonefile > sign.ns5.root.out
