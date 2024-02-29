#!/usr/bin/python3

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

import pytest

pytest.importorskip("dns")
import dns.message
import dns.query
import dns.rcode


def test_connreset(named_port):
    msg = dns.message.make_query(
        "sub.example.", "A", want_dnssec=True, use_edns=0, payload=1232
    )
    ans = dns.query.udp(msg, "10.53.0.2", timeout=10, port=named_port)
    assert ans.rcode() == dns.rcode.SERVFAIL
