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

for domain in example example.com; do
	zone=${domain}.
	infile=${domain}.db.in
	zonefile=${domain}.db

	keyname1=`$KEYGEN -q -r $RANDFILE -a NSEC3RSASHA1 -b 1024 -n zone $zone`
	keyname2=`$KEYGEN -q -r $RANDFILE -a NSEC3RSASHA1 -b 1024 -f KSK -n zone $zone`

	cat $infile $keyname1.key $keyname2.key > $zonefile

	$SIGNER -3 bebe -r $RANDFILE -o $zone $zonefile > /dev/null 2>&1
done

# remove "removed" record from example.com, causing the server to
# send an apparently-invalid NXDOMAIN
sed '/^removed/d' example.com.db.signed > example.com.db.new
rm -f example.com.db.signed
mv example.com.db.new example.com.db.signed
