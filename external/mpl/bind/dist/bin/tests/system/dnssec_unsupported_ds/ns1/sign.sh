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

echo_i "ns1/sign.sh"

zone=.

mkdir -p keys
ksk=$("$KEYGEN" -K keys -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -f KSK "${zone}")
zsk=$("$KEYGEN" -K keys -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "${zone}")

cat "zones/root.db.in" "keys/$ksk.key" "keys/$zsk.key" ../ns2/dsset-example. >"zones/root.db"

"$SIGNER" -S -K "keys" \
  -o . \
  -f "zones/root.db.signed" \
  "zones/root.db" >/dev/null 2>&1

keyfile_to_static_ds "keys/$ksk" >trusted.conf
cp trusted.conf ../ns2/trusted.conf
cp trusted.conf ../ns3/trusted.conf
cp trusted.conf ../ns4/trusted.conf
