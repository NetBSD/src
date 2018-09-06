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

SYSTESTDIR=wildcard

dssets=

zone=dlv.
infile=dlv.db.in
zonefile=dlv.db
outfile=dlv.db.signed
dssets="$dssets dsset-`echo $zone |sed -e "s/.$//g"`$TP"

keyname1=`$KEYGEN -r $RANDFILE -a RSASHA1 -b 1024 -n zone $zone 2> /dev/null` 
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a RSASHA1 -b 1024 -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key > $zonefile

$SIGNER -r $RANDFILE -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"

zone=nsec.
infile=nsec.db.in
zonefile=nsec.db
outfile=nsec.db.signed
dssets="$dssets dsset-`echo $zone |sed -e "s/.$//g"`$TP"

keyname1=`$KEYGEN -r $RANDFILE -a RSASHA1 -b 1024 -n zone $zone 2> /dev/null` 
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a RSASHA1 -b 1024 -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key > $zonefile

$SIGNER -r $RANDFILE -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"

zone=private.nsec.
infile=private.nsec.db.in
zonefile=private.nsec.db
outfile=private.nsec.db.signed

keyname1=`$KEYGEN -r $RANDFILE -a RSASHA1 -b 1024 -n zone $zone 2> /dev/null` 
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a RSASHA1 -b 1024 -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key > $zonefile

$SIGNER -r $RANDFILE -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"

keyfile_to_trusted_keys $keyname2 > private.nsec.conf

zone=nsec3.
infile=nsec3.db.in
zonefile=nsec3.db
outfile=nsec3.db.signed
dssets="$dssets dsset-`echo $zone |sed -e "s/.$//g"`$TP"

keyname1=`$KEYGEN -r $RANDFILE -a NSEC3RSASHA1 -b 1024 -n zone $zone 2> /dev/null` 
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a NSEC3RSASHA1 -b 1024 -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key > $zonefile

$SIGNER -r $RANDFILE -3 - -H 10 -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"

zone=private.nsec3.
infile=private.nsec3.db.in
zonefile=private.nsec3.db
outfile=private.nsec3.db.signed

keyname1=`$KEYGEN -r $RANDFILE -a NSEC3RSASHA1 -b 1024 -n zone $zone 2> /dev/null` 
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a NSEC3RSASHA1 -b 1024 -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key > $zonefile

$SIGNER -r $RANDFILE -3 - -H 10 -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"

keyfile_to_trusted_keys $keyname2 > private.nsec3.conf

zone=.
infile=root.db.in
zonefile=root.db
outfile=root.db.signed

keyname1=`$KEYGEN -r $RANDFILE -a RSASHA1 -b 1024 -n zone $zone 2> /dev/null`
keyname2=`$KEYGEN -f KSK -r $RANDFILE -a RSASHA1 -b 1024 -n zone $zone 2> /dev/null`

cat $infile $keyname1.key $keyname2.key $dssets >$zonefile

$SIGNER -r $RANDFILE -o $zone -f $outfile $zonefile > /dev/null 2> signer.err || cat signer.err
echo_i "signed $zone"

keyfile_to_trusted_keys $keyname2 > trusted.conf
