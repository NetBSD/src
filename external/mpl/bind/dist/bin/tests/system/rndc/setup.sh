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

$SHELL ../genzone.sh 2 >ns2/nil.db
$SHELL ../genzone.sh 2 >ns2/other.db
$SHELL ../genzone.sh 2 >ns2/static.db

$SHELL ../genzone.sh 2 >ns6/huge.zone.db
awk 'END { for (i = 1; i <= 1000000; i++)
     printf "host%d IN A 10.53.0.6\n", i; }' < /dev/null >> ns6/huge.zone.db

copy_setports ns2/named.conf.in ns2/named.conf
copy_setports ns3/named.conf.in ns3/named.conf
copy_setports ns4/named.conf.in ns4/named.conf
copy_setports ns5/named.conf.in ns5/named.conf
copy_setports ns6/named.conf.in ns6/named.conf

make_key () {
    $RNDCCONFGEN -r $RANDFILE -k key$1 -A $3 -s 10.53.0.4 -p $2 \
            > ns4/key${1}.conf 2> /dev/null
    egrep -v '(^# Start|^# End|^# Use|^[^#])' ns4/key$1.conf | cut -c3- | \
            sed 's/allow { 10.53.0.4/allow { any/' >> ns4/named.conf
}

make_key 1 ${EXTRAPORT1} hmac-md5
make_key 2 ${EXTRAPORT2} hmac-sha1
make_key 3 ${EXTRAPORT3} hmac-sha224
make_key 4 ${EXTRAPORT4} hmac-sha256
make_key 5 ${EXTRAPORT5} hmac-sha384
make_key 6 ${EXTRAPORT6} hmac-sha512
