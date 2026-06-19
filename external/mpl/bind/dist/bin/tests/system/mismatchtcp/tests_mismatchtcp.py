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
End-to-end check for the immediate UDP-to-TCP fallback on a query-id
mismatch.

The fake authoritative server at 10.53.0.2 answers every UDP query for
trigger.example./A with a response whose DNS message id has been flipped.
The resolver at 10.53.0.1 must escalate to TCP on the first such response
and return the correct A record that the fake server serves over TCP.
"""

from pathlib import Path

import dns.message
import dns.rdatatype
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "ans*/ans.run",
        "ns*/named.stats*",
    ]
)


MISMATCH_LABEL = "mismatch responses received"
MISMATCHTCP_LABEL = "queries retried over TCP after a response with mismatched query id"


def _named_stats(ns1) -> str:
    stats_path = Path(ns1.directory) / "named.stats"
    if stats_path.exists():
        stats_path.unlink()
    ns1.rndc("stats")
    return stats_path.read_text(encoding="utf-8")


def _counter(stats: str, label: str) -> int:
    for line in stats.splitlines():
        line = line.strip()
        if line.endswith(label):
            return int(line.split()[0])
    return 0


def test_mismatch_tcp_fallback(ns1):
    """
    Issue a single recursive query for a name whose UDP responses are
    being spoofed.  The resolver must escalate to TCP on the first
    near-miss and return the correct A record.
    """
    msg = dns.message.make_query("trigger.example.", dns.rdatatype.A, want_dnssec=False)
    res = isctest.query.udp(msg, ns1.ip, timeout=10)
    isctest.check.noerror(res)

    answers = [rrset for rrset in res.answer if rrset.rdtype == dns.rdatatype.A]
    assert answers, f"no A RRset in response: {res}"
    addresses = {item.address for rrset in answers for item in rrset}
    assert "192.0.2.42" in addresses, f"unexpected answer: {addresses}"


def test_mismatch_counter(ns1):
    """
    After the spoofed exchange completes the resolver's existing
    "mismatch responses received" counter must be non-zero, confirming
    the dispatcher actually saw the wrong-id response, and the new
    "queries retried over TCP after a response with mismatched query
    id" counter must also be non-zero, confirming that the TCP
    fallback path actually fired in response to that mismatch.
    """
    msg = dns.message.make_query("trigger.example.", dns.rdatatype.A, want_dnssec=False)
    isctest.query.udp(msg, ns1.ip, timeout=10)

    stats = _named_stats(ns1)
    assert _counter(stats, MISMATCH_LABEL) > 0, stats
    assert _counter(stats, MISMATCHTCP_LABEL) > 0, stats
