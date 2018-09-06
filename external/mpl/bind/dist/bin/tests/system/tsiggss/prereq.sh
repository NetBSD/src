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

# enable the tsiggss test only if gssapi was enabled
$FEATURETEST --gssapi ||  {
        echo_i "gssapi and krb5 not supported - skipping tsiggss test"
        exit 255
}

# ... and crypto
exec $SHELL ../testcrypto.sh
