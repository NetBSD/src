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

rm -f dig.ns*.test*
rm -f ns*/named.conf
rm -f ns*/named.lock
rm -f ns*/named.memstats
rm -f ns*/named.run
rm -f ns1/dynamic.db
rm -f ns1/dynamic.db.jnl
rm -f ns2/dynamic.bk
rm -f ns2/dynamic.bk.jnl
rm -f ns2/example.bk
rm -f ns*/managed-keys.bind*
