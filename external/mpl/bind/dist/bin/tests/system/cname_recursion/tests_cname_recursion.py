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

"""
An RD=1 query to a server which is authoritative for the queried zone,
where the answer is a CNAME pointing to a name outside of that zone, must
recurse to resolve the CNAME target and return the complete chain.

Regression test: when a zone shares the view's "allow-query" ACL object
(zones added at runtime via "rndc addzone" or as catalog zone members do;
zones configured in named.conf get their own copy), evaluating the ACL
during the zone lookup used to clear all other client query attributes
(NS_QUERYATTR_RECURSIONOK and NS_QUERYATTR_CACHEOK among them), so the
restarted query for the CNAME target could not recurse and the response
contained only the CNAME record without the target's address record.
"""

import dns.name
import dns.rdataclass
import dns.rdatatype
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "ns2/_default.nz*",
        "ns2/catalog.db*",
    ]
)


def assert_full_chain(res, alias, target, address):
    isctest.check.noerror(res)

    cname = res.get_rrset(
        res.answer,
        dns.name.from_text(alias),
        dns.rdataclass.IN,
        dns.rdatatype.CNAME,
    )
    assert cname is not None, "CNAME missing from the answer section"

    answer = res.get_rrset(
        res.answer,
        dns.name.from_text(target),
        dns.rdataclass.IN,
        dns.rdatatype.A,
    )
    assert answer is not None, "CNAME target not resolved (partial response)"
    assert str(answer[0]) == address


@pytest.mark.requires_zones_loaded("ns1", "ns2")
def test_cname_recursion_static_zone(ns2):
    msg = isctest.query.create("alias.internal.", "A")
    res = isctest.query.udp(msg, ns2.ip)
    assert_full_chain(res, "alias.internal.", "target.external.", "10.0.0.99")


@pytest.mark.requires_zones_loaded("ns1", "ns2")
def test_cname_recursion_added_zone(ns2):
    with ns2.watch_log_from_here() as watcher:
        ns2.rndc('addzone added { type primary; file "added.db"; };')
        watcher.wait_for_line("zone added/IN: loaded serial 1")

    msg = isctest.query.create("alias.added.", "A")
    res = isctest.query.udp(msg, ns2.ip)
    assert_full_chain(res, "alias.added.", "target2.external.", "10.0.0.98")


@pytest.mark.requires_zones_loaded("ns1", "ns2")
def test_cname_recursion_catz_member_zone(ns2):
    with ns2.watch_log_from_start() as watcher:
        watcher.wait_for_line("zone member/IN: transferred serial 1")

    msg = isctest.query.create("alias.member.", "A")
    res = isctest.query.udp(msg, ns2.ip)
    assert_full_chain(res, "alias.member.", "target3.external.", "10.0.0.97")
