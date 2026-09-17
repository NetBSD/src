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
Tests for the delv tool.
"""

from re import compile as Re

import re

import pytest

from digdelv.common import ARTIFACTS, check_ttl_range, parse_yaml
from isctest.util import param

import isctest
import isctest.mark

pytestmark = [
    pytest.mark.extra_artifacts(ARTIFACTS),
]


@pytest.fixture(name="delv")
def delv_fixture(named_port):
    # use delv insecure mode by default, as we're mostly not testing dnssec
    return isctest.run.EnvCmd("DELV", f"+noroot -p {named_port}")


@pytest.mark.parametrize("option", ["+short", "+sh"])
def test_short(delv, ns3, option):
    """Check that delv +short (and its +sh abbreviation) returns a
    single-line answer."""
    result = delv(f"@{ns3.ip} {option} a a.example")
    assert len(result.out.splitlines()) == 1


@pytest.mark.parametrize("option", ["+split=4", "+sp=4"])
def test_split_width(delv, ns3, option):
    """Check that delv +split (and its +sp abbreviation) splits hex data
    into fields of the requested width."""
    result = delv(f"@{ns3.ip} {option} -t sshfp foo.example")
    assert " 9ABC DEF6 7890 " in result.out
    assert check_ttl_range(result.out, "SSHFP", 300)


def test_unknownformat(delv, ns3):
    """Check that delv +unknownformat prints RFC 3597 format."""
    result = delv(f"@{ns3.ip} +unknownformat a a.example")
    assert Re(r"CLASS1\s+TYPE1\s+\\# 4 0A000001") in result.out
    assert check_ttl_range(result.out, "TYPE1", 300)


def test_4_and_6_mutually_exclusive(delv, ns3):
    """Check that delv rejects -4 combined with -6."""
    result = delv(f"@{ns3.ip} -4 -6 A a.example", raise_on_exception=False)
    assert result.rc != 0
    assert "only one of -4 and -6 allowed" in result.err


def test_malformed_query_name(delv, ns3):
    """Check that delv exits cleanly on a malformed query name instead of
    aborting in the dns_client_detach(NULL) cleanup path."""
    longlabel = "a" * 64
    result = delv(f"@{ns3.ip} -t a {longlabel}.example.com", raise_on_exception=False)
    assert result.rc >= 0
    assert "label too long" in result.err


@isctest.mark.with_ipv6
@pytest.mark.parametrize(
    "server_args,message",
    [
        param(
            "@fd92:7065:b8e:ffff::3 @{ns3} -6",
            "Use of IPv4 disabled by -6",
            id="ipv4-server-with-6",
        ),
        param(
            "@{ns3} @fd92:7065:b8e:ffff::3 -4",
            "Use of IPv6 disabled by -4",
            id="ipv6-server-with-4",
        ),
    ],
)
def test_address_family_mismatch(delv, ns3, server_args, message):
    """Check that the last @server option overrides earlier ones and that
    the forced address family makes such a lookup fail."""
    result = delv(
        server_args.format(ns3=ns3.ip) + " -t txt foo.example",
        raise_on_exception=False,
    )
    assert result.rc != 0
    # it should have no results but error output
    assert "testing" not in result.out
    assert message in result.err


def test_reverse_lookup(delv, ns3):
    """Check that delv -x works."""
    result = delv(f"@{ns3.ip} -x 127.0.0.1")
    # doesn't matter if has answer
    assert Re(r"127\.in-addr\.arpa\.", re.IGNORECASE) in result.out
    assert check_ttl_range(result.out, r"\-ANY", 10800)


def test_tcp(delv, ns3):
    """Check that delv over TCP works."""
    result = delv(f"+tcp @{ns3.ip} a a.example")
    assert Re(r"10\.0\.0\.1$") in result.out
    assert check_ttl_range(result.out, "A", 300)


@pytest.mark.parametrize(
    "args,expect_rrcomment,ttl_rrtype",
    [
        param(
            "+multi +norrcomments DNSKEY example",
            False,
            "DNSKEY",
            id="multi-norrcomments-dnskey",
        ),
        param(
            "+multi +norrcomments SOA example",
            False,
            "SOA",
            id="multi-norrcomments-soa",
        ),
        param("+rrcomments DNSKEY example", True, "DNSKEY", id="rrcomments"),
        param("+short +rrcomments DNSKEY example", True, None, id="short-rrcomments"),
    ],
)
def test_rrcomments(delv, ns3, zsk, args, expect_rrcomment, ttl_rrtype):
    """Check that +[no]rrcomments controls the DNSKEY comment
    (the default is rrcomments, even with +multi)."""
    result = delv(f"+tcp @{ns3.ip} {args}")
    assert (zsk.rrcomment in result.out) == expect_rrcomment
    if ttl_rrtype:
        assert check_ttl_range(result.out, ttl_rrtype, 300)


def test_short_rrcomments_line(delv, ns3, zsk):
    """Check the exact delv +short +rrcomments output line."""
    result = delv(f"+tcp @{ns3.ip} +short +rrcomments DNSKEY example")
    assert f"{zsk.keydata}  {zsk.rrcomment}" in result.out


def test_short_nosplit(delv, ns3, zsk):
    """Check that delv +short +nosplit does not split the key data."""
    result = delv(f"+tcp @{ns3.ip} +short +nosplit DNSKEY example")
    assert zsk.keydata.replace(" ", "") in result.out
    assert len(result.out.splitlines()) == 1
    assert len(result.out.split()) == 14


def test_short_nosplit_norrcomments(delv, ns3, zsk):
    """Check that delv +short +nosplit +norrcomments prints the bare
    unsplit rdata."""
    result = delv(f"+tcp @{ns3.ip} +short +nosplit +norrcomments DNSKEY example")
    nosplit = zsk.keydata.replace(" ", "")
    assert Re(re.escape(nosplit) + "$") in result.out
    assert len(result.out.splitlines()) == 1
    assert len(result.out.split()) == 4


@pytest.mark.parametrize(
    "qclass",
    [
        param("IN", id="in"),
        param("CH", id="ch-ignored"),
    ],
)
def test_class_option(delv, ns3, qclass):
    """Check that delv -c IN works and that -c CH is ignored and treated
    like IN."""
    result = delv(f"@{ns3.ip} -c {qclass} -t a a.example")
    assert "a.example." in result.out
    assert check_ttl_range(result.out, "A", 300)


def test_q_m(delv, ns3):
    """Check that -q -m treats -m as a query name, not as the memory
    debugging flag."""
    result = delv(f"@{ns3.ip} -q -m")
    assert Re(r"^; -m\..*\d*.*IN.*ANY.*;") in result.out
    for stream in (result.out, result.err):
        assert Re(r"^add ") not in stream
        assert Re(r"^del ") not in stream
    assert check_ttl_range(result.out, r"\-ANY", 300)


def test_any_query(delv, ns3):
    """Check that delv -t ANY works."""
    result = delv(f"@{ns3.ip} -t ANY example")
    assert Re(r"^example\.") in result.out
    assert check_ttl_range(result.out, "NS", 300)
    assert check_ttl_range(result.out, "SOA", 300)


@pytest.mark.parametrize(
    "anchor",
    [
        param("anchor.dnskey", id="key-style"),
        param("anchor.ds", id="ds-style"),
    ],
)
def test_trust_anchors(delv, ns3, anchor):
    """Check that delv loads key-style and DS-style trust anchors and
    validates with them."""
    result = delv(f"-a ns3/{anchor} +root=example @{ns3.ip} -t DNSKEY example")
    assert "fully validated" in result.out


def test_refused_chasing_ds(delv, ns2):
    """Check that delv handles REFUSED when chasing DS records."""
    result = delv(f"@{ns2.ip} +root xxx.example.tld A")
    assert ";; resolution failed: broken trust chain" in result.err


def test_yaml_any(delv, ns3):
    """Check the structure of delv +yaml output."""
    result = delv(f"+yaml @{ns3.ip} any ns2.example")
    data = parse_yaml(result.out)
    assert data["status"] == "success"
    assert data["query_name"] == "ns2.example"
    answer = data["records"][0]["answer_not_validated"][0]
    assert len(str(answer).split()) == 5


@pytest.mark.parametrize(
    "qtype,qname,status",
    [
        param("type500", "ns2.example", "ncache nxrrset", id="nodata"),
        param("a", "this-does-not-exist.ns2.example", "ncache nxdomain", id="nxdomain"),
    ],
)
def test_yaml_negative(delv, ns3, qtype, qname, status):
    """Check the structure of delv +yaml output for negative responses."""
    result = delv(f"+yaml @{ns3.ip} {qtype} {qname}")
    data = parse_yaml(result.out)
    assert data["status"] == status
    assert data["query_name"] == qname
    answer = data["records"][0]["negative_response_answer_not_validated"][0]
    assert len(str(answer).split()) == 5


@pytest.mark.usefixtures("ns1")
def test_ns_output(delv):
    """Check the NS records in delv +ns output."""
    result = delv(
        "-i +ns +nortrace +nostrace +nomtrace +novtrace +hint=root.hint ns example"
    )
    ns_lines = [
        fields
        for fields in (line.split() for line in result.out.splitlines())
        if len(fields) >= 4 and fields[0] == "example." and fields[3] == "NS"
    ]
    assert len(ns_lines) == 2


@pytest.mark.parametrize(
    "args,marker,expect_no_qmin_labels",
    [
        param(
            "-i +ns +hint=root.hint",
            "; authoritative",
            True,
            id="no-validation",
        ),
        param(
            "-i +ns +qmin +hint=root.hint",
            "; authoritative",
            False,
            id="no-validation-qmin",
        ),
        param(
            "-a ns1/anchor.dnskey +root +ns +hint=root.hint",
            "; fully validated",
            True,
            id="validation",
        ),
        param(
            "-a ns1/anchor.dnskey +root +ns +qmin +hint=root.hint",
            "; fully validated",
            False,
            id="validation-qmin",
        ),
    ],
)
@pytest.mark.usefixtures("ns1")
def test_ns_lookup(delv, args, marker, expect_no_qmin_labels):
    """Check delv +ns lookups with and without validation and query name
    minimization."""
    result = delv(f"{args} a a.example")
    assert marker in result.out
    if expect_no_qmin_labels:
        assert "_.example" not in result.out


@isctest.mark.with_ipv6
@pytest.mark.parametrize("family", ["-4", "-6"])
@pytest.mark.usefixtures("ns1")
def test_ns_address_family(delv, family):
    """Check that delv +ns with -4/-6 uses only the selected address
    family."""
    ipv4_packet = "sending packet to 10.53"
    ipv6_packet = "sending packet to fd92:7065"
    result = delv(
        f"-a ns1/anchor.dnskey +root {family} +ns +hint=root.hint a a.example"
    )
    assert (ipv4_packet in result.out) == (family == "-4")
    assert (ipv6_packet in result.out) == (family == "-6")
