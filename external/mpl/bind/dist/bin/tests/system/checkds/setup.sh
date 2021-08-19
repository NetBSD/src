#!/bin/sh -e
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# shellcheck source=conf.sh
. ../conf.sh

set -e

$SHELL clean.sh

copy_setports ns2/named.conf.in ns2/named.conf
copy_setports ns4/named.conf.in ns4/named.conf
copy_setports ns5/named.conf.in ns5/named.conf
copy_setports ns6/named.conf.in ns6/named.conf
copy_setports ns7/named.conf.in ns7/named.conf
copy_setports ns9/named.conf.in ns9/named.conf

# Setup zones
(
	cd ns9
	$SHELL setup.sh
)
(
	cd ns5
	$SHELL setup.sh
)
(
	cd ns2
	$SHELL setup.sh
)
