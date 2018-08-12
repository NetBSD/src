#!/bin/sh 
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

zone=.
infile=root.db.in
zonefile=root.db

(cd ../ns2 && $SHELL -e sign.sh )

cp ../ns2/dsset-example$TP .
cp ../ns2/dsset-example.com$TP .

keyname1=`$KEYGEN -q -r $RANDFILE -a RSASHA256 -b 1024 -n zone $zone`
keyname2=`$KEYGEN -q -r $RANDFILE -a RSASHA256 -b 2048 -f KSK -n zone $zone`
cat $infile $keyname1.key $keyname2.key > $zonefile

$SIGNER -g -r $RANDFILE -o $zone $zonefile > /dev/null 2>&1

# Configure the resolving server with a trusted key.
keyfile_to_trusted_keys $keyname2 > trusted.conf
cp trusted.conf ../ns2/trusted.conf
cp trusted.conf ../ns3/trusted.conf
cp trusted.conf ../ns4/trusted.conf
