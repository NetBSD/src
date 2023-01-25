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
. "$SYSTEMTESTTOP/conf.sh"

set -e

$SHELL clean.sh

mkdir keys

copy_setports ns2/named.conf.in ns2/named.conf
if ! $SHELL ../testcrypto.sh -q RSASHA1
then
	copy_setports ns3/named-fips.conf.in ns3/named.conf
else
	copy_setports ns3/named-fips.conf.in ns3/named-fips.conf
	copy_setports ns3/named.conf.in ns3/named.conf
fi
copy_setports ns4/named.conf.in ns4/named.conf
copy_setports ns5/named.conf.in ns5/named.conf
copy_setports ns6/named.conf.in ns6/named.conf

if $SHELL ../testcrypto.sh ed25519; then
	echo "yes" > ed25519-supported.file
fi

if $SHELL ../testcrypto.sh ed448; then
	echo "yes" > ed448-supported.file
fi

copy_setports ns3/policies/autosign.conf.in ns3/policies/autosign.conf
copy_setports ns3/policies/kasp-fips.conf.in ns3/policies/kasp-fips.conf
copy_setports ns3/policies/kasp.conf.in ns3/policies/kasp.conf
if ! $SHELL ../testcrypto.sh -q RSASHA1
then
	cp ns3/policies/kasp-fips.conf ns3/policies/kasp.conf
fi

copy_setports ns6/policies/csk1.conf.in ns6/policies/csk1.conf
copy_setports ns6/policies/csk2.conf.in ns6/policies/csk2.conf
copy_setports ns6/policies/kasp-fips.conf.in ns6/policies/kasp-fips.conf
copy_setports ns6/policies/kasp.conf.in ns6/policies/kasp.conf
if ! $SHELL ../testcrypto.sh -q RSASHA1
then
	cp ns6/policies/kasp-fips.conf ns6/policies/kasp.conf
fi

# Setup zones
(
	cd ns2
	$SHELL setup.sh
)
(
	cd ns3
	$SHELL setup.sh
)
(
	cd ns4
	$SHELL setup.sh
)
(
	cd ns5
	$SHELL setup.sh
)
(
	cd ns6
	$SHELL setup.sh
)
