#!/bin/sh -e
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

SYSTEMTESTTOP=../..
. $SYSTEMTESTTOP/conf.sh

zone=example.
zonefile=example.db

ksk=`$KEYGEN -q -a RSASHA256 -b 2048 -fk $zone`
zsk=`$KEYGEN -q -a RSASHA256 -b 1024 $zone`
$SIGNER -S -o $zone example.db > /dev/null
