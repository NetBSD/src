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

import dns.rcode

import isctest


def attempt_query(ns):
    ns.rndc("flush")
    msg = isctest.query.create("foo.example.", "A")
    res = isctest.query.udp(msg, ns.ip)
    if msg.rcode() == dns.rcode.NOERROR:
        return len(res.answer) == 1
    return False


def test_randomizens(ns6):
    resolved = False
    for _ in range(1, 25):
        if attempt_query(ns6):
            resolved = True
            break
    assert resolved
