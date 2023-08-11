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

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

cp ns4/tld1.db ns4/tld.db
cp ns6/to-be-removed.tld.db.in ns6/to-be-removed.tld.db
cp ns7/server.db.in ns7/server.db

copy_setports ns1/named.conf.in ns1/named.conf
copy_setports ns4/named.conf.in ns4/named.conf
copy_setports ns5/named.conf.in ns5/named.conf
copy_setports ns6/named.conf.in ns6/named.conf
copy_setports ns7/named1.conf.in ns7/named.conf
copy_setports ns9/named.conf.in ns9/named.conf

(cd ns6 && $SHELL keygen.sh)
