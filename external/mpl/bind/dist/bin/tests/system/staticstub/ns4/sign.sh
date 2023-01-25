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

zone=sub.example
infile=${zone}.db.in
zonefile=${zone}.db

keyname1=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
keyname2=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -f KSK -n zone $zone)

cat $infile $keyname1.key $keyname2.key > $zonefile

$SIGNER -o $zone $zonefile > /dev/null
