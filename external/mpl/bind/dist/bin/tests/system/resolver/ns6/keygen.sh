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

zone=ds.example.net
zonefile="${zone}.db"
infile="${zonefile}.in"
cp $infile $zonefile
ksk=`$KEYGEN -q -a rsasha256 -r $RANDFILE -fk $zone`
zsk=`$KEYGEN -q -a rsasha256 -r $RANDFILE -b 2048 $zone`
cat $ksk.key $zsk.key >> $zonefile
$SIGNER -P -r $RANDFILE -o $zone $zonefile > /dev/null 2>&1

zone=example.net
zonefile="${zone}.db"
infile="${zonefile}.in"
cp $infile $zonefile
ksk=`$KEYGEN -q -a rsasha256 -r $RANDFILE -fk $zone`
zsk=`$KEYGEN -q -a rsasha256 -r $RANDFILE $zone`
cat $ksk.key $zsk.key dsset-ds.example.net$TP >> $zonefile
$SIGNER -P -r $RANDFILE -o $zone $zonefile > /dev/null 2>&1

# Configure a trusted key statement (used by delv)
keyfile_to_trusted_keys $ksk > ../ns5/trusted.conf
