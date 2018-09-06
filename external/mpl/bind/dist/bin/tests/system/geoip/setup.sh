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

copy_setports ns2/named1.conf.in ns2/named.conf

for i in 1 2 3 4 5 6 7 other bogus; do
        cp ns2/example.db.in ns2/example${i}.db
        echo "@ IN TXT \"$i\"" >> ns2/example$i.db
done

mkdir -p data2
cp data/GeoIPv6.dat data2/
