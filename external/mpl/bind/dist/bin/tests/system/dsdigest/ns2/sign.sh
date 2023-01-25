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

zone1=good
infile1=good.db.in
zonefile1=good.db
zone2=bad
infile2=bad.db.in
zonefile2=bad.db

keyname11=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone1)
keyname12=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone1)
keyname21=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone2)
keyname22=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone -f KSK $zone2)

cat $infile1 $keyname11.key $keyname12.key >$zonefile1
cat $infile2 $keyname21.key $keyname22.key >$zonefile2

$SIGNER -P -g -o $zone1 $zonefile1 > /dev/null
$SIGNER -P -g -o $zone2 $zonefile2 > /dev/null

DSFILENAME1=dsset-${zone1}${TP}
DSFILENAME2=dsset-${zone2}${TP}
$DSFROMKEY -a SHA-256 $keyname12 > $DSFILENAME1
$DSFROMKEY -a SHA-256 $keyname22 > $DSFILENAME2

algo=SHA-384

$DSFROMKEY -a $algo $keyname12 >> $DSFILENAME1
$DSFROMKEY -a $algo $keyname22 > $DSFILENAME2

