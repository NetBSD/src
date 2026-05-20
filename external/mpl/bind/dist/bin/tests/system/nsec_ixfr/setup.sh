#!/bin/sh

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
. ../conf.sh

set -e

# Start with the unsigned zone (serial 1).
cp ns1/example.db.in ns1/example.db

# Generate DNSSEC keys for NSEC signing.
"$KEYGEN" -q -f KSK -a "$DEFAULT_ALGORITHM" -K ns1 example >/dev/null
"$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -K ns1 example >/dev/null

# Create the signed zone file with serial 2.
# Bump the serial, then sign with plain NSEC (no -3 flag).
# The -S flag tells dnssec-signzone to automatically find keys and
# include DNSKEY records.
sed 's/1\([	 ]*;[	 ]*serial\)/2\1/' ns1/example.db.in >ns1/example.db.tosign
"$SIGNER" -P -S -K ns1 -o example -f ns1/example.db.signed \
  ns1/example.db.tosign >/dev/null 2>&1
rm -f ns1/example.db.tosign
