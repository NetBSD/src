#!/bin/sh -ef

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

# RSASHA1 is validate only in FIPS mode so we need to have a pre-signed
# version of the zone to test with in FIPS mode.  This requires a non
# FIPS build which supports RSASHA1 to be used to generate it.

. ../../conf.sh

zone=rsasha1-1024.example
k1=$("$KEYGEN" -a rsasha1 -b 1024 $zone)
k2=$("$KEYGEN" -a rsasha1 -b 1024 -f KSK $zone)
cat $zone.db.in $k1.key $k2.key >$zone.tmp
# use maximum expirey period (-e 2^31-1-3600)
# use output format full for easy extraction of KSK (-O full)
"$SIGNER" -e +2147480047 -o $zone -f $zone.db -O full $zone.tmp
rm -f $k1.key $k1.private $k2.key $k2.private $zone.tmp
