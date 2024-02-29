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

$SHELL clean.sh

copy_setports ns2/named1.conf.in ns2/named.conf

copy_setports ns2/named-alt1.conf.in ns2/named-alt1.conf
copy_setports ns2/named-alt2.conf.in ns2/named-alt2.conf
copy_setports ns2/named-alt3.conf.in ns2/named-alt3.conf
copy_setports ns2/named-alt4.conf.in ns2/named-alt4.conf
copy_setports ns2/named-alt5.conf.in ns2/named-alt5.conf
copy_setports ns2/named-alt6.conf.in ns2/named-alt6.conf
copy_setports ns2/named-alt7.conf.in ns2/named-alt7.conf

mkdir ns2/nope
chmod 555 ns2/nope
