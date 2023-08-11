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

( cd ../ns2 && $SHELL -e sign.sh )

cp ../ns2/dsset-* .

zone=.
infile=root.db.in
zonefile=root.db

keyname1=$($KEYGEN -a ${DEFAULT_ALGORITHM} -f KSK $zone 2> /dev/null)
keyname2=$($KEYGEN -a ${DEFAULT_ALGORITHM} $zone 2> /dev/null)

cat $infile $keyname1.key $keyname2.key > $zonefile

$SIGNER -P -g -o $zone $zonefile > /dev/null

# Add a trust anchor for a name whose non-existence can be securely proved
# without recursing when the root zone is mirrored.  This will exercise code
# attempting to send TAT queries for such names (in ns3).  Key data is
# irrelevant here, so just reuse the root zone key generated above.
sed "s/^\./nonexistent./;" $keyname1.key > $keyname1.modified.key

keyfile_to_static_ds $keyname1 $keyname1.modified > trusted.conf
