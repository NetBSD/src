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

# leave as expr as expr treats arguments with leading 0's as base 10
# handle exit code 1 from expr when the result is 0
oldid=${1:-00000}
newid=$(expr \( ${oldid} + 1000 \) % 65536 || true)
newid=$(expr "0000${newid}" : '.*\(.....\)$')	# prepend leading 0's
badid=$(expr \( ${oldid} + 7777 \) % 65536 || true)
badid=$(expr "0000${badid}" : '.*\(.....\)$')	# prepend leading 0's

SYSTEMTESTTOP=../..
. $SYSTEMTESTTOP/conf.sh

zone=example.
infile=example.db.in
zonefile=example.db

keyname1=$($KEYGEN -q -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone)
keyname2=$($KEYGEN -q -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n zone $zone)

cat $infile $keyname1.key $keyname2.key >$zonefile
echo root-key-sentinel-is-ta-$oldid A 10.53.0.1 >> $zonefile
echo root-key-sentinel-not-ta-$oldid A 10.53.0.2 >> $zonefile
echo root-key-sentinel-is-ta-$newid A 10.53.0.3 >> $zonefile
echo root-key-sentinel-not-ta-$newid A 10.53.0.4 >> $zonefile
echo old-is-ta CNAME root-key-sentinel-is-ta-$oldid >> $zonefile
echo old-not-ta CNAME root-key-sentinel-not-ta-$oldid >> $zonefile
echo new-is-ta CNAME root-key-sentinel-is-ta-$newid >> $zonefile
echo new-not-ta CNAME root-key-sentinel-not-ta-$newid >> $zonefile
echo bad-is-ta CNAME root-key-sentinel-is-ta-$badid >> $zonefile
echo bad-not-ta CNAME root-key-sentinel-not-ta-$badid >> $zonefile

$SIGNER -P -g -o $zone -k $keyname1 $zonefile $keyname2 > /dev/null
