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

# Have the child generate subdomain keys and pass DS sets to us.
( cd ../ns3 && $SHELL keygen.sh )

for subdomain in secure nsec3 autonsec3 optout rsasha256 rsasha512 \
		 nsec3-to-nsec oldsigs sync dname-at-apex-nsec3 cds-delete \
		 cdnskey-delete
do
	cp ../ns3/dsset-$subdomain.example$TP .
done

# Create keys and pass the DS to the parent.
zone=example
zonefile="${zone}.db"
infile="${zonefile}.in"
cat $infile dsset-*.example$TP > $zonefile

kskname=$($KEYGEN -a ${DEFAULT_ALGORITHM} -3 -q -fk $zone)
$KEYGEN -a ${DEFAULT_ALGORITHM} -3 -q $zone > /dev/null
$DSFROMKEY $kskname.key > dsset-${zone}$TP

# Create keys for a private secure zone.
zone=private.secure.example
zonefile="${zone}.db"
infile="${zonefile}.in"
ksk=$($KEYGEN -a ${DEFAULT_ALGORITHM} -3 -q -fk $zone)
$KEYGEN -a ${DEFAULT_ALGORITHM} -3 -q $zone > /dev/null
keyfile_to_static_ds $ksk > private.conf
cp private.conf ../ns4/private.conf
$SIGNER -S -3 beef -A -o $zone -f $zonefile $infile > /dev/null

# Extract saved keys for the revoke-to-duplicate-key test
zone=bar
zonefile="${zone}.db"
infile="${zonefile}.in"
cat $infile > $zonefile
for i in Xbar.+013+59973.key Xbar.+013+59973.private \
	 Xbar.+013+60101.key Xbar.+013+60101.private
do
    cp $i $(echo $i | sed s/X/K/)
done
$KEYGEN -a ECDSAP256SHA256 -q $zone > /dev/null
$DSFROMKEY Kbar.+013+60101.key > dsset-bar$TP
