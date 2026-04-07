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

# shellcheck source=conf.sh
. ../../conf.sh

echo_i "ns6/setup.sh"

$SIGNER -3 DEADBEEF -A -H 10 -o evil.test -t evil.test.db >/dev/null 2>&1
$CHECKZONE -s full -f text -F text -o evil.test.db.signed2 evil.test evil.test.db.signed >/dev/null 2>&1
mv evil.test.db.signed2 evil.test.db.signed
