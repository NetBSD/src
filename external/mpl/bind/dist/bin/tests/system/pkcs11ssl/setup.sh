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

infile=ns1/example.db.in

/bin/echo -n ${HSMPIN:-1234}> pin
PWD=`pwd`

zone=rsa.example
zonefile=ns1/rsa.example.db

$PK11GEN -a RSA -b 1024 -l robie-rsa-zsk1 -i 01
$PK11GEN -a RSA -b 1024 -l robie-rsa-zsk2 -i 02
$PK11GEN -a RSA -b 2048 -l robie-rsa-ksk

rsazsk1=`$KEYFRLAB -a RSASHA1 \
        -l "robie-rsa-zsk1" rsa.example`
rsazsk2=`$KEYFRLAB -a RSASHA1 \
        -l "robie-rsa-zsk2" rsa.example`
rsaksk=`$KEYFRLAB -a RSASHA1 -f ksk \
        -l "robie-rsa-ksk" rsa.example`

cat $infile $rsazsk1.key $rsaksk.key > $zonefile
$SIGNER -a -P -g -r $RANDFILE -o $zone $zonefile \
        > /dev/null 2> signer.err || cat signer.err
cp $rsazsk2.key ns1/rsa.key
mv Krsa* ns1

rm -f signer.err
