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

import os

import isctest
import pytest

import dns.message


@pytest.mark.parametrize(
    "qname",
    [
        "zone1.com.",  # glob include of zone1 config
        "zone2.com.",  # glob include of zone2 config
        "mars.com.",  # checking include of standard file path config
    ],
)
def test_include_multiplecfg(qname):
    msg = dns.message.make_query(qname, "A")
    res = isctest.query.tcp(msg, "10.53.0.2")

    isctest.check.noerror(res)

    assert res.answer[0] == dns.rrset.from_text(qname, 86400, "IN", "A", "10.53.0.1")


def test_include_multiplecfg_checkconf():
    """Test that named-checkconf correctly parses glob includes"""
    isctest.run.cmd([os.environ["CHECKCONF"], "named.conf"], cwd="ns2")
