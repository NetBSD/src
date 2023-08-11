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

rm -f dig.out.ns1* dig.out.ns2 dig.out.ns1 dig.out.ns3 dig.out.ns1.after
rm -f ns1/*.jnl ns2/*.jnl ns3/*.jnl ns1/example.db ns2/*.bk ns3/*.bk
rm -f ns3/nomaster1.db
rm -f ns3/dnstap.out*
rm -f ns3/dnstap.conf
rm -f dnstap.out*
rm -f dnstapread.out*
rm -f */named.memstats
rm -f */named.run
rm -f */named.conf
rm -f */ans.run
rm -f Ksig0.example2.*
rm -f keyname keyname.err
rm -f ns*/named.lock
rm -f ns1/example2.db
rm -f ns*/managed-keys.bind*
rm -f nsupdate.out.*
rm -f ns*/named.run.prev
