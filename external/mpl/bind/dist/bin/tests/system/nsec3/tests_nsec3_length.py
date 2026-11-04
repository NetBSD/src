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

# pylint: disable=redefined-outer-name,unused-import

import dns.message

import isctest

ZONES = {
    "evil.test",
}


def bootstrap():
    return {
        "zones": ZONES,
    }


def test_nsec3_invalid_length():
    msg = dns.message.make_query("xxx.evil.test", "A")
    res = isctest.query.udp(msg, "10.53.0.5")
    isctest.check.servfail(res)
