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

import isctest


# This tests ensures the server does not crash during this query.
# See issue B#6407
def test_rpz_6407(ns11):
    msg = isctest.query.create("1.0.0.127.in-addr.arpa.", "PTR")
    res = isctest.query.tcp(msg, ns11.ip)
    isctest.check.noerror(res)
