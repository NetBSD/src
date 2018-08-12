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

rm -f tmp
rm -f dig.out.*
rm -f ns*/named.lock
rm -f ns*/named.conf
rm -f ns3/example.db
rm -f ns3/undelegated.db
rm -f ns4/sub.example.db
rm -f ns?/named.memstats
rm -f ns?/named.run
rm -f ns?/named_dump.db
rm -rf */*.signed
rm -rf */K*
rm -rf */dsset-*
rm -rf */trusted.conf
