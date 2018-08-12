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

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

$SHELL clean.sh

test -r $RANDFILE || $GENRANDOM 800 $RANDFILE

copy_setports ns1/named.conf.in ns1/named.conf

key=`$KEYGEN -Cq -K ns1 -a DSA -b 512 -r $RANDFILE -n HOST -T KEY key.example.nil.`
cat ns1/example.nil.db.in ns1/${key}.key > ns1/example.nil.db
