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

echo_i "ns1/sign.sh"

ksk=$("$KEYGEN" -q -fk -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")
zsk=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")

cat "$infile" "$ksk.key" "$zsk.key" >"$zonefile"

SALT="$(printf "%04x" "$(($(date +%s) / 3600 % 65536))")"
echo_ic "NSEC3 salt for this hour: $SALT"
"$SIGNER" -3 "$SALT" -o "$zone" "$zonefile" 2>&1 >"$zonefile.sign.log"

keyfile_to_initial_ds "$ksk" >managed-keys.conf
