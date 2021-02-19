#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

rm -f dig.out.*
rm -f rndc.out*
rm -f */named.memstats
rm -f ns2/*.nzf
rm -f ns2/*.nzd ns2/*nzd-lock
rm -f ns2/core*
rm -f ns2/inline.db.jbk
rm -f ns2/inline.db.signed
rm -f ns2/inlinesec.bk*
rm -f ns*/named.lock
rm -f ns2/nzf-*
rm -f ns*/managed-keys.bind*
