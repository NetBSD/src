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
have_rsa=`grep rsa supported`
if [ "x$have_rsa" != "x" ]; then
    $PK11GEN -a RSA -b 1024 -l robie-rsa-zsk1 -i 01
    $PK11GEN -a RSA -b 1024 -l robie-rsa-zsk2 -i 02
    $PK11GEN -a RSA -b 2048 -l robie-rsa-ksk

    rsazsk1=`$KEYFRLAB -a RSASHA1 \
            -l "object=robie-rsa-zsk1;pin-source=$PWD/pin" rsa.example`
    rsazsk2=`$KEYFRLAB -a RSASHA1 \
            -l "object=robie-rsa-zsk2;pin-source=$PWD/pin" rsa.example`
    rsaksk=`$KEYFRLAB -a RSASHA1 -f ksk \
            -l "object=robie-rsa-ksk;pin-source=$PWD/pin" rsa.example`

    cat $infile $rsazsk1.key $rsaksk.key > $zonefile
    $SIGNER -a -P -g -r $RANDFILE -o $zone $zonefile \
            > /dev/null 2> signer.err || cat signer.err
    cp $rsazsk2.key ns1/rsa.key
    mv Krsa* ns1
else
    # RSA not available and will not be tested; make a placeholder
    cp $infile ${zonefile}.signed
fi

zone=ecc.example
zonefile=ns1/ecc.example.db
have_ecc=`grep ecc supported`
if [ "x$have_ecc" != "x" ]; then
    $PK11GEN -a ECC -b 256 -l robie-ecc-zsk1 -i 03
    $PK11GEN -a ECC -b 256 -l robie-ecc-zsk2 -i 04
    $PK11GEN -a ECC -b 384 -l robie-ecc-ksk

    ecczsk1=`$KEYFRLAB -a ECDSAP256SHA256 \
            -l "object=robie-ecc-zsk1;pin-source=$PWD/pin" ecc.example`
    ecczsk2=`$KEYFRLAB -a ECDSAP256SHA256 \
            -l "object=robie-ecc-zsk2;pin-source=$PWD/pin" ecc.example`
    eccksk=`$KEYFRLAB -a ECDSAP384SHA384 -f ksk \
            -l "object=robie-ecc-ksk;pin-source=$PWD/pin" ecc.example`

    cat $infile $ecczsk1.key $eccksk.key > $zonefile
    $SIGNER -a -P -g -r $RANDFILE -o $zone $zonefile \
        > /dev/null 2> signer.err || cat signer.err
    cp $ecczsk2.key ns1/ecc.key
    mv Kecc* ns1
else
    # ECC not available and will not be tested; make a placeholder
    cp $infile ${zonefile}.signed
fi

zone=ecx.example
zonefile=ns1/ecx.example.db
have_ecx=`grep ecx supported`
if [ "x$have_ecx" != "x" ]; then
    $PK11GEN -a ECX -b 256 -l robie-ecx-zsk1 -i 05
    $PK11GEN -a ECX -b 256 -l robie-ecx-zsk2 -i 06
    $PK11GEN -a ECX -b 256 -l robie-ecx-ksk
#   $PK11GEN -a ECX -b 456 -l robie-ecx-ksk

    ecxzsk1=`$KEYFRLAB -a ED25519 \
            -l "object=robie-ecx-zsk1;pin-source=$PWD/pin" ecx.example`
    ecxzsk2=`$KEYFRLAB -a ED25519 \
            -l "object=robie-ecx-zsk2;pin-source=$PWD/pin" ecx.example`
    ecxksk=`$KEYFRLAB -a ED25519 -f ksk \
            -l "object=robie-ecx-ksk;pin-source=$PWD/pin" ecx.example`
#   ecxksk=`$KEYFRLAB -a ED448 -f ksk \
#           -l "object=robie-ecx-ksk;pin-source=$PWD/pin" ecx.example`

    cat $infile $ecxzsk1.key $ecxksk.key > $zonefile
    $SIGNER -a -P -g -r $RANDFILE -o $zone $zonefile \
        > /dev/null 2> signer.err || cat signer.err
    cp $ecxzsk2.key ns1/ecx.key
    mv Kecx* ns1
else
    # ECX not available and will not be tested; make a placeholder
    cp $infile ${zonefile}.signed
fi

rm -f signer.err
