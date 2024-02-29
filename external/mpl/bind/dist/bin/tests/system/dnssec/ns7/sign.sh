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

echo_i "ns7/sign.sh"

zone=split-rrsig
infile=split-rrsig.db.in
zonefile=split-rrsig.db

k1=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
k2=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$k1.key" "$k2.key" >"$zonefile"

# The awk script below achieves two goals:
#
# - it puts one of the two RRSIG(SOA) records at the end of the zone file, so
#   that these two records (forming a single RRset) are not placed immediately
#   next to each other; the test then checks if RRSIG RRsets split this way are
#   correctly added to resigning heaps,
#
# - it places a copy of one of the RRSIG(SOA) records somewhere else than at the
#   zone apex; the test then checks whether such signatures are automatically
#   removed from the zone after it is loaded.
"$SIGNER" -P -3 - -A -o "$zone" -O full -f "$zonefile.unsplit" -e now-3600 -s now-7200 "$zonefile" >/dev/null 2>&1
awk 'BEGIN { r = ""; }
     $4 == "RRSIG" && $5 == "SOA" && r == "" { r = $0; next; }
     { print }
     END { print r; print "not-at-zone-apex." r; }' "$zonefile.unsplit" >"$zonefile.signed"
