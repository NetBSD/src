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


@pytest.mark.parametrize(
    "qname,rdtype,expected_ttl",
    [
        ("min-example.", "SOA", 60),
        ("min-example.", "MX", 30),
        ("max-example.", "SOA", 120),
        ("max-example.", "MX", 60),
    ],
)
def test_cache_ttl(qname, rdtype, expected_ttl, named_port):
    msg = dns.message.make_query(qname, rdtype)
    response = dns.query.udp(msg, "10.53.0.2", timeout=10, port=named_port)
    for rr in response.answer + response.authority:
        assert rr.ttl == expected_ttl
