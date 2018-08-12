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

test -r $RANDFILE || $GENRANDOM 800 $RANDFILE

copy_setports ns1/named1.conf.in ns1/named.conf
copy_setports ns2/named1.conf.in ns2/named.conf
copy_setports ns3/named1.conf.in ns3/named.conf
copy_setports ns4/named1.conf.in ns4/named.conf

if $SHELL ../testcrypto.sh -q
then
	(cd ns1 && $SHELL -e sign.sh)
	(cd ns4 && $SHELL -e sign.sh)
else
	echo_i "using pre-signed zones"
	cp -f ns1/signed.db.presigned ns1/signed.db.signed
	cp -f ns4/signed.db.presigned ns4/signed.db.signed
fi
