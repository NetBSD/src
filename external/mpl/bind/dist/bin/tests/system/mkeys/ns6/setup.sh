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

# an RSA key
rsakey=`$KEYGEN -a rsasha256 -qfk rsasha256.`

# a key with unsupported algorithm
unsupportedkey=Kunknown.+255+00000
cp unsupported-managed.key "${unsupportedkey}.key"

# root key
rootkey=`cat ../ns1/managed.key`
cp "../ns1/${rootkey}.key" .

# Configure the resolving server with a managed trusted key.
keyfile_to_managed_keys $unsupportedkey $rsakey $rootkey > managed.conf
