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

rm -f ns*/named.conf
rm -f dig.out.ns?.test*
rm -f nsupdate.out.test*
rm -f ns1/*.example.db
rm -f ns1/*.update.db
rm -f ns1/*.update.db.jnl
rm -f ns4/*.update.db
rm -f ns4/*.update.db.jnl
rm -f */named.memstats
rm -f */named.run
rm -f ns*/named.lock
