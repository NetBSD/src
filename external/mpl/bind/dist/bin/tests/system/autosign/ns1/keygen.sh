#!/bin/sh -e

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

SYSTEMTESTTOP=../..
. $SYSTEMTESTTOP/conf.sh

zone=.
zonefile=root.db
infile=root.db.in

(cd ../ns2 && $SHELL keygen.sh )

cat $infile ../ns2/dsset-example$TP ../ns2/dsset-bar$TP > $zonefile

zskact=$($KEYGEN -3 -a ${DEFAULT_ALGORITHM} -q $zone)
zskvanish=$($KEYGEN -3 -a ${DEFAULT_ALGORITHM} -q $zone)
zskdel=$($KEYGEN -3 -a ${DEFAULT_ALGORITHM} -q -D now $zone)
zskinact=$($KEYGEN -3 -a ${DEFAULT_ALGORITHM} -q -I now $zone)
zskunpub=$($KEYGEN -3 -a ${DEFAULT_ALGORITHM} -q -G $zone)
zsksby=$($KEYGEN -3 -a ${DEFAULT_ALGORITHM} -q -A none $zone)
zskactnowpub1d=$($KEYGEN -3 -a ${DEFAULT_ALGORITHM} -q -A now -P +1d $zone)
zsknopriv=$($KEYGEN -3 -a ${DEFAULT_ALGORITHM} -q $zone)
rm $zsknopriv.private

ksksby=$($KEYGEN -3 -a ${DEFAULT_ALGORITHM} -q -P now -A now+15s -fk $zone)
kskrev=$($KEYGEN -3 -a ${DEFAULT_ALGORITHM} -q -R now+15s -fk $zone)

keyfile_to_static_ds $ksksby > trusted.conf
cp trusted.conf ../ns2/trusted.conf
cp trusted.conf ../ns3/trusted.conf
cp trusted.conf ../ns4/trusted.conf

keyfile_to_static_ds $kskrev > trusted.conf
cp trusted.conf ../ns5/trusted.conf

echo $zskact > ../active.key
echo $zskvanish > ../vanishing.key
echo $zskdel > ../del.key
echo $zskinact > ../inact.key
echo $zskunpub > ../unpub.key
echo $zsknopriv > ../nopriv.key
echo $zsksby > ../standby.key
echo $zskactnowpub1d > ../activate-now-publish-1day.key
$REVOKE -R $kskrev > ../rev.key
