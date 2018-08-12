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

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

$SHELL clean.sh

cp -f ns1/example1.db ns1/example.db
cp -f ns3/nomaster.db ns3/nomaster1.db

copy_setports ns1/named.conf.in ns1/named.conf
copy_setports ns2/named.conf.in ns2/named.conf
copy_setports ns3/named.conf.in ns3/named.conf

#
# SIG(0) required cryptographic support which may not be configured.
#
test -r $RANDFILE || $GENRANDOM 800 $RANDFILE
keyname=`$KEYGEN  -q -r $RANDFILE -n HOST -a RSASHA1 -b 1024 -T KEY sig0.example2 2>/dev/null | $D2U`
if test -n "$keyname"
then
	cat ns1/example1.db $keyname.key > ns1/example2.db
	echo $keyname > keyname
else
	cat ns1/example1.db > ns1/example2.db
	rm -f keyname
fi
