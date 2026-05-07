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

import itertools

import dns.flags
import dns.rrset
import pytest

import isctest


@pytest.mark.parametrize(
    "name,limit",
    [
        ("1000", 1000),
        ("2000", 2000),
        ("3000", 3000),
        ("4000", 4000),
        ("a-maximum-rrset", 4091),
    ],
)
def test_limits(name, limit):
    msg_query = isctest.query.create(f"{name}.example.", "A")
    res = isctest.query.tcp(msg_query, "10.53.0.1", log_response=False)

    iplist = [
        f"10.0.{x}.{y}"
        for x, y in itertools.islice(itertools.product(range(256), repeat=2), limit)
    ]

    msg_rrset = [dns.rrset.from_text_list(f"{name}.example.", "5M", "IN", "A", iplist)]

    assert res.answer == msg_rrset


def test_limit_exceeded():
    msg_query = isctest.query.create("5000.example.", "A")
    res = isctest.query.tcp(msg_query, "10.53.0.1", log_response=False)

    assert res.flags & dns.flags.TC, "TC flag was not set"
