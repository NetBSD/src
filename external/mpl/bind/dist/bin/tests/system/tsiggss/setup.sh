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

key=$($KEYGEN -Cq -K ns1 -a $DEFAULT_ALGORITHM -b $DEFAULT_BITS -n HOST -T KEY key.example.nil.)
cat ns1/example.nil.db.in ns1/${key}.key >ns1/example.nil.db
