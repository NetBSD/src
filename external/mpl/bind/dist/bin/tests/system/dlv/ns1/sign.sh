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

SYSTESTDIR=dlv

(cd ../ns2 && $SHELL -e ./sign.sh || exit 1)

echo_i "dlv/ns1/sign.sh"

zone=.
infile=root.db.in
zonefile=root.db
outfile=root.signed

keyname1=`$KEYGEN -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null` 
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a DSA -b 768 -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -r $RANDFILE -g -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err

echo_i "signed $zone"

keyfile_to_trusted_keys $keyname2 > trusted.conf
cp trusted.conf ../ns5
