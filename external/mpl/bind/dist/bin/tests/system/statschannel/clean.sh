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

rm -f traffic traffic.out.* traffic.json.* traffic.xml.*
rm -f zones zones.out.* zones.json.* zones.xml.* zones.expect.*
rm -f dig.out*
rm -f */named.memstats
rm -f */named.conf
rm -f */named.run*
rm -f ns*/named.lock
rm -f ns*/named.stats
rm -f xml.*stats json.*stats
rm -f xml.*mem json.*mem
rm -f compressed.headers regular.headers compressed.out regular.out
rm -f ns*/managed-keys.bind*
rm -f ns2/Kdnssec* ns2/dnssec.*.id
rm -f ns2/dnssec.db.signed* ns2/dsset-dnssec.
rm -f ns2/core
