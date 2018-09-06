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
infile=../ns1/root.db.in
zonefile=root.db.signed

keyname=`$KEYGEN -r $RANDFILE -a RSASHA1 -qfk $zone`

# copy the KSK out first, then revoke it
keyfile_to_managed_keys $keyname > revoked.conf

$SETTIME -R now ${keyname}.key > /dev/null

# create a current set of keys, and sign the root zone
$KEYGEN -r $RANDFILE -a RSASHA1 -q $zone > /dev/null
$KEYGEN -r $RANDFILE -a RSASHA1 -qfk $zone > /dev/null
$SIGNER -S -r $RANDFILE -o $zone -f $zonefile $infile > /dev/null 2>&1
