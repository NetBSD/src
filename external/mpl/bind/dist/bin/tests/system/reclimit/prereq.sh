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

if $PERL -e 'use Net::DNS;' 2>/dev/null
then
    if $PERL -e 'use Net::DNS; die if ($Net::DNS::VERSION <= 0.78);' 2>/dev/null
    then
        :
    else
        echo_i "Net::DNS versions up to 0.78 have a bug that causes this test to fail: please update." >&2
        exit 1
    fi
else
    echo_i "This test requires the Net::DNS library." >&2
    exit 1
fi

if $PERL -e 'use Net::DNS::Nameserver;' 2>/dev/null
then
    :
else
    echo_i "This test requires the Net::DNS::Nameserver library." >&2
    exit 1
fi
