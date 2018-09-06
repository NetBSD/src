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

zone=optout-tld
infile=optout-tld.db.in
zonefile=optout-tld.db

keyname=`$KEYGEN -q -r $RANDFILE -a RSASHA256 -b 1024 -n zone $zone`

cat $infile $keyname.key >$zonefile

$SIGNER -P -3 - -A -r $RANDFILE -o $zone $zonefile > /dev/null 2>&1
