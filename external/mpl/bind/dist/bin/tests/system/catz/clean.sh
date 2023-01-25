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

rm -f dig.out.*
rm -f ns*/*.jnl
rm -f ns*/*.nzf
rm -f ns*/named.lock
rm -f ns*/named.memstats
rm -f ns*/named.conf
rm -f ns*/named.run
rm -f ns*/named.run.prev
rm -f ns1/*dom*example.db
rm -f ns2/__catz__*db
rm -f ns2/named.conf.tmp
rm -f ns3/dom13.example.db ns3/dom14.example.db
rm -f nsupdate.out.*
rm -f ns[123]/catalog[1234].example.db
rm -rf ns2/zonedir
rm -f ns*/*.nzd ns*/*.nzd-lock
rm -f ns*/managed-keys.bind*
rm -f wait_for_message.*
