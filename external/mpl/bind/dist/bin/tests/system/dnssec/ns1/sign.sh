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

(cd ../ns2 && $SHELL sign.sh )
(cd ../ns6 && $SHELL sign.sh )
(cd ../ns7 && $SHELL sign.sh )

cp ../ns2/dsset-example$TP .
cp ../ns2/dsset-dlv$TP .
cp ../ns2/dsset-in-addr.arpa$TP .

grep "8 [12] " ../ns2/dsset-algroll$TP > dsset-algroll$TP
cp ../ns6/dsset-optout-tld$TP .

keyname=`$KEYGEN -q -r $RANDFILE -a RSAMD5 -b 1024 -n zone $zone`

cat $infile $keyname.key > $zonefile

$SIGNER -P -g -r $RANDFILE -o $zone $zonefile > /dev/null

# Configure the resolving server with a trusted key.
keyfile_to_trusted_keys $keyname > trusted.conf
cp trusted.conf ../ns2/trusted.conf
cp trusted.conf ../ns3/trusted.conf
cp trusted.conf ../ns4/trusted.conf
cp trusted.conf ../ns6/trusted.conf
cp trusted.conf ../ns7/trusted.conf

# ...or with a managed key.
keyfile_to_managed_keys $keyname > managed.conf
cp managed.conf ../ns4/managed.conf

#
#  Save keyid for managed key id test.
#
keyid=`expr $keyname : 'K.+001+\(.*\)'`
keyid=`expr $keyid + 0`
echo "$keyid" > managed.key.id
