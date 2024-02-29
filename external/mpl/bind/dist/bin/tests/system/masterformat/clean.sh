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

rm -f ./ns1/example.db.raw*
rm -f ./ns1/example.db.compat
rm -f ./ns1/example.db.serial.raw
rm -f ./ns1/large.db ./ns1/large.db.raw
rm -f ./ns1/signed.db.raw
rm -f ./ns1/session.key
rm -f ./dig.out.*
rm -f ./dig.out
rm -f ./*/named.memstats
rm -f ./*/named.conf
rm -f ./*/named.run
rm -f ./ns2/example.db
rm -f ./ns2/transfer.db.*
rm -f ./ns2/formerly-text.db
rm -f ./ns2/db-*
rm -f ./ns2/large.bk
rm -f ./ns3/example.db.raw ./ns3/dynamic.db.raw
rm -f ./baseline.txt ./text.* ./raw.*
rm -f ./ns1/Ksigned.* ./ns1/dsset-signed. ./ns1/signed.db.signed
rm -f ./rndc.out
rm -f ./ns*/named.lock
rm -f ./ns*/managed-keys.bind*
