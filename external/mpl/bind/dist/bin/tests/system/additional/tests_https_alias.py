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
import dns.name
import dns.rdatatype

import isctest


def _query(ns, qname, qtype):
    msg = isctest.query.create(qname, qtype)
    res = isctest.query.udp(msg, ns.ip)
    isctest.check.noerror(res)
    return res


def _additional(res, target):
    """
    Return the additional RRsets of 'res' keyed by type, checking that
    all of them are owned by 'target'.
    """
    owner = dns.name.from_text(target)
    assert all(rrset.name == owner for rrset in res.additional), res.additional
    return {rrset.rdtype: rrset for rrset in res.additional}


def test_https_alias_target_too_many_records(ns3):
    """
    Resolve HTTPS AliasMode records whose targets are already cached, then
    shut named down and check that nothing was leaked.

    The target of alias14 is a 14-record ServiceMode RRset, i.e. more than
    DNS_RDATASET_MAXADDITIONAL.  Following the alias clones the cached
    target RRset into the caller's rdataset, and the subsequent additional
    processing of that RRset fails with DNS_R_TOOMANYRECORDS.  That error
    used to be returned before the clone was disassociated, leaking a
    reference to the cache node and its slab for every such query.

    The target of alias13 is at the limit and is processed normally.

    The limit bounds the processing of a target RRset for its own
    additional data, not the inclusion of the target RRset itself, which
    is added to the response before it is processed.  The ServiceMode
    records point at "." (the owner name), so processing a target RRset
    adds the target's cached A record: it is present for target13 and
    absent for target14.
    """
    # Prime the cache with both target RRsets and their A records.
    res = _query(ns3, "target14.https.example.", "HTTPS")
    isctest.check.rr_count_eq(res.answer, 14)
    res = _query(ns3, "target13.https.example.", "HTTPS")
    isctest.check.rr_count_eq(res.answer, 13)
    for target in ("target14", "target13"):
        res = _query(ns3, f"{target}.https.example.", "A")
        isctest.check.rr_count_eq(res.answer, 1)

    # The first pass resolves the aliases recursively, the second one is
    # answered from the cache.
    for _ in range(2):
        # An error while collecting the additional data must not turn into
        # a failed response (RFC 9460 section 4.2).
        res = _query(ns3, "alias14.https.example.", "HTTPS")
        expected = dns.message.from_text(""";ANSWER
alias14.https.example. 86400 IN HTTPS 0 target14.https.example.
""")
        isctest.check.rrsets_equal(res.answer, expected.answer)
        # The target RRset is returned in full, but as it is over the
        # limit it is not processed, so its A record is not added.
        target14 = _additional(res, "target14.https.example.")
        assert set(target14) == {dns.rdatatype.HTTPS}, res.additional
        isctest.check.rr_count_eq([target14[dns.rdatatype.HTTPS]], 14)

        # The alias at the limit is followed, its target is included, and
        # the target is processed in turn, adding its A record.
        res = _query(ns3, "alias13.https.example.", "HTTPS")
        expected = dns.message.from_text(""";ANSWER
alias13.https.example. 86400 IN HTTPS 0 target13.https.example.
""")
        isctest.check.rrsets_equal(res.answer, expected.answer)
        target13 = _additional(res, "target13.https.example.")
        assert set(target13) == {dns.rdatatype.HTTPS, dns.rdatatype.A}, res.additional
        isctest.check.rr_count_eq([target13[dns.rdatatype.HTTPS]], 13)
        expected = dns.message.from_text(""";ADDITIONAL
target13.https.example. 86400 IN A 10.53.0.13
""")
        isctest.check.rrsets_equal([target13[dns.rdatatype.A]], expected.additional)

    # Stop the server and check for leaked references.  A leaked cache
    # rdataset pins the cache database and its memory context, which shows
    # up in named's memory tracking output at exit.
    ns3.stop()
    assert "outstanding memory" not in ns3.log
