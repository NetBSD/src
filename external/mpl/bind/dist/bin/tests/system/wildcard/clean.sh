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

rm -f ns*/named.run
rm -f ns*/named.conf
rm -f ns1/K*
rm -f ns1/*.db
rm -f ns1/*.signed
rm -f ns1/dsset-*
rm -f ns1/keyset-*
rm -f ns1/trusted.conf
rm -f ns1/private.nsec.conf
rm -f ns1/private.nsec3.conf
rm -f ns1/signer.err
rm -f */named.memstats
rm -f dig.out.ns*.test*
rm -f ns*/named.lock
rm -f ns*/managed-keys.bind*
