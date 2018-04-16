#!/bin/sh
#
# Copyright (C) 2010, 2012, 2014, 2017  Internet Systems Consortium, Inc. ("ISC")
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

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
