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

. ../conf.sh

# Drop unusual RR sets dnspython can't handle. For more information
# see https://github.com/rthalley/dnspython/issues/1034#issuecomment-1896541899.
$SHELL "${TOP_SRCDIR}/bin/tests/system/genzone.sh" 2 \
  | sed \
    -e '/AMTRELAY.*\# 2 0004/d' \
    -e '/GPOS.*"" "" ""/d' \
    -e '/URI.*30 40 ""/d' >ns1/example.db
