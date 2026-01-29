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

O="OMNIPRESENT"
ksktimes="-P now -A now -P sync now"
zsktimes="-P now -A now"

zone="model2.multisigner"
echo_i "setting up zone: $zone"
zonefile="${zone}.db"

KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -f KSK -L 3600 $ksktimes $zone 2>keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600 $zsktimes $zone 2>keygen.out.$zone.2)
$SETTIME -s -g $O -k $O now -r $O now -d $O now "$KSK" >settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O now -z $O now "$ZSK" >settime.out.$zone.2 2>&1

zone="model2.secondary"
echo_i "setting up zone: $zone"
zonefile="${zone}.db"
cp "../ns5/${zonefile}.in" "$zonefile"

KSK=$($KEYGEN -a $DEFAULT_ALGORITHM -f KSK -L 3600 $ksktimes $zone 2>keygen.out.$zone.1)
ZSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600 $zsktimes $zone 2>keygen.out.$zone.2)
$SETTIME -s -g $O -k $O now -r $O now -d $O now "$KSK" >settime.out.$zone.1 2>&1
$SETTIME -s -g $O -k $O now -z $O now "$ZSK" >settime.out.$zone.2 2>&1
# ZSK will be added to the other provider with nsupdate.
cat "${ZSK}.key" | grep -v ";.*" >"${zone}.zsk"
