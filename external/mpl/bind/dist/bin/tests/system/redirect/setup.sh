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

. ../conf.sh

cp ns2/redirect.db.in ns2/redirect.db
cp ns2/example.db.in ns2/example.db
(cd ns1 && $SHELL sign.sh)

cp ns4/example.db.in ns4/example.db
(cd ns3 && $SHELL sign.sh)
(cd ns5 && $SHELL sign.sh)
