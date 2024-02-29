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

$SHELL clean.sh
$SHELL ${TOP_SRCDIR}/bin/tests/system/genzone.sh 2 3 >ns2/example.db
$SHELL ${TOP_SRCDIR}/bin/tests/system/genzone.sh 2 3 >ns2/tsigzone.db
copy_setports ns2/named1.conf.in ns2/named.conf
copy_setports ns3/named.conf.in ns3/named.conf
copy_setports ns4/named.conf.in ns4/named.conf
