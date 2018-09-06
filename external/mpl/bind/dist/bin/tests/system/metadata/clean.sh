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

rm -f K* dsset-* *.signed *.new
rm -f zsk.key ksk.key parent.ksk.key parent.zsk.key 
rm -f pending.key rolling.key standby.key inact.key
rm -f prerev.key postrev.key oldstyle.key
rm -f keys sigs
rm -f tmp.out
rm -f settime1.test* settime2.test*
rm -f ns*/named.lock
