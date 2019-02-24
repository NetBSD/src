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

#
# Clean up after zone transfer tests.
#

rm -f dig.out.*
rm -f axfr.out
rm -f stats.*
rm -f ns1/slave.db ns2/slave.db
rm -f ns1/edns-expire.db
rm -f ns2/example.db ns2/tsigzone.db ns2/example.db.jnl
rm -f ns3/example.bk ns3/xfer-stats.bk ns3/tsigzone.bk ns3/example.bk.jnl
rm -f ns3/master.bk ns3/master.bk.jnl
rm -f ns4/nil.db ns4/root.db
rm -f ns6/*.db ns6/*.bk ns6/*.jnl
rm -f ns7/*.db ns7/*.bk ns7/*.jnl
rm -f ns8/large.db ns8/small.db
rm -f */named.conf
rm -f */named.run
rm -f */named.memstats
rm -f */named.run
rm -f */ans.run
rm -f ns*/named.lock
rm -f ns2/mapped.db
rm -f ns3/mapped.bk
rm -f ns1/ixfr-too-big.db ns1/ixfr-too-big.db.jnl
rm -f ns*/managed-keys.bind*
