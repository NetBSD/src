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

# shellcheck source=conf.sh
. ../../conf.sh

echo_i "ns4/setup.sh"

#
# Set up zones that potentially will be initially signed.
#
for zn in inherit.inherit override.inherit none.inherit \
  inherit.override override.override none.override \
  inherit.none override.none none.none; do
  zone="$zn.signed"
  echo_i "setting up zone: $zone"
  zonefile="${zone}.db"
  cp template.db.in $zonefile
done

cp example1.db.in example1.db
cp example2.db.in example2.db

# Regression test for GL #5315
cp purgekeys1.conf purgekeys.conf
cp example1.db.in purgekeys.kasp.example1.db
cp example2.db.in purgekeys.kasp.example2.db

zone="purgekeys.kasp"
H="HIDDEN"
O="OMNIPRESENT"
T="now-9mo"
# KSK omnipresent
KSK=$($KEYGEN -fk -a 13 -L 3600 $zone 2>keygen.out.$zone.1)
$SETTIME -s -g $O -d $O $T -k $O $T -r $O $T "$KSK" >settime.out.$zone.1 2>&1
# ZSK omnipresent
ZSK1=$($KEYGEN -a 13 -L 3600 $zone 2>keygen.out.$zone.2)
$SETTIME -s -g $O -k $O $T -z $O $T "$ZSK1" >settime.out.$zone.2 2>&1
# ZSK hidden (may be purged)
ZSK2=$($KEYGEN -a 13 -L 3600 $zone 2>keygen.out.$zone.2)
$SETTIME -s -g $H -k $H $T -z $H $T "$ZSK2" >settime.out.$zone.2 2>&1
