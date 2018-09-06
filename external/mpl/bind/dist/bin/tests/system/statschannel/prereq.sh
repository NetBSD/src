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

fail=0

if $PERL -e 'use File::Fetch;' 2>/dev/null
then
        :
else
    echo_i "This test requires the File::Fetch library." >&2
    fail=1
fi

exit $fail
