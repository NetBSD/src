#!/bin/sh -e

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

. ../../conf.sh

zone=tc-test-signed
infile=tc-test-signed.db.in
zonefile=tc-test-signed.db

# The signing algorithm and key sizes used here are NOT arbitrary - they have
# been carefully chosen to ensure that the signed referral response checked in
# the test will be around 512 bytes in size with glue records excluded.  Please
# keep this in mind when updating signing algorithms used in system tests.
keyname=$($KEYGEN -q -a RSASHA256 -b 2048 -n zone $zone)
cat "$infile" "$keyname.key" >"$zonefile"

$SIGNER -P -o $zone $zonefile >/dev/null
