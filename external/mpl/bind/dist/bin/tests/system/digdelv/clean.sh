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

rm -f ./*/named.memstats
rm -f ./*/named.run
rm -f ./*/named.conf
rm -f ./delv.out.test*
rm -f ./dig.out.*test*
rm -f ./dig.out.mm.*
rm -f ./dig.out.mn.*
rm -f ./dig.out.nm.*
rm -f ./dig.out.nn.*
rm -f ./ns*/named.lock
rm -f ./ns*/managed-keys.bind*
rm -f ./ns2/example.db ./ns2/K* ./ns2/keyid ./ns2/keydata
