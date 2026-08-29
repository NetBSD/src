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
With "stale-answer-client-timeout 0" a negative cache entry which is still
within its TTL must be answered straight from the cache.  Only a stale entry
may trigger a refresh of the RRset.
"""

import time

import dns.message
import dns.rcode
import pytest

from isctest.instance import AnsInstance, NamedInstance

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "ans*/ans.run",
        "ns*/named.run",
        "ns*/root.bk",
    ]
)

# ans2 answers both of these from a SOA with a 600 second TTL and a 600
# second MINIMUM, so the negative answer stays fresh for the whole test.
NODATA_NAME = "longttl-nodata.example."
NXDOMAIN_NAME = "longttl-nxdomain.example."


def upstream_queries(ans2: AnsInstance, qname: str) -> int:
    """Number of TXT queries for `qname` which reached the authoritative server."""
    return len(ans2.log.grep(f"request: {qname.rstrip('.')}/TXT"))


def enable_ans2_responses(ans2: AnsInstance) -> None:
    isctest.query.udp(dns.message.make_query("enable.", "TXT"), ans2.ip)


@pytest.fixture(name="resolver")
def resolver_fixture(servers: dict[str, NamedInstance]) -> NamedInstance:
    ns3 = servers["ns3"]
    enable_ans2_responses(servers["ans2"])
    ns3.rndc("serve-stale on")
    ns3.rndc("flush")
    return ns3


@pytest.mark.parametrize(
    "qname,expected_rcode",
    [
        pytest.param(NODATA_NAME, dns.rcode.NOERROR, id="nodata"),
        pytest.param(NXDOMAIN_NAME, dns.rcode.NXDOMAIN, id="nxdomain"),
    ],
)
def test_fresh_ncache_entry_is_not_refreshed(
    servers: dict[str, NamedInstance],
    resolver: NamedInstance,
    qname: str,
    expected_rcode: dns.rcode.Rcode,
) -> None:
    ans2 = servers["ans2"]
    msg = dns.message.make_query(qname, "TXT")

    # Prime the cache; this is the one query the authoritative server is
    # allowed to see.
    res = isctest.query.udp(msg, resolver.ip)
    isctest.check.rcode(res, expected_rcode)
    assert not res.answer

    primed = upstream_queries(ans2, qname)
    assert primed == 1, "priming the cache should send exactly one upstream query"

    # Repeat the query.  The negative answer is nowhere near its expiry, so
    # every one of these has to be a cache hit, and none of them may mark the
    # answer as stale.
    for _ in range(3):
        res = isctest.query.udp(msg, resolver.ip)
        isctest.check.rcode(res, expected_rcode)
        isctest.check.noede(res)

    # Refreshing stale data is detached from the client query, so give it a
    # moment to attempt to reach ans2 before concluding it never happened.
    time.sleep(1)

    assert (
        upstream_queries(ans2, qname) == primed
    ), "a negative cache entry within its TTL was refreshed upstream"

    prohibited_log = (
        f"{qname.rstrip('.')} TXT stale answer used, "
        "an attempt to refresh the RRset will still be made"
    )
    assert prohibited_log not in resolver.log
