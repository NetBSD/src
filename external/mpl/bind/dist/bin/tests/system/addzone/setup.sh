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

cp -f ns1/redirect.db.1 ns1/redirect.db
cp -f ns2/redirect.db.1 ns2/redirect.db
cp -f ns3/redirect.db.1 ns3/redirect.db

cp -f ns2/default.nzf.in ns2/3bf305731dd26307.nzf
mkdir ns2/new-zones
