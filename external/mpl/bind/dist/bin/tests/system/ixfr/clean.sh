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

rm -f stats.*
rm -f ns1/*.db ns1/*.jnl
rm -f ns3/*.jnl ns3/mytest.db ns3/subtest.db
rm -f ns4/*.jnl ns4/*.db
rm -f ns5/*.jnl ns5/*.db
rm -f */named.memstats
rm -f */named.conf
rm -f */named.run
rm -f */ans.run
rm -f dig.out.test* dig.out1.test* dig.out2.test* dig.out3.test*
rm -f ns3/large.db
rm -f ns*/named.lock
rm -f ns*/managed-keys.bind* ns*/*.mkeys
