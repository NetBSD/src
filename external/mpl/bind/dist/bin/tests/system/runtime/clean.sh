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

[ -d ns2/nope ] && chmod 755 ns2/nope

rm -f *.pid
rm -f */named*.run
rm -f */named.memstats
rm -f kill*.out
rm -f ns*/managed-keys.bind*
rm -f ns*/named.lock ns*/named*.pid ns*/other.lock
rm -f ns2/named.conf ns2/named-alt*.conf
rm -f rndc.out*
rm -rf ns2/nope
rm -rf ns2/tmp.*
