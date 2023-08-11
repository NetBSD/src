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

rm -f */*.conf
rm -f */*.db
rm -f */*.jnl
rm -f */*.mirror
rm -f */*.nzd*
rm -f */*.prev
rm -f */*.signed
rm -f */K*
rm -f */db-*
rm -f */dsset-*
rm -f */jn-*
rm -f */_default.nzf
rm -f */managed-keys.bind*
rm -f */named.memstats
rm -f */named.run
rm -f dig.out.*
rm -f rndc.out.*
