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

zone=dnssec.
infile=dnssec.db.in
zonefile=dnssec.db.signed
ksk=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -f KSK "$zone")
zsk=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")
# Sign deliberately with a very short expiration date.
"$SIGNER" -P -S -x -O full -e "now"+1s -o "$zone" -f "$zonefile" "$infile" >"signzone.out.$zone" 2>&1
keyfile_to_key_id "$ksk" >dnssec.ksk.id
keyfile_to_key_id "$zsk" >dnssec.zsk.id

zone=manykeys.
infile=manykeys.db.in
zonefile=manykeys.db.signed
ksk8=$("$KEYGEN" -q -a RSASHA256 -b 2048 -f KSK "$zone")
zsk8=$("$KEYGEN" -q -a RSASHA256 -b 2048 "$zone")
ksk13=$("$KEYGEN" -q -a ECDSAP256SHA256 -b 256 -f KSK "$zone")
zsk13=$("$KEYGEN" -q -a ECDSAP256SHA256 -b 256 "$zone")
ksk14=$("$KEYGEN" -q -a ECDSAP384SHA384 -b 384 -f KSK "$zone")
zsk14=$("$KEYGEN" -q -a ECDSAP384SHA384 -b 384 "$zone")
# Sign deliberately with a very short expiration date.
"$SIGNER" -S -x -O full -e "now"+1s -o "$zone" -f "$zonefile" "$infile" >"signzone.out.$zone" 2>&1
keyfile_to_key_id "$ksk8" >manykeys.ksk8.id
keyfile_to_key_id "$zsk8" >manykeys.zsk8.id
keyfile_to_key_id "$ksk13" >manykeys.ksk13.id
keyfile_to_key_id "$zsk13" >manykeys.zsk13.id
keyfile_to_key_id "$ksk14" >manykeys.ksk14.id
keyfile_to_key_id "$zsk14" >manykeys.zsk14.id
