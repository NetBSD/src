#!/bin/sh

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

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

cp -f ns1/example1.db ns1/example.db
cp -f ns3/nomaster.db ns3/nomaster1.db

copy_setports ns1/named.conf.in ns1/named.conf
copy_setports ns2/named.conf.in ns2/named.conf
copy_setports ns3/named1.conf.in ns3/named.conf

if $FEATURETEST --enable-dnstap
then
	cat <<'EOF' > ns3/dnstap.conf
	dnstap-identity "ns3";
	dnstap-version "xxx";
	dnstap-output file "dnstap.out";
	dnstap { all; };
EOF
else
	echo "/* DNSTAP NOT ENABLED */" >ns3/dnstap.conf
fi


#
# SIG(0) required cryptographic support which may not be configured.
#
keyname=$($KEYGEN  -q -n HOST -a ${DEFAULT_ALGORITHM} -b 1024 -T KEY sig0.example2 2>keyname.err)
if test -n "$keyname"
then
	cat ns1/example1.db $keyname.key > ns1/example2.db
	echo $keyname > keyname
else
	cat ns1/example1.db > ns1/example2.db
fi
cat_i < keyname.err
