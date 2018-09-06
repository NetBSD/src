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

DIGOPTS="+tcp +noadd +nosea +nostat +noquest +noauth +nocomm +nocmd -p ${PORT}"

status=0

echo_i "test 2-element sortlist statement"
cat <<EOF >test1.good
a.example.		300	IN	A	192.168.3.1
a.example.		300	IN	A	192.168.1.1
a.example.		300	IN	A	1.1.1.5
a.example.		300	IN	A	1.1.1.1
a.example.		300	IN	A	1.1.1.3
a.example.		300	IN	A	1.1.1.2
a.example.		300	IN	A	1.1.1.4
EOF
$DIG $DIGOPTS a.example. @10.53.0.1 -b 10.53.0.1 >test1.dig
# Note that this can't use digcomp.pl because here, the ordering of the
# result RRs is significant.
$DIFF test1.dig test1.good || status=1

echo_i "test 1-element sortlist statement and undocumented BIND 8 features"
	cat <<EOF >test2.good
b.example.		300	IN	A	10.53.0.$n
EOF

$DIG $DIGOPTS b.example. @10.53.0.1 -b 10.53.0.2 | sed 1q | \
        egrep '10.53.0.(2|3)$' > test2.out &&
$DIG $DIGOPTS b.example. @10.53.0.1 -b 10.53.0.3 | sed 1q | \
        egrep '10.53.0.(2|3)$' >> test2.out &&
$DIG $DIGOPTS b.example. @10.53.0.1 -b 10.53.0.4 | sed 1q | \
        egrep '10.53.0.4$' >> test2.out &&
$DIG $DIGOPTS b.example. @10.53.0.1 -b 10.53.0.5 | sed 1q | \
        egrep '10.53.0.5$' >> test2.out || status=1

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
