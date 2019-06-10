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

zone=signing.test
rm -rf keys/signing.test
mkdir -p keys/signing.test

timetodnssec() {
    $PERL -e 'my ($S,$M,$H,$d,$m,$y,$x) = gmtime(@ARGV[0]);
	      printf("%04u%02u%02u%02u%02u%02u\n", $y+1900,$m+1,$d,$H,$M,$S);' ${1}
}

KEYDIR=keys/signing.test
KSK=`$KEYGEN -a RSASHA256 -K $KEYDIR -q -f KSK $zone`

ZSK0=`$KEYGEN -a RSASHA256 -K $KEYDIR -q $zone`
ZSK1=`$KEYGEN -a RSASHA256 -K $KEYDIR -q $zone`
ZSK2=`$KEYGEN -a RSASHA256 -K $KEYDIR -q $zone`
ZSK3=`$KEYGEN -a RSASHA256 -K $KEYDIR -q $zone`
ZSK4=`$KEYGEN -a RSASHA256 -K $KEYDIR -q $zone`
ZSK5=`$KEYGEN -a RSASHA256 -K $KEYDIR -q $zone`
ZSK6=`$KEYGEN -a RSASHA256 -K $KEYDIR -q $zone`
ZSK7=`$KEYGEN -a RSASHA256 -K $KEYDIR -q $zone`
ZSK8=`$KEYGEN -a RSASHA256 -K $KEYDIR -q $zone`
ZSK9=`$KEYGEN -a RSASHA256 -K $KEYDIR -q $zone`

# clear all times on all keys
for FILEN in keys/signing.test/*.key
do
    $SETTIME -P none -A none -R none -I none -D none $FILEN
done

BASE=`date +%s`
BASET=`timetodnssec $BASE`

# reset the publish and activation time on the KSK
$SETTIME -P $BASET -A $BASET $KEYDIR/$KSK

# reset the publish and activation time on the first ZSK
$SETTIME -P $BASET -A $BASET $KEYDIR/$ZSK0

# schedule the first roll
R1=`expr $BASE + 300`
R1T=`timetodnssec $R1`

$SETTIME -I $R1T $KEYDIR/$ZSK0
$SETTIME -P $BASET -A $R1T $KEYDIR/$ZSK1

# schedule the second roll (which includes the delete of the first key)
R2=`expr $R1 + 300`
R2T=`timetodnssec $R2`
DT=$R2
DTT=`timetodnssec $DT`

$SETTIME -D $DTT $KEYDIR/$ZSK0
$SETTIME -I $R2T $KEYDIR/$ZSK1
$SETTIME -P $R1T -A $R2T $KEYDIR/$ZSK2

# schedule the third roll
# this isn't long enough for the signing to complete
R3=`expr $R2 + 60`
R3T=`timetodnssec $R3`

$SETTIME -D $R3T $KEYDIR/$ZSK1
$SETTIME -I $R3T $KEYDIR/$ZSK2
$SETTIME -P $R2T -A $R3T $KEYDIR/$ZSK3

$SETTIME -P $R3T $KEYDIR/$ZSK4

echo KSK=$KSK
echo ZSK0=$ZSK0
echo ZSK1=$ZSK1
echo ZSK2=$ZSK2
echo ZSK3=$ZSK3
echo ZSK4=$ZSK4

exit

# schedule the fourth roll
# this isn't long enough for the signing to complete
R4=`expr $R3 + 30`
R4T=`timetodnssec $R4`

$SETTIME -D $R4T $KEYDIR/$ZSK2
$SETTIME -I $R4T $KEYDIR/$ZSK3
$SETTIME -P $R3T -A $R4T $KEYDIR/$ZSK4
