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

SYSTEMTESTTOP=../..
. $SYSTEMTESTTOP/conf.sh

zone=signed
infile=example.db
zonefile=signed.db

key1=$($KEYGEN -q -a $DEFAULT_ALGORITHM $zone)
key2=$($KEYGEN -q -a $DEFAULT_ALGORITHM -fk $zone)

cat $infile $key1.key $key2.key > $zonefile

$SIGNER -P -g -o $zone $zonefile > /dev/null
