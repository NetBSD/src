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

set -e

longago="now-1y"
keytimes="-P ${longago} -A ${longago}"
O="omnipresent"

zone=dnssec.
infile=dnssec.db.in
zonefile=dnssec.db
cp $infile $zonefile
ksk=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -L 3600 -b "$DEFAULT_BITS" -f KSK "$zone")
zsk=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -L 3600 -b "$DEFAULT_BITS" "$zone")
# Sign deliberately with a very short expiration date.
"$SIGNER" -P -S -x -O full -e "now"+1s -o "$zone" "$zonefile" >"signzone.out.$zone" 2>&1
id=$(keyfile_to_key_id "$ksk")
echo "$DEFAULT_ALGORITHM_NUMBER+$id" >dnssec.ksk.id
id=$(keyfile_to_key_id "$zsk")
echo "$DEFAULT_ALGORITHM_NUMBER+$id" >dnssec.zsk.id

zone=manykeys.
infile=manykeys.db.in
zonefile=manykeys.db
cp $infile $zonefile
ksk8=$("$KEYGEN" -q -a RSASHA256 -L 3600 -b 2048 -f KSK $keytimes -P sync $longago "$zone")
zsk8=$("$KEYGEN" -q -a RSASHA256 -L 3600 -b 2048 $keytimes "$zone")
$SETTIME -s -g $O -k $O $longago -r $O $longago -z $O $longago -d $O $longago "$ksk8" >settime.out.$zone 2>&1
$SETTIME -s -g $O -k $O $longago -z $O $longago "$zsk8" >settime.out.$zone 2>&1
cat $ksk8.key $zsk8.key >>$zonefile
ksk13=$("$KEYGEN" -q -a ECDSAP256SHA256 -L 3600 -b 256 -f KSK "$zone")
zsk13=$("$KEYGEN" -q -a ECDSAP256SHA256 -L 3600 -b 256 "$zone")
cat $ksk13.key $zsk13.key >>$zonefile
ksk14=$("$KEYGEN" -q -a ECDSAP384SHA384 -L 3600 -b 384 -f KSK "$zone")
zsk14=$("$KEYGEN" -q -a ECDSAP384SHA384 -L 3600 -b 384 "$zone")
cat $ksk14.key $zsk14.key >>$zonefile
# Sign deliberately with a very short expiration date.
# Disable zone verification (-P) as records may expire before signing is complete
"$SIGNER" -P -S -x -O full -e "now"+1s -o "$zone" "$zonefile" >"signzone.out.$zone" 2>&1
id=$(keyfile_to_key_id "$ksk8")
echo "8+$id" >manykeys.ksk8.id
id=$(keyfile_to_key_id "$zsk8")
echo "8+$id" >manykeys.zsk8.id
id=$(keyfile_to_key_id "$ksk13")
echo "13+$id" >manykeys.ksk13.id
id=$(keyfile_to_key_id "$zsk13")
echo "13+$id" >manykeys.zsk13.id
id=$(keyfile_to_key_id "$ksk14")
echo "14+$id" >manykeys.ksk14.id
id=$(keyfile_to_key_id "$zsk14")
echo "14+$id" >manykeys.zsk14.id
