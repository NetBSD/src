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

ksk=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone example.)

cp example.db.in example.db

"$SIGNER" -Sz -f example.db -o example example.db.in >/dev/null 2>&1

keyfile_to_key_id "$ksk" >keyid
grep -Ev '^;' <"$ksk.key" | cut -f 7- -d ' ' >keydata

keyfile_to_initial_keys "$ksk" >../ns3/anchor.dnskey
keyfile_to_initial_ds "$ksk" >../ns3/anchor.ds
