#!/bin/sh

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

set -e

# shellcheck source=conf.sh
. ../conf.sh

cp -f ns2/example1.db ns2/example.db
#
# We remove k1 and k2 as KEYGEN is deterministic when given the
# same source of "random" data and we want different keys for
# internal and external instances of inline.
#
$KEYGEN -K ns2/internal -a ${DEFAULT_ALGORITHM} -q inline >/dev/null 2>&1
$KEYGEN -K ns2/internal -a ${DEFAULT_ALGORITHM} -qfk inline >/dev/null 2>&1
k1=$($KEYGEN -K ns2/external -a ${DEFAULT_ALGORITHM} -q inline 2>/dev/null)
k2=$($KEYGEN -K ns2/external -a ${DEFAULT_ALGORITHM} -qfk inline 2>/dev/null)
$KEYGEN -K ns2/external -a ${DEFAULT_ALGORITHM} -q inline >/dev/null 2>&1
$KEYGEN -K ns2/external -a ${DEFAULT_ALGORITHM} -qfk inline >/dev/null 2>&1
test -n "$k1" && rm -f ns2/external/"$k1".*
test -n "$k2" && rm -f ns2/external/"$k2".*
