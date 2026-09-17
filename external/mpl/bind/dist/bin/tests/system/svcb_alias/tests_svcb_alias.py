# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

import dns.rcode
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "ns2/tree.db",
    ]
)

# Highest node index that owns an HTTPS RRset in ns2/tree.db (setup.sh).  Its
# children are empty leaves, so the tree served from cache is three levels
# deep: n0 (root), n1..n13, n14..n182.
PARENTS = 182

# DNS_RDATASET_MAXADDITIONAL from lib/dns/include/dns/rdataset.h: the most
# records one RRset contributes to a single level of additional processing.
MAXADDITIONAL = 13


def _additional_rrs(res):
    return sum(len(rrset) for rrset in res.additional)


def _query_tcp(ns, index):
    msg = isctest.query.create(f"n{index}.tree.example.", "HTTPS")
    # Query over TCP so the additional section is not capped by the UDP buffer.
    res = isctest.query.tcp(msg, ns.ip)
    isctest.check.noerror(res)
    return res


def test_svcb_alias_tree_additional_is_bounded(ns1):
    """
    A cached tree of HTTPS AliasMode records must not let a single query walk
    the whole tree while building the additional section.

    ns2 serves a 13-way HTTPS AliasMode tree.  Priming the resolver cache
    top-down (ascending index, so every node's children are still cache misses
    while it is being cached) plants the tree without any single response
    walking it.  A query for the root then follows every cached descendant:
    the per-RRset limit (DNS_RDATASET_MAXADDITIONAL) and the per-path depth
    limit (max-restarts) each bound one path, but neither bounds the aggregate
    number of records visited across the sibling paths.
    """
    # Prime the cache with every node that owns an HTTPS RRset.
    for i in range(PARENTS + 1):
        res = isctest.query.udp(
            isctest.query.create(f"n{i}.tree.example.", "HTTPS"),
            ns1.ip,
            expected_rcode=dns.rcode.NOERROR,
        )
        isctest.check.noerror(res)

    # Root of the full three-level tree.
    root = _query_tcp(ns1, 0)
    # A node one level down: its grandchildren are empty leaves, so following
    # it touches only a single level of the tree.  This is roughly the amount
    # of additional data a bounded resolver should also produce for the root.
    shallow = _query_tcp(ns1, 1)

    isctest.log.info(
        "additional records: root=%d (names=%d) shallow=%d (names=%d)",
        _additional_rrs(root),
        len(root.additional),
        _additional_rrs(shallow),
        len(shallow.additional),
    )

    # Without an aggregate limit, the root walk visits the whole planted tree
    # and its additional section grows with the tree.  With the limit in place
    # the walk is cut to roughly one level, so the root must not carry more
    # than about one extra RRset's worth of additional names beyond a node
    # whose own subtree is a single level.  This pins the behaviour without
    # depending on the limit's exact value.
    assert len(root.additional) <= len(shallow.additional) + MAXADDITIONAL
