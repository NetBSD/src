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

from dns.rcode import NOERROR, NXDOMAIN, REFUSED
from pytest import mark

import isctest


@mark.parametrize(
    "qname, qtype, srcip, rcode",
    [
        ("a.root-servers.nil", "A", "10.53.0.1", NOERROR),
        ("foo.", "A", "10.53.0.1", NXDOMAIN),
        ("example.nil", "SOA", "10.53.0.2", REFUSED),
        ("example.nil", "SOA", "10.53.0.3", REFUSED),
        ("example.nil", "SOA", "10.53.0.4", NOERROR),
    ],
)
def test_allow_query_on(ns1, qname, qtype, srcip, rcode):
    msg = isctest.query.create(qname, qtype)
    res = isctest.query.udp(msg, ns1.ip, source=srcip)
    isctest.check.rcode(res, rcode)
    if qname == "example.nil" and rcode == NOERROR:
        assert res.answer
        isctest.check.aaflag(res)
