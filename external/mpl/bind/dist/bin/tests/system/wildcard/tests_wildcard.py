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


"""
Example property-based test for wildcard synthesis.
Verifies that otherwise-empty zone with single wildcard record * A 192.0.2.1
produces synthesized answers for <random_label>.test. A, and returns NODATA for
<random_label>.test. when rdtype is not A.

Limitations - untested properties:
    - empty non-terminals prevent expansion
    - or more generally any existing node prevents expansion
    - DNSSEC record inclusion
    - possibly others, see RFC 4592 and company
    - content of authority & additional sections
    - flags beyond RCODE
    - special behavior of rdtypes like CNAME
"""

import pytest

pytest.importorskip("dns", minversion="2.0.0")
import dns.message
import dns.name
import dns.query
import dns.rcode
import dns.rdataclass
import dns.rdatatype
import dns.rrset

# in FIPs mode md5 fails so we need 4.41.2 or later which does not use md5
try:
    import hashlib

    hashlib.md5(b"1234")
    pytest.importorskip("hypothesis")
except ValueError:
    pytest.importorskip("hypothesis", minversion="4.41.2")
from hypothesis import assume, example, given

from isctest.hypothesis.strategies import dns_names, dns_rdatatypes_without_meta
import isctest.check
import isctest.name
import isctest.query


# labels of a zone with * A 192.0.2.1 wildcard
SUFFIX = dns.name.from_text("allwild.test.")
WILDCARD_RDTYPE = dns.rdatatype.A
WILDCARD_RDATA = "192.0.2.1"
IP_ADDR = "10.53.0.1"
TIMEOUT = 5  # seconds, just a sanity check


@given(name=dns_names(suffix=SUFFIX), rdtype=dns_rdatatypes_without_meta)
def test_wildcard_rdtype_mismatch(
    name: dns.name.Name, rdtype: dns.rdatatype.RdataType, named_port: int
) -> None:
    """Any label non-matching rdtype must result in NODATA."""
    assume(rdtype != WILDCARD_RDTYPE)

    # NS and SOA are present in the zone and DS gets answered from parent.
    assume(
        not (
            name == SUFFIX
            and rdtype in (dns.rdatatype.SOA, dns.rdatatype.NS, dns.rdatatype.DS)
        )
    )

    # Subdomains of *.allwild.test. are not to be synthesized.
    # See RFC 4592 section 2.2.1.
    assume(name == SUFFIX or name.labels[-len(SUFFIX) - 1] != b"*")

    query_msg = dns.message.make_query(name, rdtype)
    response_msg = isctest.query.tcp(query_msg, IP_ADDR, named_port, timeout=TIMEOUT)

    isctest.check.is_response_to(response_msg, query_msg)
    isctest.check.noerror(response_msg)
    isctest.check.empty_answer(response_msg)


@given(name=dns_names(suffix=SUFFIX, min_labels=len(SUFFIX) + 1))
def test_wildcard_match(name: dns.name.Name, named_port: int) -> None:
    """Any label with maching rdtype must result in wildcard data in answer."""

    # Subdomains of *.allwild.test. are not to be synthesized.
    # See RFC 4592 section 2.2.1.
    assume(name.labels[-len(SUFFIX) - 1] != b"*")

    query_msg = dns.message.make_query(name, WILDCARD_RDTYPE)
    response_msg = isctest.query.tcp(query_msg, IP_ADDR, named_port, timeout=TIMEOUT)

    isctest.check.is_response_to(response_msg, query_msg)
    isctest.check.noerror(response_msg)
    expected_answer = [
        dns.rrset.from_text(
            query_msg.question[0].name,
            300,  # TTL, ignored by dnspython comparison
            dns.rdataclass.IN,
            WILDCARD_RDTYPE,
            WILDCARD_RDATA,
        )
    ]
    assert response_msg.answer == expected_answer, str(response_msg)


# Force the `*.*.allwild.test.` corner case to be checked.
@example(name=isctest.name.prepend_label("*", isctest.name.prepend_label("*", SUFFIX)))
@given(
    name=dns_names(
        suffix=isctest.name.prepend_label("*", SUFFIX), min_labels=len(SUFFIX) + 2
    )
)
def test_wildcard_with_star_not_synthesized(
    name: dns.name.Name, named_port: int
) -> None:
    """RFC 4592 section 2.2.1 ghost.*.example."""
    query_msg = dns.message.make_query(name, WILDCARD_RDTYPE)
    response_msg = isctest.query.tcp(query_msg, IP_ADDR, named_port, timeout=TIMEOUT)

    isctest.check.is_response_to(response_msg, query_msg)
    isctest.check.nxdomain(response_msg)
    isctest.check.empty_answer(query_msg)


NESTED_SUFFIX = dns.name.from_text("*.*.nestedwild.test.")


# Force `*.*.*.nestedwild.test.` to be checked.
@example(name=isctest.name.prepend_label("*", NESTED_SUFFIX))
@given(name=dns_names(suffix=NESTED_SUFFIX, min_labels=len(NESTED_SUFFIX) + 1))
def test_name_in_between_wildcards(name: dns.name.Name, named_port: int) -> None:
    """Check nested wildcard cases.

    There are `*.nestedwild.test. A` and `*.*.*.nestedwild.test. A` records present in their zone.
    This means that `foo.*.nestedwild.test. A` must not be synthetized (see test above)
    but `foo.*.*.nestedwild.test A` must.
    """

    # `*.*.*.nestedwild.test.` and `*.foo.*.*.nestedwild.test.` must be NOERROR
    # `foo.*.*.*.nestedwild.test` must be NXDOMAIN (see test below).
    assume(
        len(name) == len(NESTED_SUFFIX) + 1
        or name.labels[-len(NESTED_SUFFIX) - 1] != b"*"
    )

    query_msg = dns.message.make_query(name, WILDCARD_RDTYPE)
    response_msg = isctest.query.tcp(query_msg, IP_ADDR, named_port, timeout=TIMEOUT)

    isctest.check.is_response_to(response_msg, query_msg)
    isctest.check.noerror(response_msg)
    expected_answer = [
        dns.rrset.from_text(
            query_msg.question[0].name,
            300,  # TTL, ignored by dnspython comparison
            dns.rdataclass.IN,
            WILDCARD_RDTYPE,
            WILDCARD_RDATA,
        )
    ]
    assert response_msg.answer == expected_answer, str(response_msg)


@given(
    name=dns_names(
        suffix=isctest.name.prepend_label("*", NESTED_SUFFIX),
        min_labels=len(NESTED_SUFFIX) + 2,
    )
)
def test_name_nested_wildcard_subdomains_not_synthesized(
    name: dns.name.Name, named_port: int
):
    """Check nested wildcard cases.

    `foo.*.*.*.nestedwild.test. A` must not be synthesized.
    """
    query_msg = dns.message.make_query(name, WILDCARD_RDTYPE)
    response_msg = isctest.query.tcp(query_msg, IP_ADDR, named_port, timeout=TIMEOUT)

    isctest.check.is_response_to(response_msg, query_msg)
    isctest.check.nxdomain(response_msg)
    isctest.check.empty_answer(query_msg)
