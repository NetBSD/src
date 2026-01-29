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
import isctest

# isctest.asyncserver requires dnspython >= 2.0.0
pytest.importorskip("dns", minversion="2.0.0")

import dns.message

pytestmark = pytest.mark.extra_artifacts(
    [
        "ans*/ans.run",
    ]
)


def test_connreset():
    msg = dns.message.make_query(
        "sub.example.", "A", want_dnssec=True, use_edns=0, payload=1232
    )
    res = isctest.query.udp(msg, "10.53.0.2")
    isctest.check.servfail(res)
