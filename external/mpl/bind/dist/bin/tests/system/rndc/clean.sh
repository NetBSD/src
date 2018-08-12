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

rm -f dig.out.*.test*
rm -f ns*/named.lock
rm -f ns*/named.memstats
rm -f ns*/named.run
rm -f ns2/named.stats
rm -f ns2/nil.db ns2/other.db ns2/static.db ns2/*.jnl
rm -f ns2/session.key
rm -f ns3/named_dump.db
rm -f ns4/*.nta
rm -f ns4/key?.conf
rm -f ns6/huge.zone.db
rm -f ns*/named.conf
rm -f nsupdate.out.*.test*
rm -f python.out.*.test*
rm -f rndc.out.*.test*
