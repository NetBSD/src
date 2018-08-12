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
# Clean up after zone transfer quota tests.
#

rm -f ns1/zone*.example.db ns1/zones.conf
rm -f ns2/zone*.example.bk ns2/zones.conf
rm -f dig.out.* ns2/changing.bk
rm -f ns1/changing.db
rm -f */named.memstats
rm -f */named.conf
rm -f */named.run
rm -f ns*/named.lock
