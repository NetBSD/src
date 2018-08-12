#!/bin/sh -e
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

SYSTEMTESTTOP=../..
. $SYSTEMTESTTOP/conf.sh

zone=example.
infile=example.db.in
zonefile=example.db

keyname1=`$KEYGEN -q -r $RANDFILE -a DSA -b 768 -n zone $zone`
keyname2=`$KEYGEN -q -r $RANDFILE -a DSA -b 768 -n zone $zone`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -P -g -r $RANDFILE -o $zone -k $keyname1 $zonefile $keyname2 > /dev/null
