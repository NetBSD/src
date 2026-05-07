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

# Explicitly setting ALGORITHM_SET is only needed is the script is executed
# standalone without the pytest runner (e.g. for debugging).
export ALGORITHM_SET="ecc_default"

. ../conf.sh

cp ns5/named1.args ns5/named.args

(cd ns1 && $SHELL sign.sh)
(cd ns4 && $SHELL sign.sh)
(cd ns6 && $SHELL setup.sh)

cp ns2/managed.conf ns2/managed1.conf

cd ns4
mkdir nope
touch nope/managed-keys.bind
touch nope/managed.keys.bind.jnl
chmod 444 nope/*
