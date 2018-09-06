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

cp ../ns2/dsset-example.in dsset-example$TP

keyname=`$KEYGEN -q -r $RANDFILE -a RSASHA1 -b 1024 -n zone $zone`

cat $infile $keyname.key > $zonefile

$SIGNER -P -g -r $RANDFILE -o $zone $zonefile > /dev/null

# Configure the resolving server with a trusted key.
keyfile_to_trusted_keys $keyname > trusted.conf
cp trusted.conf ../ns2/trusted.conf
cp trusted.conf ../ns3/trusted.conf

cd ../ns2 && $SHELL -e ./sign.sh
