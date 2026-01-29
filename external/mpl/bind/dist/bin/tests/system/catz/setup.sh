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

. ../conf.sh

cp -f ns1/catalog.example.db.in ns1/catalog0.example.db
cp -f ns1/catalog.example.db.in ns1/catalog1.example.db
cp -f ns3/catalog.example.db.in ns3/catalog2.example.db
cp -f ns1/catalog.example.db.in ns1/catalog3.example.db
cp -f ns1/catalog.example.db.in ns1/catalog4.example.db
# catalog5 is missing on purpose
cp -f ns1/catalog.example.db.in ns1/catalog6.example.db
cp -f ns1/catalog.example.db.in ns1/catalog-misc.example.db
cp -f ns1/catalog.example.db.in ns1/catalog-tls.example.db
cp -f ns4/catalog.example.db.in ns4/catalog-self.example.db

mkdir -p ns2/zonedir
