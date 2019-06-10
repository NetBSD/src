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

# shellcheck source=conf.sh
. "$SYSTEMTESTTOP/conf.sh"

set -e

if "$PERL" -e 'use Net::DNS;' 2>/dev/null
then
    # shellcheck disable=SC2016
    if "$PERL" -e 'use Net::DNS; die if ($Net::DNS::VERSION >= 0.69 && $Net::DNS::VERSION <= 0.70);' 2>/dev/null
    then
        :
    else
        echo_i "Net::DNS versions 0.69 to 0.70 have bugs that cause this test to fail: please update." >&2
        exit 1
    fi
fi

exit 0
