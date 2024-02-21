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

. ../../conf.sh

zone=sub.foo
zonefile=sub.foo.db

keyname=$($KEYGEN -a ${DEFAULT_ALGORITHM} -qfk $zone)
zskkeyname=$($KEYGEN -a ${DEFAULT_ALGORITHM} -q $zone)

$SIGNER -Sg -o $zone $zonefile >/dev/null 2>/dev/null
keyfile_to_initial_ds $keyname >private.conf
cp private.conf ../ns5/private.conf
