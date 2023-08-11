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

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

copy_setports ns1/named.conf.in ns1/named.conf
copy_setports ns2/named.conf.in ns2/named.conf
copy_setports ns3/named.conf.in ns3/named.conf
copy_setports ns4/named.conf.in ns4/named.conf

$SHELL ../genzone.sh 2 4 | sed -e 's/^$TTL 3600$/$TTL 0 ; force TTL to zero/' -e 's/86400.IN SOA/0 SOA/' > ns2/example.db
