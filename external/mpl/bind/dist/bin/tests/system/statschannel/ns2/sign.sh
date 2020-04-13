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

zone=dnssec.
infile=dnssec.db.in
zonefile=dnssec.db.signed

ksk=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -f KSK "$zone")
zsk=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" "$zone")
# Sign deliberately with a very short expiration date.
"$SIGNER" -S -x -O full -e "now"+1s -o "$zone" -f "$zonefile" "$infile" > /dev/null 2>&1

keyfile_to_key_id "$ksk" > dnssec.ksk.id
keyfile_to_key_id "$zsk" > dnssec.zsk.id

