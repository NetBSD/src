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
# Clean up after resolver tests.
#
rm -f */named.conf
rm -f */named.memstats
rm -f */named.run
rm -f */ans.run
rm -f */*.jdb
rm -f dig.out dig.out.*
rm -f dig.*.out.*
rm -f dig.*.foo.*
rm -f dig.*.bar.*
rm -f dig.*.prime.*
rm -f ns4/tld.db
rm -f ns6/K*
rm -f ns6/example.net.db.signed ns6/example.net.db
rm -f ns6/ds.example.net.db.signed ns6/ds.example.net.db
rm -f ns6/dsset-ds.example.net*
rm -f ns6/dsset-example.net* ns6/example.net.db.signed.jnl
rm -f ns6/to-be-removed.tld.db ns6/to-be-removed.tld.db.jnl
rm -f ns7/server.db ns7/server.db.jnl
rm -f resolve.out.*.test*
rm -f .digrc
rm -f ns*/named.lock
rm -f ns5/trusted.conf
