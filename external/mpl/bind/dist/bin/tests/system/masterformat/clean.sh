#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

rm -f named-compilezone
rm -f ns1/example.db.raw*
rm -f ns1/example.db.compat
rm -f ns1/example.db.serial.raw
rm -f ns1/large.db ns1/large.db.raw
rm -f ns1/example.db.map ns1/signed.db.map
rm -f ns1/session.key
rm -f dig.out.*
rm -f dig.out
rm -f */named.memstats
rm -f */named.conf
rm -f */named.run
rm -f ns2/example.db
rm -f ns2/transfer.db.*
rm -f ns2/formerly-text.db
rm -f ns2/db-*
rm -f ns2/large.bk
rm -f ns3/example.db.map ns3/dynamic.db.map
rm -f baseline.txt text.1 text.2 raw.1 raw.2 map.1 map.2 map.5 text.5 badmap
rm -f ns1/Ksigned.* ns1/dsset-signed. ns1/signed.db.signed
rm -f rndc.out
rm -f ns*/named.lock
