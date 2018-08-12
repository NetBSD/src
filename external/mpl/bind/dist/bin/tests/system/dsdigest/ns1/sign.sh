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

zone=.
infile=root.db.in
zonefile=root.db

(cd ../ns2 && $SHELL sign.sh)

cp ../ns2/dsset-good$TP .
cp ../ns2/dsset-bad$TP .

key1=`$KEYGEN -q -r $RANDFILE -a RSASHA1 -b 1024 -n zone $zone`
key2=`$KEYGEN -q -r $RANDFILE -a RSASHA1 -b 2048 -n zone -f KSK $zone`

cat $infile $key1.key $key2.key > $zonefile

$SIGNER -P -g -r $RANDFILE -o $zone $zonefile > /dev/null

# Configure the resolving server with a trusted key.
keyfile_to_trusted_keys $key2 > trusted.conf
cp trusted.conf ../ns2/trusted.conf
cp trusted.conf ../ns3/trusted.conf
cp trusted.conf ../ns4/trusted.conf
