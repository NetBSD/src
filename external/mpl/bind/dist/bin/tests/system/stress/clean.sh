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

rm -f reload.pid

rm -f ns?/zones.conf
rm -f ns?/zone*.bk

rm -f ns1/delegations.db
rm -f ns1/root.db

rm -f ns2/zone0*.db
rm -f ns2/zone0*.jnl
rm -f */named.memstats
rm -f ns*/named.lock
rm -f ns*/managed-keys.bind*
rm -f ns*/named.run
rm -f ns*/named.conf
