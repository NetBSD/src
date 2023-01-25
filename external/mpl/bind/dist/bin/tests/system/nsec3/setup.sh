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

# shellcheck source=conf.sh
. ../conf.sh

set -e

$SHELL clean.sh

copy_setports ns2/named.conf.in ns2/named.conf
(
	cd ns2
	$SHELL setup.sh
)
copy_setports ns3/named.conf.in ns3/named.conf
(
	cd ns3
	$SHELL setup.sh
)
