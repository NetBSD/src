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

rm -f ./keygen.*
rm -f ./K*.private ./K*.key ./K*.state ./K*.cmp
rm -rf ./keys/
rm -f dig.out* rrsig.out.* keyevent.out.*
rm -f ns*/named.conf ns*/named.memstats ns*/named.run*
rm -f ns*/*.jnl ns*/*.jbk
rm -f ns*/K*.private ns*/K*.key ns*/K*.state
rm -f ns*/dsset-* ns*/*.db ns*/*.db.signed
rm -f ns*/keygen.out.* ns*/settime.out.* ns*/signer.out.*
rm -f ns*/managed-keys.bind
rm -f ns*/*.mkeys
rm -f ns*/zones ns*/*.db.infile
rm -f *.created published.test* retired.test*
rm -f rndc.dnssec.status.out.*
rm -f python.out.*
