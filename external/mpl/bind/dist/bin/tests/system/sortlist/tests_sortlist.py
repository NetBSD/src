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

import dns.message

import pytest

import isctest


def test_sortlist():
    """Test two-element sortlist statement"""
    msg = dns.message.make_query("a.example.", "A")
    resp = isctest.query.tcp(msg, "10.53.0.1", source="10.53.0.1")
    sortlist = [
        "192.168.3.1",
        "192.168.1.1",
        "1.1.1.5",
        "1.1.1.1",
        "1.1.1.3",
        "1.1.1.2",
        "1.1.1.4",
    ]
    rrset = dns.rrset.from_text_list("a.example.", 300, "IN", "A", sortlist)
    assert len(resp.answer) == 1
    assert resp.answer[0] == rrset
    assert list(resp.answer[0].items) == list(rrset.items)


@pytest.mark.parametrize(
    "source_ip,possible_results",
    [
        ("10.53.0.2", ["10.53.0.2", "10.53.0.3"]),
        ("10.53.0.3", ["10.53.0.2", "10.53.0.3"]),
        ("10.53.0.4", ["10.53.0.4"]),
        ("10.53.0.5", ["10.53.0.5"]),
    ],
)
def test_sortlist_compat(possible_results, source_ip):
    """Test one-element sortlist statement and undocumented BIND 8 features"""
    msg = dns.message.make_query("b.example.", "A")
    resp = isctest.query.tcp(msg, "10.53.0.1", source=source_ip)
    assert (
        resp.answer[0][0].to_text() in possible_results
    ), f"{possible_results} not found"
