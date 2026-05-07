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

. ../conf.sh

sed -e s/big[.]/signed./g <ns1/big.db >ns1/signed.db
$KEYGEN -K ns1 -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -f KSK signed >/dev/null 2>&1
$KEYGEN -K ns1 -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" signed >/dev/null 2>&1
$SIGNER -K ns1 -S -f ns1/signed.db.signed -o signed ns1/signed.db >/dev/null
