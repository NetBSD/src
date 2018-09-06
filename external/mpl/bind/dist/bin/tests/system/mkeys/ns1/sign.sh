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
zonefile=root.db

keyname=`$KEYGEN -a rsasha256 -qfk -r $RANDFILE $zone`
zskkeyname=`$KEYGEN -a rsasha256 -q -r $RANDFILE $zone`

$SIGNER -Sg -r $RANDFILE -o $zone $zonefile > /dev/null 2>/dev/null

# Configure the resolving server with a managed trusted key.
keyfile_to_managed_keys $keyname > managed.conf
cp managed.conf ../ns2/managed.conf
cp managed.conf ../ns4/managed.conf
cp managed.conf ../ns5/managed.conf

# Configure a trusted key statement (used by delv)
keyfile_to_trusted_keys $keyname > trusted.conf

#
#  Save keyname and keyid for managed key id test.
#
echo "$keyname" > managed.key
keyid=`expr $keyname : 'K\.+00.+\([0-9]*\)'`
keyid=`expr $keyid + 0`
echo "$keyid" > managed.key.id
