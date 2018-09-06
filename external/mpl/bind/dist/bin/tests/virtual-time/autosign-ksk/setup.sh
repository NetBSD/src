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

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh
. ./clean.sh

../../../tools/genrandom 800 random.data
dd if=random.data of=random.data1 bs=1k count=400 2> /dev/null
dd if=random.data of=random.data2 bs=1k skip=400 2> /dev/null

cd ns1 && sh sign.sh

