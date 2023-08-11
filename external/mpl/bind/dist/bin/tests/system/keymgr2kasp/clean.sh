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

rm -f ns*/K*.private ns*/K*.key ns*/K*.state
rm -f ns*/named.conf ns*/kasp.conf
rm -f ns*/named.memstats ns*/named.run
rm -f ns*/keygen.out* ns*/signer.out*
rm -f ns*/zones
rm -f ns*/dsset-*
rm -f ns*/*.db ns*/*.db.jnl ns*/*.db.jbk
rm -f ns*/*.db.signed* ns*/*.db.infile
rm -f ns*/managed-keys.bind*
rm -f ns*/*.mkeys*
rm -f ./*.created
rm -f ./created.key-*
rm -f ./dig.out*
rm -f ./python.out.*
rm -f ./retired.*
rm -f ./rndc.dnssec.*
rm -f ./unused.key*
rm -f ./verify.out.*

