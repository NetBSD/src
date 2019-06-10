#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

echo "I:(Native PKCS#11)" >&2
ecxfail=0

$SHELL ../testcrypto.sh -q eddsa || ecxfail=1

rm -f supported
touch supported
echo rsa >> supported
echo ecc >> supported
if [ $ecxfail = 0 ]; then
	echo ecx >> supported
fi
