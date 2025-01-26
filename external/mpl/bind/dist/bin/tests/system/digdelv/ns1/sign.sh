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

(cd ../ns2 && $SHELL sign.sh)

cp "../ns2/dsset-example." .

ksk=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone .)

cp root.db.in root.db

"$SIGNER" -Sgz -f root.db -o . root.db.in >/dev/null 2>&1

keyfile_to_key_id "$ksk" >keyid
grep -Ev '^;' <"$ksk.key" | cut -f 7- -d ' ' >keydata
keyfile_to_initial_keys "$ksk" >anchor.dnskey
