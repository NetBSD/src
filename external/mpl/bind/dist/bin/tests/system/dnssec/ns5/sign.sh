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

zone=.
infile=../ns1/root.db.in
zonefile=root.db.signed

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -f KSK "$zone")

# copy the KSK out first, then revoke it
keyfile_to_managed_keys "$keyname" > revoked.conf

"$SETTIME" -R now "${keyname}.key" > /dev/null

# create a current set of keys, and sign the root zone
"$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" $zone > /dev/null
"$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -f KSK $zone > /dev/null
"$SIGNER" -S -o "$zone" -f "$zonefile" "$infile" > /dev/null 2>&1

keyname=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone ".")

keyfile_to_trusted_keys "$keyname" > trusted.conf
