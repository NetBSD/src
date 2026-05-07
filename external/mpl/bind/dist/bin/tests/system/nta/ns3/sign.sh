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

# a validly signed zone
zone=secure.example.
infile=secure.example.db.in
zonefile=secure.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -D -o "$zone" "$zonefile" >/dev/null
cat "$zonefile" "$zonefile".signed >"$zonefile".tmp
mv "$zonefile".tmp "$zonefile".signed

# a zone that we'll add bogus data to
zone=bogus.example.
infile=bogus.example.db.in
zonefile=bogus.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -z -o "$zone" "$zonefile" >/dev/null

{
  echo "a.bogus.example.	A	10.0.0.22"
  echo "b.bogus.example.	A	10.0.0.23"
  echo "c.bogus.example.	A	10.0.0.23"
} >>bogus.example.db.signed

#
# A zone with a bad DS in the parent
# (sourced from bogus.example.db.in)
#
zone=badds.example.
infile=bogus.example.db.in
zonefile=badds.example.db

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")

cat "$infile" "$keyname.key" >"$zonefile"

"$SIGNER" -P -o "$zone" "$zonefile" >/dev/null
sed -e 's/bogus/badds/g' <dsset-bogus.example. >dsset-badds.example.
