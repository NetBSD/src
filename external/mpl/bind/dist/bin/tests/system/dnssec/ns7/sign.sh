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

zone=split-rrsig
infile=split-rrsig.db.in
zonefile=split-rrsig.db

k1=`$KEYGEN -q -r $RANDFILE -a RSASHA256 -b 1024 -n zone $zone`
k2=`$KEYGEN -q -r $RANDFILE -a RSASHA256 -b 1024 -n zone $zone`

cat $infile $k1.key $k2.key >$zonefile

$SIGNER -P -3 - -A -r $RANDFILE -o $zone -O full -f $zonefile.unsplit -e now-3600 -s now-7200 $zonefile > /dev/null 2>&1
awk 'BEGIN { r = ""; }
     $4 == "RRSIG" && $5 == "SOA" && r == "" { r = $0; next; }
     { print }
     END { print r }' $zonefile.unsplit > $zonefile.signed
