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

#
# Clean up after zone transfer tests.
#

rm -f */ans.run
rm -f */named.conf
rm -f */named.memstats
rm -f */named.run
rm -f */named.run.prev
rm -f axfr.out
rm -f dig.out.*
rm -f ns*/managed-keys.bind*
rm -f ns*/named.lock
rm -f ns1/dot-fallback.db
rm -f ns1/edns-expire.db
rm -f ns1/ixfr-too-big.db ns1/ixfr-too-big.db.jnl
rm -f ns1/sec.db ns2/sec.db
rm -f ns2/example.db ns2/tsigzone.db ns2/example.db.jnl ns2/dot-fallback.db
rm -f ns2/mapped.db
rm -f ns3/example.bk ns3/xfer-stats.bk ns3/tsigzone.bk ns3/example.bk.jnl
rm -f ns3/mapped.bk
rm -f ns3/primary.bk ns3/primary.bk.jnl
rm -f ns4/*.db ns4/*.jnl
rm -f ns6/*.db ns6/*.bk ns6/*.jnl
rm -f ns7/*.db ns7/*.bk ns7/*.jnl
rm -f ns8/large.db ns8/small.db
rm -f stats.*
rm -f wait_for_message.*
