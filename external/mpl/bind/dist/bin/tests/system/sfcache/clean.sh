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

set -e

rm -f ./*/K*.key ./*/K*.private ./*/*.signed ./*/*.db ./*/dsset-*
rm -f ./*/managed.conf ./*/trusted.conf
rm -f ./*/named.memstats
rm -f ./*/named.conf
rm -f ./*/named.run ./*/named.run.prev
rm -f ./dig.*
rm -f ./rndc.*
rm -f ./sfcache.*
rm -f ./ns*/managed-keys.bind*
rm -f ./ns*/named.lock
rm -f ./ns5/named.run.part*
rm -f ./ns5/named_dump*
