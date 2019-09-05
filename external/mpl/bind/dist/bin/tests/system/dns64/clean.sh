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

rm -f */named.memstats
rm -f */named.run
rm -f checkconf.out*
rm -f dig.out.*
rm -f ns*/managed-keys.bind*
rm -f ns*/named.conf
rm -f ns*/named.lock
rm -f ns1/K*
rm -f ns1/dsset-signed.
rm -f ns1/signed.db*
