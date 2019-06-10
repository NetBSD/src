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

# Have the child generate subdomain keys and pass DS sets to us.
( cd ../ns3 && $SHELL keygen.sh )

for subdomain in secure nsec3 autonsec3 optout rsasha256 rsasha512 nsec3-to-nsec oldsigs sync \
    dname-at-apex-nsec3
do
	cp ../ns3/dsset-$subdomain.example$TP .
done

# Create keys and pass the DS to the parent.
zone=example
zonefile="${zone}.db"
infile="${zonefile}.in"
cat $infile dsset-*.example$TP > $zonefile

kskname=`$KEYGEN -a RSASHA1 -3 -q -fk $zone`
$KEYGEN -a RSASHA1 -3 -q $zone > /dev/null
$DSFROMKEY $kskname.key > dsset-${zone}$TP

# Create keys for a private secure zone.
zone=private.secure.example
zonefile="${zone}.db"
infile="${zonefile}.in"
ksk=`$KEYGEN -a RSASHA1 -3 -q -fk $zone`
$KEYGEN -a RSASHA1 -3 -q $zone > /dev/null
keyfile_to_trusted_keys $ksk > private.conf
cp private.conf ../ns4/private.conf
$SIGNER -S -3 beef -A -o $zone -f $zonefile $infile > /dev/null 2>&1

# Extract saved keys for the revoke-to-duplicate-key test
zone=bar
zonefile="${zone}.db"
infile="${zonefile}.in"
cat $infile > $zonefile
for i in Xbar.+005+30676.key Xbar.+005+30804.key Xbar.+005+30676.private \
	 Xbar.+005+30804.private
do
	cp $i `echo $i | sed s/X/K/`
done
$KEYGEN -a RSASHA1 -q $zone > /dev/null
$DSFROMKEY Kbar.+005+30804.key > dsset-bar$TP
