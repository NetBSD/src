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

#
# Clean up after forward tests.
#
rm -f ./ans11/query.log
rm -f ./dig.out.*
rm -f ./*/named.conf
rm -f ./*/named.memstats
rm -f ./*/named.run ./*/named.run.prev
rm -f ./*/named_dump.db
rm -f ./ns*/named.lock
rm -f ./ns*/managed-keys.bind*
rm -f ./ns1/root.db ./ns1/root.db.signed
rm -f ./ns*/trusted.conf
rm -f ./ns1/K* ./ns1/dsset-*
