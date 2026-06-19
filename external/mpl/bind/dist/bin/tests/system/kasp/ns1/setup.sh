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

echo_i "ns1/setup.sh"

# Make lines shorter by storing key states in environment variables.
H="HIDDEN"
O="OMNIPRESENT"

zone="."
echo_i "setting up zone: $zone"
Tpub="now-30d"
Tact="now-1d"
keytimes="-P ${Tpub} -A ${Tact}"
CSK=$($KEYGEN -a $DEFAULT_ALGORITHM -f KSK -L 3600 $keytimes $zone 2>keygen.out.$zone.1)
$SETTIME -s -g $O -k $O $Tpub -r $O $Tpub -d $H $Tact -z $O $Tpub "$CSK" >settime.out.$zone.1 2>&1
echo "KSK: yes" >>"${CSK}".state
echo "ZSK: yes" >>"${CSK}".state
