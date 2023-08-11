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

$SHELL clean.sh

copy_setports ns1/named.conf.in ns1/named.conf

if $FEATURETEST --md5
then
	cat >> ns1/named.conf << EOF
# Conditionally included when support for MD5 is available
key "md5" {
        secret "97rnFx24Tfna4mHPfgnerA==";
        algorithm hmac-md5;
};

key "md5-trunc" {
        secret "97rnFx24Tfna4mHPfgnerA==";
        algorithm hmac-md5-80;
};
EOF
fi
