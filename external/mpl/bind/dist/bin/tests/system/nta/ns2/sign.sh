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

# Sign child zones (served by ns3).
(cd ../ns3 && $SHELL sign.sh)

# The "example." zone.
zone=example.
infile=example.db.in
zonefile=example.db

# Get the DS records for the "example." zone.
for subdomain in bogus badds secure; do
  cp "../ns3/dsset-$subdomain.example." .
done

# Sign the "example." zone.
keyname1=$("$KEYGEN" -q -a "$ALTERNATIVE_ALGORITHM" -b "$ALTERNATIVE_BITS" -f KSK "$zone")
keyname2=$("$KEYGEN" -q -a "$ALTERNATIVE_ALGORITHM" -b "$ALTERNATIVE_BITS" "$zone")

cat "$infile" "$keyname1.key" "$keyname2.key" >"$zonefile"

"$SIGNER" -g -o "$zone" -k "$keyname1" "$zonefile" "$keyname2" >/dev/null 2>&1
