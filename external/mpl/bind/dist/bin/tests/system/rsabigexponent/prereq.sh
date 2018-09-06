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

test -r $RANDFILE || $GENRANDOM 800 $RANDFILE

if $BIGKEY > /dev/null 2>&1
then
    rm -f Kexample.*
else
    echo_i "This test requires cryptography" >&2
    echo_i "configure with --with-openssl, or --with-pkcs11 and --enable-native-pkcs11" >&2
    exit 255
fi
