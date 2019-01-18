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

SYSTEMTESTTOP=../..
. $SYSTEMTESTTOP/conf.sh

SYSTESTDIR=filter-aaaa

dlvsets=

zone=signed.
infile=signed.db.in
zonefile=signed.db.signed
outfile=signed.db.signed

$KEYGEN -a $DEFAULT_ALGORITHM $zone 2>&1 > /dev/null | cat_i
$KEYGEN -f KSK -a $DEFAULT_ALGORITHM $zone 2>&1 > keygen.out | cat_i
keyname=`cat keygen.out`
rm -f keygen.out

keyfile_to_trusted_keys $keyname > trusted.conf
cp trusted.conf ../ns2/trusted.conf
cp trusted.conf ../ns3/trusted.conf
cp trusted.conf ../ns5/trusted.conf

$SIGNER -S -o $zone -f $outfile $infile > /dev/null 2> signer.err || cat signer.err
echo_i "signed zone '$zone'"
