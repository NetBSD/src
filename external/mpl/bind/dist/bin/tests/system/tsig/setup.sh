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

if $FEATURETEST --md5; then
  copy_setports ns1/named-fips.conf.in ns1/named-fips.conf
  # includes named-fips.conf
  cp ns1/named.conf.in ns1/named.conf
else
  copy_setports ns1/named-fips.conf.in ns1/named.conf
fi
