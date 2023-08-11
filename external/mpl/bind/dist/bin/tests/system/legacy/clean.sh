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

rm -f dig.out.*
rm -f ns*/named.conf
rm -f ns*/named.memstats
rm -f ns*/named.run
rm -f ns*/named.lock

# build.sh
rm -f ns1/named_dump.db*
rm -f ns6/K*
rm -f ns6/dsset-*
rm -f ns6/edns512.db
rm -f ns6/signer.err
rm -f ns7/K*
rm -f ns7/dsset-*
rm -f ns7/edns512-notcp.db
rm -f ns7/signer.err
rm -f ns7/trusted.conf
rm -f ns*/managed-keys.bind*
