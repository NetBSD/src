#!/bin/sh

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

. ../../conf.sh

SYSTESTDIR=legacy

echo_i "sign edns512-notcp"

zone=edns512-notcp
infile=edns512-notcp.db.in
zonefile=edns512-notcp.db
outfile=edns512-notcp.db.signed

keyname1=$($KEYGEN -a RSASHA512 -b 4096 -n zone $zone 2>/dev/null)
keyname2=$($KEYGEN -f KSK -a RSASHA512 -b 4096 -n zone $zone 2>/dev/null)

cat $infile $keyname1.key $keyname2.key >$zonefile

$SIGNER -g -o $zone -f $outfile -e +30y $zonefile >/dev/null 2>signer.err || cat signer.err

keyfile_to_static_ds $keyname2 >trusted.conf
cp trusted.conf ../ns1
