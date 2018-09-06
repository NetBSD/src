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

rm -f */named.conf */named.memstats */ans.run */named.recursing */named.run
rm -f dig.out*
rm -f ans4/norespond
rm -f ns3/named.stats ns3/named_dump.db
rm -f burst.input.*
