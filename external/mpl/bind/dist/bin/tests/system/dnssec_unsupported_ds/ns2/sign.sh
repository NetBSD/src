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

echo_i "ns2/sign.sh"

zone=example.

mkdir -p keys
ksk=$("$KEYGEN" -K keys -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -f KSK "${zone}")
zsk=$("$KEYGEN" -K keys -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "${zone}")

cat "zones/${zone}db.in" "keys/$ksk.key" "keys/$zsk.key" >"zones/${zone}db"

<"../ns3/dsset-child.${zone}" sed -E "s/[[:space:]]+$DEFAULT_ALGORITHM_NUMBER[[:space:]]+/ 12 /" >>"zones/${zone}db"
<"../ns3/dsset-child.${zone}" sed -E "s/[[:space:]]+$DEFAULT_ALGORITHM_NUMBER[[:space:]]+2[[:space:]]+.*/ $DEFAULT_ALGORITHM_NUMBER 2 AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA/" >>"zones/${zone}db"

"$SIGNER" -S -K "keys" \
  -o "${zone}" \
  -f "zones/${zone}db.signed" \
  "zones/${zone}db" >/dev/null 2>&1
