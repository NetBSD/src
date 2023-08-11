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

rm -f */named.memstats
rm -f */named.run
rm -f */named.conf
rm -f dig.out.test*
rm -f ns2/example.com.bk
rm -f ns2/example.net.bk
rm -f ns*/managed-keys.bind* ns*/*mkeys*
