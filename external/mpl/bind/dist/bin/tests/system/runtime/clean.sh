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

rm -f ns2/named.conf ns2/named-alt*.conf
rm -f */named.memstats
rm -f */named*.run
rm -f ns*/named.lock ns*/named*.pid ns*/other.lock
rm -f *.pid
rm -f rndc.out*
[ -d ns2/nope ] && chmod 755 ns2/nope
rm -rf ns2/nope
