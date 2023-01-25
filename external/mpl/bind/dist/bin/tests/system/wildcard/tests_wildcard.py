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
    - expansion works with multiple labels
    - asterisk in qname does not cause expansion
    - empty non-terminals prevent expansion
    - or more generally any existing node prevents expansion
    - DNSSEC record inclusion
    - possibly others, see RFC 4592 and company
    - content of authority & additional sections
    - flags beyond RCODE
    - special behavior of rdtypes like CNAME
"""
import pytest

pytest.importorskip("dns")
import dns.message
import dns.name
import dns.query
import dns.rcode
import dns.rdataclass
import dns.rdatatype
import dns.rrset

pytest.importorskip("hypothesis")
from hypothesis import given
from hypothesis.strategies import binary, integers


# labels of a zone with * A 192.0.2.1 wildcard
WILDCARD_ZONE = ("allwild", "test", "")
WILDCARD_RDTYPE = dns.rdatatype.A
WILDCARD_RDATA = "192.0.2.1"
IPADDR = "10.53.0.1"
TIMEOUT = 5  # seconds, just a sanity check


# Helpers
def is_nonexpanding_rdtype(rdtype):
    """skip meta types to avoid weird rcodes caused by AXFR etc.; RFC 6895"""
    return not (
        rdtype == WILDCARD_RDTYPE
        or dns.rdatatype.is_metatype(rdtype)  # known metatypes: OPT ...
        or 128 <= rdtype <= 255
    )  # unknown meta types


def tcp_query(where, port, qname, qtype):
    querymsg = dns.message.make_query(qname, qtype)
    assert len(querymsg.question) == 1
    return querymsg, dns.query.tcp(querymsg, where, port=port, timeout=TIMEOUT)


def query(where, port, label, rdtype):
    labels = (label,) + WILDCARD_ZONE
    qname = dns.name.Name(labels)
    return tcp_query(where, port, qname, rdtype)


# Tests
@given(
    label=binary(min_size=1, max_size=63),
    rdtype=integers(min_value=0, max_value=65535).filter(is_nonexpanding_rdtype),
)
def test_wildcard_rdtype_mismatch(label, rdtype, named_port):
    """any label non-matching rdtype must result in to NODATA"""
    check_answer_nodata(*query(IPADDR, named_port, label, rdtype))


def check_answer_nodata(querymsg, answer):
    assert querymsg.is_response(answer), str(answer)
    assert answer.rcode() == dns.rcode.NOERROR, str(answer)
    assert answer.answer == [], str(answer)


@given(label=binary(min_size=1, max_size=63))
def test_wildcard_match(label, named_port):
    """any label with maching rdtype must result in wildcard data in answer"""
    check_answer_noerror(*query(IPADDR, named_port, label, WILDCARD_RDTYPE))


def check_answer_noerror(querymsg, answer):
    assert querymsg.is_response(answer), str(answer)
    assert answer.rcode() == dns.rcode.NOERROR, str(answer)
    assert len(querymsg.question) == 1, str(answer)
    expected_answer = [
        dns.rrset.from_text(
            querymsg.question[0].name,
            300,  # TTL, ignored by dnspython comparison
            dns.rdataclass.IN,
            WILDCARD_RDTYPE,
            WILDCARD_RDATA,
        )
    ]
    assert answer.answer == expected_answer, str(answer)
