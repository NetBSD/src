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
rsafail=0 eccfail=0 ecxfail=0

$SHELL ../testcrypto.sh -q rsa || rsafail=1
$SHELL ../testcrypto.sh -q ecdsa || eccfail=1
$SHELL ../testcrypto.sh -q eddsa || ecxfail=1

if [ $rsafail = 1 -a $eccfail = 1 ]; then
	echo "I:This test requires PKCS#11 support for either RSA or ECDSA cryptography." >&2
	exit 255
fi
rm -f supported
touch supported
if [ $rsafail = 0 ]; then
	echo rsa >> supported
fi
if [ $eccfail = 0 ]; then
	echo ecc >> supported
fi
if [ $ecxfail = 0 ]; then
	echo ecx >> supported
fi
