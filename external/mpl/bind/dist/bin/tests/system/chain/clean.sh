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

rm -f dig.out.* named*.pid
rm -f ns*/named.conf
rm -f */named.memstats */named.recursing */named.lock */named.run */ans.run
rm -f ns2/K* ns2/dsset-* ns2/example.db.signed
