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
Regression test for GL #6170.

The NSEC3-signed zone entwild.test has an apex wildcard together with a deeper
name (en.jun2026.2048._domainkey) that forms empty non-terminals.  Querying
one of those empty non-terminals (2048._domainkey) with DNSSEC requested, while
the zone is served from the RBT zone database, used to abort named with:

    lib/dns/include/dns/name.h:1013: REQUIRE(suffixlabels <= name->labels) failed

wildcard_blocked() in lib/dns/rbt-zonedb.c did not treat the DNS_R_NEWORIGIN
result of dns_rbtnodechain_next() as a successful step when looking for the
successor of the queried name, so the empty non-terminal that blocks the
wildcard expansion was missed.  The empty non-terminal must instead be answered
with NODATA and an NSEC3 proof.

The bug only affected RBTDB; QPzone is unaffected.
"""

import dns.rdatatype
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "ns1/K*",
        "ns1/dsset-*",
        "ns1/*.signed",
        "ns1/allwild.db",
        "ns1/entwild.db",
        "ns1/example.db",
        "ns1/nestedwild.db",
        "ns1/nsec.db",
        "ns1/nsec3.db",
        "ns1/private.nsec.conf",
        "ns1/private.nsec.db",
        "ns1/private.nsec3.conf",
        "ns1/private.nsec3.db",
        "ns1/root.db",
        "ns1/signer.err",
        "ns1/trusted.conf",
    ]
)

IP_ADDR = "10.53.0.1"

# 2048._domainkey.entwild.test. and _domainkey.entwild.test. are empty
# non-terminals: they exist only because en.jun2026.2048._domainkey.entwild.test.
# and crm._domainkey.entwild.test. do.  They sit between the apex wildcard and
# the existing names.
ENT_NAME = "2048._domainkey.entwild.test."


def test_empty_nonterminal_under_wildcard_is_nodata(named_port):
    """Querying the empty non-terminal must yield NODATA, not crash named."""
    query = isctest.query.create(ENT_NAME, "A")
    response = isctest.query.tcp(query, IP_ADDR, named_port)

    isctest.check.is_response_to(response, query)
    # The empty non-terminal exists, so the wildcard must not expand: NODATA.
    isctest.check.noerror(response)
    isctest.check.empty_answer(response)
    # The NSEC3 empty-non-terminal proof is the code path that used to crash.
    assert any(
        rrset.rdtype == dns.rdatatype.NSEC3 for rrset in response.authority
    ), str(response)


def test_wildcard_still_expands_without_blocking_ent(named_port):
    """A name with no blocking empty non-terminal is still synthesized."""
    query = isctest.query.create("nonexistent.entwild.test.", "A")
    response = isctest.query.tcp(query, IP_ADDR, named_port)

    isctest.check.is_response_to(response, query)
    isctest.check.noerror(response)
    assert response.answer, str(response)
    assert response.answer[0][0].to_text() == "192.0.2.1", str(response)
