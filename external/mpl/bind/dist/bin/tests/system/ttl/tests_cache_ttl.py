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

import isctest


@pytest.mark.parametrize(
    "qname,rdtype,expected_ttl",
    [
        ("min-example.", "SOA", 60),
        ("min-example.", "MX", 30),
        ("max-example.", "SOA", 120),
        ("max-example.", "MX", 60),
    ],
)
def test_cache_ttl(qname, rdtype, expected_ttl):
    msg = isctest.query.create(qname, rdtype)
    response = isctest.query.udp(msg, "10.53.0.2")
    for rr in response.answer + response.authority:
        assert rr.ttl == expected_ttl
