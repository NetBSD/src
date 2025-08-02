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
if $FEATURETEST --have-fips-dh; then
  copy_setports ns3/named-tls.conf.in ns3/named-tls.conf
  copy_setports ns3/options-tls.conf.in ns3/options-tls.conf
  copy_setports ns3/named.conf.in ns3/named.conf
else
  cp /dev/null ns3/named-tls.conf
  cp /dev/null ns3/options-tls.conf
  copy_setports ns3/named.conf.in ns3/named.conf
fi
copy_setports ns4/named.conf.in ns4/named.conf
copy_setports ns5/named.conf.in ns5/named.conf

copy_setports ns4/named.port.in ns4/named.port

cp -f ns2/example1.db ns2/example.db
cp -f ns2/generic.db ns2/x21.db
