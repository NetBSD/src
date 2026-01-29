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

import os

import pytest

pytest.importorskip("dns", minversion="2.0.0")
import dns.rcode
import dns.rrset

import isctest
from isctest.compat import dns_rcode


pytestmark = pytest.mark.extra_artifacts(
    [
        "ns3/*-rpz-external.local.db",
        "ns3/rpz*.txt",
    ]
)


@pytest.mark.parametrize(
    "qname,source,rcode",
    [
        # For 10.53.0.1 source IP:
        # - baddomain.com isn't allowed (CNAME .), should return NXDOMAIN
        # - gooddomain.com is allowed
        # - allowed. is allowed
        ("baddomain.", "10.53.0.1", dns.rcode.NXDOMAIN),
        ("gooddomain.", "10.53.0.1", dns.rcode.NOERROR),
        ("allowed.", "10.53.0.1", dns.rcode.NOERROR),
        # For 10.53.0.2 source IP:
        # - allowed.com isn't allowed (CNAME .), should return NXDOMAIN
        # - baddomain.com is allowed
        # - gooddomain.com is allowed
        ("baddomain.", "10.53.0.2", dns.rcode.NOERROR),
        ("gooddomain.", "10.53.0.2", dns.rcode.NOERROR),
        ("allowed.", "10.53.0.2", dns.rcode.NXDOMAIN),
        # For 10.53.0.3 source IP:
        # - gooddomain.com is allowed
        # - baddomain.com is allowed
        # - allowed. is allowed
        ("baddomain.", "10.53.0.3", dns.rcode.NOERROR),
        ("gooddomain.", "10.53.0.3", dns.rcode.NOERROR),
        ("allowed.", "10.53.0.3", dns.rcode.NOERROR),
        # For 10.53.0.4 source IP:
        # - gooddomain.com isn't allowed (CNAME .), should return NXDOMAIN
        # - baddomain.com isn't allowed (CNAME .), should return NXDOMAIN
        # - allowed. is allowed
        ("baddomain.", "10.53.0.4", dns.rcode.NXDOMAIN),
        ("gooddomain.", "10.53.0.4", dns.rcode.NXDOMAIN),
        ("allowed.", "10.53.0.4", dns.rcode.NOERROR),
        # For 10.53.0.5 (any) source IP:
        # - baddomain.com is allowed
        # - gooddomain.com isn't allowed (CNAME .), should return NXDOMAIN
        # - allowed.com isn't allowed (CNAME .), should return NXDOMAIN
        ("baddomain.", "10.53.0.5", dns.rcode.NOERROR),
        ("gooddomain.", "10.53.0.5", dns.rcode.NXDOMAIN),
        ("allowed.", "10.53.0.5", dns.rcode.NXDOMAIN),
    ],
)
def test_rpz_multiple_views(qname, source, rcode):
    # Wait for the rpz-external.local zone transfer
    msg = isctest.query.create("rpz-external.local", "SOA")
    isctest.query.tcp(
        msg,
        ip="10.53.0.3",
        source="10.53.0.2",
        expected_rcode=dns_rcode.NOERROR,
    )
    isctest.query.tcp(
        msg,
        ip="10.53.0.3",
        source="10.53.0.5",
        expected_rcode=dns_rcode.NOERROR,
    )

    msg = isctest.query.create(qname, "A")
    res = isctest.query.udp(msg, "10.53.0.3", source=source, expected_rcode=rcode)
    if rcode == dns.rcode.NOERROR:
        assert res.answer == [dns.rrset.from_text(qname, 300, "IN", "A", "10.53.0.2")]


def test_rpz_passthru_logging():
    resolver_ip = "10.53.0.3"

    # Should generate a log entry into rpz_passthru.txt
    msg_allowed = isctest.query.create("allowed.", "A")
    res_allowed = isctest.query.udp(
        msg_allowed, resolver_ip, source="10.53.0.1", expected_rcode=dns.rcode.NOERROR
    )
    assert res_allowed.answer == [
        dns.rrset.from_text("allowed.", 300, "IN", "A", "10.53.0.2")
    ]

    # Should also generate a log entry into rpz_passthru.txt
    msg_allowed_any = isctest.query.create("allowed.", "ANY")
    res_allowed_any = isctest.query.udp(
        msg_allowed_any,
        resolver_ip,
        source="10.53.0.1",
        expected_rcode=dns.rcode.NOERROR,
    )
    assert res_allowed_any.answer == [
        dns.rrset.from_text("allowed.", 300, "IN", "NS", "ns1.allowed."),
        dns.rrset.from_text("allowed.", 300, "IN", "A", "10.53.0.2"),
    ]
    # The comparison above doesn't compare the TTL values, and we want to
    # make sure that the "passthru" rpz doesn't cap the TTL with max-policy-ttl.
    assert res_allowed_any.answer[0].ttl > 200
    assert res_allowed_any.answer[1].ttl > 200

    # baddomain.com isn't allowed (CNAME .), should return NXDOMAIN
    # Should generate a log entry into rpz.txt
    msg_not_allowed = isctest.query.create("baddomain.", "A")
    res_not_allowed = isctest.query.udp(
        msg_not_allowed,
        resolver_ip,
        source="10.53.0.1",
        expected_rcode=dns.rcode.NXDOMAIN,
    )
    isctest.check.nxdomain(res_not_allowed)

    rpz_passthru_logfile = os.path.join("ns3", "rpz_passthru.txt")
    rpz_logfile = os.path.join("ns3", "rpz.txt")

    assert os.path.isfile(rpz_passthru_logfile)
    assert os.path.isfile(rpz_logfile)

    with open(rpz_passthru_logfile, encoding="utf-8") as log_file:
        line = log_file.read()
        assert "rpz QNAME PASSTHRU rewrite allowed/A/IN" in line

    with open(rpz_logfile, encoding="utf-8") as log_file:
        line = log_file.read()
        assert "rpz QNAME PASSTHRU rewrite allowed/A/IN" not in line
        assert "rpz QNAME NXDOMAIN rewrite baddomain/A/IN" in line
