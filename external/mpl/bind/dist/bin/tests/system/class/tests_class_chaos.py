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


import dns.opcode
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "*/*.db",
    ]
)


def test_chaos_recursion():
    msg = isctest.query.create("foo.example.", "TXT", qclass="CH")
    res = isctest.query.udp(msg, "10.53.0.1")
    isctest.check.refused(res)


def test_chaos_auth():
    msg = isctest.query.create("a.example.", "A", qclass="CH")
    res = isctest.query.udp(msg, "10.53.0.2")
    isctest.check.noerror(res)


def test_chaos_forward():
    msg = isctest.query.create("a.example.", "A", qclass="CH")
    res = isctest.query.udp(msg, "10.53.0.3")
    isctest.check.refused(res)


def test_chaos_notify():
    msg = isctest.query.create("example.", "SOA", qclass="CH", rd=False, dnssec=False)
    msg.set_opcode(dns.opcode.NOTIFY)
    msg.flags = dns.opcode.to_flags(dns.opcode.NOTIFY)
    res = isctest.query.udp(msg, "10.53.0.2")
    isctest.check.notimp(res)


def test_query_class_none():
    msg = isctest.query.create("example.", "A", qclass="NONE")
    res = isctest.query.udp(msg, "10.53.0.2")
    isctest.check.formerr(res)
