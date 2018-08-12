# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# Clean up after rpz tests.

rm -f proto.* dsset-* trusted.conf dig.out* nsupdate.tmp ns*/*tmp
rm -f ns*/*.key ns*/*.private ns2/tld2s.db ns2/bl.tld2.db
rm -f ns3/bl*.db ns*/*switch ns*/empty.db ns*/empty.db.jnl
rm -f ns5/requests ns5/example.db ns5/bl.db ns5/*.perf
rm -f */named.memstats */*.run */named.stats */session.key
rm -f */*.log */*.jnl */*core */*.pid
rm -f */policy2.db
rm -f ns*/named.lock
rm -f ns*/named.conf
rm -f dnsrps*.conf
rm -f dnsrpzd.conf
rm -f dnsrpzd-license-cur.conf dnsrpzd.rpzf dnsrpzd.sock dnsrpzd.pid
rm -f tmp
