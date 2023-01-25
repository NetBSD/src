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

set -e

rm -f dig.out.* rndc.signing.* update.out.* verify.out.*
rm -f ns*/named.conf ns*/named.memstats ns*/named.run*
rm -f ns*/*.jnl ns*/*.jbk ns*/managed-keys.bind
rm -f ns*/K*.private ns*/K*.key ns*/K*.state
rm -f ns*/dsset-* ns*/*.db ns*/*.db.signed

