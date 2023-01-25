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

#
# We use rsasha256 here to get a ZSK + KSK that don't fit in 512 bytes.
#
zone=ds.example.net
zonefile="${zone}.db"
infile="${zonefile}.in"
cp $infile $zonefile
ksk=$($KEYGEN -q -a rsasha256 -fk $zone)
zsk=$($KEYGEN -q -a rsasha256 -b 2048 $zone)
cat $ksk.key $zsk.key >> $zonefile
$SIGNER -P -o $zone $zonefile > /dev/null

zone=example.net
zonefile="${zone}.db"
infile="${zonefile}.in"
cp $infile $zonefile
ksk=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -fk $zone)
zsk=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} $zone)
cat $ksk.key $zsk.key dsset-ds.example.net$TP >> $zonefile
$SIGNER -P -o $zone $zonefile > /dev/null

# Configure a static key to be used by delv
keyfile_to_static_ds $ksk > ../ns5/trusted.conf
