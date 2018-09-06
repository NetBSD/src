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

rm -f ns*/named.conf
rm -f ns3/example.bk dig.out.ns?.?
rm -f ns2/example.db ns3/internal.bk
rm -f */*.jnl
rm -f */named.memstats
rm -f */named.run */named.run.prev
rm -f ns2/external/K*
rm -f ns2/external/inline.db.jbk
rm -f ns2/external/inline.db.signed
rm -f ns2/external/inline.db.signed.jnl
rm -f ns2/internal/K*
rm -f ns2/internal/inline.db.jbk
rm -f ns2/internal/inline.db.signed
rm -f ns2/internal/inline.db.signed.jnl
rm -f dig.out.external dig.out.internal
rm -f ns*/named.lock
