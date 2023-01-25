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

zone=.
zonefile=root.db

# a key for a trust island
islandkey=$($KEYGEN -a ${DEFAULT_ALGORITHM} -qfk island.)

# a key with unsupported algorithm
unsupportedkey=Kunknown.+255+00000
cp unsupported-managed.key "${unsupportedkey}.key"

# root key
rootkey=$(cat ../ns1/managed.key)
cp "../ns1/${rootkey}.key" .

# Configure the resolving server with an initializing key.
# (We use key-format trust anchors here because otherwise the
# unsupported algorithm test won't work.)
keyfile_to_initial_keys $unsupportedkey $islandkey $rootkey > managed.conf
