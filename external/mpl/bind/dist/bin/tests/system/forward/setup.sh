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

copy_setports ns1/named.conf.in ns1/named.conf

if $FEATURETEST --have-fips-dh; then
  copy_setports ns2/named-tls.conf.in ns2/named-tls.conf
  copy_setports ns2/options-tls.conf.in ns2/options-tls.conf
  copy_setports ns2/named.conf.in ns2/named.conf
else
  cp /dev/null ns2/named-tls.conf
  cp /dev/null ns2/options-tls.conf
  copy_setports ns2/named.conf.in ns2/named.conf
fi

copy_setports ns3/named1.conf.in ns3/named.conf

if $FEATURETEST --have-fips-dh; then
  copy_setports ns4/named-tls.conf.in ns4/named-tls.conf
  copy_setports ns4/options-tls.conf.in ns4/options-tls.conf
  copy_setports ns4/named.conf.in ns4/named.conf
else
  cp /dev/null ns4/named-tls.conf
  cp /dev/null ns4/options-tls.conf
  copy_setports ns4/named.conf.in ns4/named.conf
fi

copy_setports ns5/named.conf.in ns5/named.conf
copy_setports ns7/named.conf.in ns7/named.conf
copy_setports ns8/named.conf.in ns8/named.conf
copy_setports ns9/named1.conf.in ns9/named.conf
copy_setports ns10/named.conf.in ns10/named.conf

(
  cd ns1
  $SHELL sign.sh
)
