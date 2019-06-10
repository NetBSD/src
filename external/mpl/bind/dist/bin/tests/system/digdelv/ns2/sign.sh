#!/bin/sh -e
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# shellcheck source=conf.sh
. "$SYSTEMTESTTOP/conf.sh"

set -e

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "dnskey.example.")

cp example.db.in example.db

cat "$keyname.key" >> example.db

echo "$keyname" | sed -e 's/.*[+]//' -e 's/^0*//' > keyid
< "$keyname.key" grep -Ev '^;' | cut -f 7- -d ' ' > keydata
