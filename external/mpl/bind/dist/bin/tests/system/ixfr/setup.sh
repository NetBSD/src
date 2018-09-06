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

rm -f ns1/*.db ns1/*.jnl ns3/*.jnl ns4/*.db ns4/*.jnl

cat <<EOF >ns1/named.conf
options {
	query-source address 10.53.0.1;
	notify-source 10.53.0.1;
	transfer-source 10.53.0.1;
	port ${PORT};
	pid-file "named.pid";
	listen-on { 10.53.0.1; };
	listen-on-v6 { none; };
	recursion no;
	notify yes;
};

key rndc_key {
	secret "1234abcd8765";
	algorithm hmac-sha256;
};

controls {
	inet 10.53.0.1 port ${CONTROLPORT} allow { any; } keys { rndc_key; };
};
EOF

copy_setports ns3/named.conf.in ns3/named.conf
copy_setports ns4/named.conf.in ns4/named.conf

# Setup initial db files for ns3
cp ns3/mytest0.db ns3/mytest.db
cp ns3/subtest0.db ns3/subtest.db
$SHELL ../genzone.sh 3 > ns3/large.db
awk 'END { for (i = 0; i < 10000; i++) printf("record%d 10 IN TXT this is record %d\n", i, i) }' < /dev/null >> ns3/large.db
