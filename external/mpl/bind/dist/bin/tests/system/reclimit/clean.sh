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

rm -f dig.out*
rm -f ans?/ans.run
rm -f ans2/ans.limit
rm -f ans4/ans.limit
rm -f ns?/named.memstats
rm -f ns?/named.run
rm -f ns*/named.conf
rm -f ns*/named.lock
rm -f ns*/managed-keys.bind*
