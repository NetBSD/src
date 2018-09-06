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

rm -f dig.out.ns?.test*
rm -f */dsset-*
rm -f */named.conf
rm -f */named.memstats
rm -f */named.run
rm -f */trusted.conf
rm -f ns1/K.*
rm -f ns1/root.db
rm -f ns1/root.db.signed
rm -f ns2/Kexample.*
rm -f ns2/example.db
rm -f ns2/example.db.signed
