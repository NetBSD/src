#!/bin/sh -e
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

zone1=good.
infile1=good.db.in
zonefile1=good.db
zone2=bad.
infile2=bad.db.in
zonefile2=bad.db

keyname11=`$KEYGEN -q -r $RANDFILE -a RSASHA256 -b 1024 -n zone $zone1`
keyname12=`$KEYGEN -q -r $RANDFILE -a RSASHA256 -b 2048 -n zone -f KSK $zone1`
keyname21=`$KEYGEN -q -r $RANDFILE -a RSASHA256 -b 1024 -n zone $zone2`
keyname22=`$KEYGEN -q -r $RANDFILE -a RSASHA256 -b 2048 -n zone -f KSK $zone2`

cat $infile1 $keyname11.key $keyname12.key >$zonefile1
cat $infile2 $keyname21.key $keyname22.key >$zonefile2

$SIGNER -P -g -r $RANDFILE -o $zone1 $zonefile1 > /dev/null
$SIGNER -P -g -r $RANDFILE -o $zone2 $zonefile2 > /dev/null

DSFILENAME1=dsset-`echo $zone1 |sed -e "s/\.$//g"`$TP
DSFILENAME2=dsset-`echo $zone2 |sed -e "s/\.$//g"`$TP
$DSFROMKEY -a SHA-256 $keyname12 > $DSFILENAME1
$DSFROMKEY -a SHA-256 $keyname22 > $DSFILENAME2

supported=`cat ../supported`
case "$supported" in
    gost) algo=GOST ;;
    *) algo=SHA-384 ;;
esac

$DSFROMKEY -a $algo $keyname12 >> $DSFILENAME1
$DSFROMKEY -a $algo $keyname22 > $DSFILENAME2

