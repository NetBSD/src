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

zone=.
infile=root.db.in
zonefile=root.db

echo_i "ns1/setup.sh"

cp "../ns2/dsset-checkds." .

ksk=$("$KEYGEN" -q -fk -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
zsk=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
cp "$ksk.key" "../ns10/"
cp "$zsk.key" "../ns10/"
cp "$ksk.private" "../ns10/"
cp "$zsk.private" "../ns10/"

cat "$infile" "$ksk.key" "$zsk.key" >"$zonefile"
"$SIGNER" -g -o "$zone" "$zonefile" >/dev/null 2>&1

(
  cd ../ns10
  cat "$infile" "$ksk.key" "$zsk.key" >"$zonefile"
  "$SIGNER" -g -o "$zone" "$zonefile" >/dev/null 2>&1
)

# Configure the resolving server with a static key.
keyfile_to_static_ds "$ksk" >trusted.conf
cp trusted.conf ../ns3/trusted.conf
cp trusted.conf ../ns8/trusted.conf
