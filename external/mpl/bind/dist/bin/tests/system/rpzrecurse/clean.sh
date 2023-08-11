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

# Clean up after rpz tests.

rm -f dig.out.*

rm -f ns*/named.lock
rm -f ns*/named.memstats
rm -f ns*/*.run
rm -f ns*/*core *core
rm -f ns*/named.conf

rm -f ns2/*.local
rm -f ns2/*.queries
rm -f ns2/named.[0-9]*.conf
rm -f ns2/named.conf.header

rm -f ns3/named2.conf
rm -f ns3/named.run.prev

rm -f dnsrps*.conf dnsrpzd*
rm -f ns*/session.key
rm -f ns*/managed-keys.bind* ns*/*.mkeys*
