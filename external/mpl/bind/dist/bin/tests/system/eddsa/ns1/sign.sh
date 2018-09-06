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

key1=`$KEYGEN -q -r $RANDFILE -a ED25519 -n zone $zone`
key2=`$KEYGEN -q -r $RANDFILE -a ED25519 -n zone -f KSK $zone`
#key2=`$KEYGEN -q -r $RANDFILE -a ED448 -n zone -f KSK $zone`
$DSFROMKEY -a sha-256 $key2.key > dsset-256

cat $infile $key1.key $key2.key > $zonefile

$SIGNER -P -g -r $RANDFILE -o $zone $zonefile > /dev/null 2> signer.err || cat signer.err

# Configure the resolving server with a trusted key.
keyfile_to_trusted_keys $key1 > trusted.conf
cp trusted.conf ../ns2/trusted.conf

cd ../ns2 && $SHELL sign.sh
