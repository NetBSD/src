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
Tests for the dig tool.
"""

from re import compile as Re

import ipaddress
import os
import re

import pytest

from digdelv.common import ARTIFACTS, check_ttl_range, parse_yaml
from isctest.util import param

import isctest
import isctest.mark

pytestmark = [
    pytest.mark.extra_artifacts(ARTIFACTS),
]


@pytest.fixture(name="dig")
def dig_fixture(named_port):
    return isctest.run.EnvCmd("DIG", f"-p {named_port}")


def edns_yaml(text, direction="query"):
    """Get the EDNS OPT pseudosection mapping of the first message in
    dig +yaml output."""
    message = parse_yaml(text)[0]["message"]
    return message[f"{direction}_message_data"]["OPT_PSEUDOSECTION"]["EDNS"]


def test_update_response(dig, ans6):
    """Check that dig rejects a response with the UPDATE opcode."""
    result = dig(
        f"@{ans6.ip} +tries=1 +timeout=1 cname foo.bar", raise_on_exception=False
    )
    assert result.rc != 0
    assert "Opcode mismatch" in result.out


def test_short(dig, ns3):
    """Check that dig +short returns a single-line answer."""
    result = dig(f"@{ns3.ip} +short a a.example")
    assert len(result.out.splitlines()) == 1


@pytest.mark.parametrize("option", ["+split=4", "+sp=4"])
def test_split_width(dig, ns3, option):
    """Check that dig +split (and its +sp abbreviation) splits hex data
    into fields of the requested width."""
    result = dig(f"@{ns3.ip} {option} -t sshfp foo.example")
    assert " 9ABC DEF6 7890 " in result.out
    assert check_ttl_range(result.out, "SSHFP", 300)


def test_unknownformat(dig, ns3):
    """Check that dig +unknownformat prints RFC 3597 format."""
    result = dig(f"@{ns3.ip} +unknownformat a a.example")
    assert Re(r"CLASS1\s+TYPE1\s+\\# 4 0A000001") in result.out
    assert check_ttl_range(result.out, "TYPE1", 300)


def test_reverse_lookup(dig, ns3):
    """Check that dig -x works."""
    result = dig(f"@{ns3.ip} -x 127.0.0.1")
    # doesn't matter if has answer
    assert Re(r"127\.in-addr\.arpa\.", re.IGNORECASE) in result.out
    assert check_ttl_range(result.out, "SOA", 86400)


def test_tcp(dig, ns3):
    """Check that dig over TCP works."""
    result = dig(f"+tcp @{ns3.ip} a a.example")
    assert Re(r"10\.0\.0\.1$") in result.out
    assert check_ttl_range(result.out, "A", 300)


@pytest.mark.parametrize(
    "args,expect_rrcomment",
    [
        param("+multi +norrcomments -t DNSKEY example", False, id="multi-norrcomments"),
        param("+rrcomments DNSKEY example", True, id="rrcomments"),
        param("+short +rrcomments DNSKEY example", True, id="short-rrcomments"),
    ],
)
def test_dnskey_rrcomments(dig, ns3, zsk, args, expect_rrcomment):
    """Check that +[no]rrcomments controls the DNSKEY comment
    (the default is rrcomments, even with +multi)."""
    result = dig(f"+tcp @{ns3.ip} {args}")
    assert (zsk.rrcomment in result.out) == expect_rrcomment
    if "+short" not in args:
        assert check_ttl_range(result.out, "DNSKEY", 300)


def test_soa_norrcomments(dig, ns3):
    """Check that +multi +norrcomments suppresses the SOA field comments."""
    result = dig(f"+tcp @{ns3.ip} +multi +norrcomments -t SOA example")
    assert "; serial" not in result.out
    assert check_ttl_range(result.out, "SOA", 300)


def test_short_nosplit(dig, ns3, zsk):
    """Check that dig +short +nosplit does not split the key data."""
    result = dig(f"+tcp @{ns3.ip} +short +nosplit DNSKEY example")
    assert zsk.keydata.replace(" ", "") in result.out


def test_short_rrcomments_line(dig, ns3, zsk):
    """Check the exact dig +short +rrcomments output line."""
    result = dig(f"+tcp @{ns3.ip} +short +rrcomments DNSKEY example")
    expected = re.escape(f"{zsk.keydata}  {zsk.rrcomment}")
    assert Re(expected + "$") in result.out


def test_multi_flag_is_local(dig, ns3):
    """Check that +[no]multi applies to a single lookup only."""
    lines = {}
    for flags in [
        ("nomulti", "nomulti"),
        ("multi", "nomulti"),
        ("nomulti", "multi"),
        ("multi", "multi"),
    ]:
        first, second = flags
        result = dig(f"+tcp @{ns3.ip} -t DNSKEY example +{first} example +{second}")
        assert check_ttl_range(result.out, "DNSKEY", 300)
        lines[flags] = len(result.out.splitlines())
    assert lines[("multi", "multi")] >= lines[("nomulti", "multi")]
    assert lines[("multi", "multi")] >= lines[("multi", "nomulti")]
    assert lines[("nomulti", "multi")] >= lines[("nomulti", "nomulti")]
    assert lines[("multi", "nomulti")] >= lines[("nomulti", "nomulti")]


def test_noheader_only(dig, ns3):
    """Check that dig +noheader-only sends a full query."""
    result = dig(f"+tcp @{ns3.ip} +noheader-only A example")
    assert "Got answer:" in result.out
    assert check_ttl_range(result.out, "SOA", 300)


@pytest.mark.parametrize(
    "class_type",
    [
        param("", id="default"),
        param("-c IN -t A", id="with-class-and-type"),
    ],
)
def test_header_only(dig, ns3, class_type):
    """Check that dig +header-only sends a query without a question."""
    result = dig(f"+tcp @{ns3.ip} +header-only {class_type} example")
    assert Re(r"^;; flags: qr rd; QUERY: 0, ANSWER: 0,") in result.out
    assert Re(r"^;; QUESTION SECTION:") not in result.out


@pytest.mark.parametrize(
    "qname,ttl",
    [
        param("weeks", "3w"),
        param("days", "3d"),
        param("hours", "3h"),
        param("minutes", "45m"),
        param("seconds", "45s"),
    ],
)
def test_ttl_units(dig, ns2, qname, ttl):
    """Check that dig +ttlunits prints TTLs in time units."""
    result = dig(f"+tcp @{ns2.ip} +ttlunits A {qname}.example")
    assert Re(rf"^{qname}\.example\.\s+{ttl}\s") in result.out


@pytest.mark.parametrize(
    "options,field",
    [
        param("+ttlunits +nottlid", "IN", id="nottlid-wins"),
        param("+nottlid +ttlunits", "3w", id="ttlunits-wins"),
        param("+nottlid +nottlunits", "1814400", id="plain-seconds"),
    ],
)
def test_ttl_units_precedence(dig, ns2, options, field):
    """Check that the last of the +ttlid/+ttlunits options wins."""
    result = dig(f"+tcp @{ns2.ip} {options} A weeks.example")
    assert Re(rf"^weeks\.example\.\s+{re.escape(field)}\s") in result.out


def test_class_chaos(dig, ns3):
    """Check that dig -c CHAOS works."""
    result = dig(f"@{ns3.ip} -c CHAOS -t txt version.bind")
    assert "version.bind.\t\t0\tCH\tTXT" in result.out


def test_bad_escape(dig, ns3):
    """Check that dig gracefully rejects a bad escape in the domain name."""
    result = dig(rf"@{ns3.ip} \0.", raise_on_exception=False)
    assert result.rc == 10
    assert "REQUIRE" not in result.err
    assert "is not a legal name (bad escape)" in result.err


def test_q_m(dig, ns3):
    """Check that -q -m treats -m as a query name, not as the memory
    debugging flag."""
    result = dig(f"@{ns3.ip} -q -m", raise_on_exception=False)
    assert Re(r"^;-m\..*IN.*A$") in result.out
    assert "Dump of all outstanding memory allocations" not in result.out


@pytest.mark.parametrize(
    "options,pattern",
    [
        param(
            "+expandaaaa",
            r"ns2\.example.*fd92:7065:0b8e:ffff:0000:0000:0000:0002",
            id="expandaaaa",
        ),
        param(
            "+noexpandaaaa", r"ns2\.example.*fd92:7065:b8e:ffff::2", id="noexpandaaaa"
        ),
        param("", r"ns2\.example.*fd92:7065:b8e:ffff::2", id="default"),
        param(
            "+short +expandaaaa",
            r"^fd92:7065:0b8e:ffff:0000:0000:0000:0002$",
            id="short-expandaaaa",
        ),
    ],
)
def test_expandaaaa(dig, ns3, options, pattern):
    """Check that +[no]expandaaaa controls AAAA address formatting
    (the default is +noexpandaaaa)."""
    result = dig(f"@{ns3.ip} {options} AAAA ns2.example")
    assert Re(pattern) in result.out


def test_bufsize_zero(dig, ns3):
    """Check that +bufsize=0 just sets the advertised buffer size to 0
    instead of disabling EDNS."""
    result = dig(f"@{ns3.ip} a.example +bufsize=0 +qr")
    assert "EDNS:" in result.out


def test_bufsize_restores_default(dig, ns3):
    """Check that a later +bufsize restores the default buffer size."""
    result = dig(f"@{ns3.ip} a.example +bufsize=0 +bufsize +qr")
    assert len(result.out.grep(Re(r"EDNS:.* udp:"))) == 2
    assert len(result.out.grep(Re(r"EDNS:.* udp: 1232"))) == 2


@pytest.mark.parametrize(
    "options,unit",
    [
        param("", "msec", id="msec"),
        param("-u", "usec", id="usec"),
    ],
)
def test_query_time_units(dig, ns3, options, unit):
    """Check that Query time is in milliseconds, or in microseconds
    with -u."""
    result = dig(f"{options} @{ns3.ip} a.example")
    assert Re(rf";; Query time: \d+ {unit}") in result.out


@pytest.mark.parametrize(
    "options,digits",
    [
        param("+yaml", 3, id="msec"),
        param("-u +yaml", 6, id="usec"),
    ],
)
def test_yaml_timestamp_precision(dig, ns3, options, digits):
    """Check that +yaml timestamps have millisecond precision, or
    microsecond precision with -u."""
    result = dig(f"{options} @{ns3.ip} a.example")
    for field in ("query_time", "response_time"):
        pattern = (
            rf"{field}: !!timestamp \d{{4}}-\d\d-\d\dT\d\d:\d\d:\d\d\.\d{{{digits}}}Z"
        )
        assert Re(pattern) in result.out


def test_local_reserved_warning(dig, ns3):
    """Check that dig warns about .local queries."""
    result = dig(f"@{ns3.ip} local soa")
    assert ";; WARNING: .local is reserved for Multicast DNS" in result.out


def test_nocrypto(dig, ns1):
    """Check that +nocrypto omits the key and signature data."""
    alg_num = os.environ["DEFAULT_ALGORITHM_NUMBER"]
    result = dig(f"+dnssec +norec +nocrypto DNSKEY . @{ns1.ip}")
    assert Re(rf"256 \d+ {alg_num} \[key id = [1-9]\d*]") in result.out
    assert Re(r"RRSIG.* \[omitted]") in result.out
    result = dig(f"+norec +nocrypto DS example @{ns1.ip}")
    assert Re(r"DS.* \d+ [12] \[omitted]") in result.out


def test_coflag(dig, ns3):
    """Check that dig +coflag sets the EDNS CO flag in the sent query."""
    result = dig(f"+tcp @{ns3.ip} +coflag +qr example")
    assert Re(r"^; EDNS: version: 0, flags: co;") in result.out
    assert check_ttl_range(result.out, "SOA", 300)


def test_coflag_yaml(dig, ns3):
    """Check that dig +coflag +yaml shows the CO flag in the sent query."""
    result = dig(f"+yaml +tcp @{ns3.ip} +coflag +qr example")
    assert edns_yaml(result.out)["flags"] == "co"


@pytest.mark.parametrize(
    "option,sent_flags",
    [
        param("+raflag", "rd ra ad"),
        param("+tcflag", "tc rd ad"),
    ],
)
def test_header_flag_options(dig, ns3, option, sent_flags):
    """Check that +raflag/+tcflag set the flag in the sent query and that
    the response is unaffected."""
    result = dig(f"+tcp @{ns3.ip} {option} +qr example")
    assert Re(rf"^;; flags: {sent_flags}; QUERY: 1, ANSWER: 0") in result.out
    assert Re(r"^;; flags: qr rd ra; QUERY: 1, ANSWER: 0,") in result.out
    assert check_ttl_range(result.out, "SOA", 300)


def test_zflag(dig, ns3):
    """Check that dig +zflag sets the MBZ bit and that named ignores it."""
    result = dig(f"+tcp @{ns3.ip} +zflag +qr A example")
    assert Re(r"^;; flags: rd ad; MBZ: 0x4;") in result.out
    assert Re(r"^;; flags: qr rd ra; QUERY: 1") in result.out
    assert check_ttl_range(result.out, "SOA", 300)


def test_ednsopt_08_no_insist(dig, ns3):
    """Check that +qr +ednsopt=08 does not cause an INSIST failure."""
    result = dig(f"@{ns3.ip} +ednsopt=08 +qr a a.example")
    assert "INSIST" not in result.out
    assert "FORMERR" in result.out


@pytest.mark.parametrize(
    "option",
    [
        param("3", id="number"),
        param("nsid", id="name"),
    ],
)
def test_ednsopt_nsid(dig, ns3, option):
    """Check that +ednsopt accepts an option number as well as a name."""
    result = dig(f"@{ns3.ip} +ednsopt={option} a.example")
    assert Re(r'NSID: .* \("ns3"\)') in result.out
    assert check_ttl_range(result.out, "A", 300)


def test_ednsopt_update_lease(dig, ns3):
    """Check that a single-lease UPDATE-LEASE option prints as expected."""
    result = dig(f"@{ns3.ip} +ednsopt=UPDATE-LEASE:00000e10 +qr a.example")
    assert "UPDATE-LEASE: 3600 (1 hour)" in result.out


def test_ednsopt_update_lease_yaml(dig, ns3):
    """Check that a single-lease UPDATE-LEASE option prints as expected
    with +yaml."""
    result = dig(f"@{ns3.ip} +yaml +ednsopt=UPDATE-LEASE:00000e10 +qr a.example")
    assert edns_yaml(result.out)["UPDATE-LEASE"]["LEASE"] == 3600
    assert "LEASE: 3600 # 1 hour" in result.out


def test_ednsopt_update_lease_split(dig, ns3):
    """Check that a split-lease UPDATE-LEASE option prints as expected."""
    result = dig(f"@{ns3.ip} +ednsopt=UPDATE-LEASE:00000e1000127500 +qr a.example")
    assert "UPDATE-LEASE: 3600/1209600 (1 hour/2 weeks)" in result.out


def test_ednsopt_update_lease_split_yaml(dig, ns3):
    """Check that a split-lease UPDATE-LEASE option prints as expected
    with +yaml."""
    result = dig(
        f"@{ns3.ip} +yaml +ednsopt=UPDATE-LEASE:00000e1000127500 +qr a.example"
    )
    update_lease = edns_yaml(result.out)["UPDATE-LEASE"]
    assert update_lease["LEASE"] == 3600
    assert update_lease["KEY-LEASE"] == 1209600
    assert "LEASE: 3600 # 1 hour" in result.out
    assert "KEY-LEASE: 1209600 # 2 weeks" in result.out


def test_ednsopt_llq(dig, ns3):
    """Check that the LLQ option prints as expected."""
    result = dig(
        f"@{ns3.ip} +ednsopt=llq:0001000200001234567812345678fefefefe +qr a.example"
    )
    pattern = (
        r"LLQ: Version: 1, Opcode: 2, Error: 0, "
        r"Identifier: 1311768465173141112, Lifetime: 4278124286$"
    )
    assert Re(pattern) in result.out


def test_ednsopt_llq_yaml(dig, ns3):
    """Check that the LLQ option prints as expected with +yaml."""
    result = dig(
        f"@{ns3.ip} +yaml +ednsopt=llq:0001000200001234567812345678fefefefe "
        "+qr a.example"
    )
    llq = edns_yaml(result.out)["LLQ"]
    assert llq["LLQ-VERSION"] == 1
    assert llq["LLQ-OPCODE"] == 2
    assert llq["LLQ-ERROR"] == 0
    assert llq["LLQ-ID"] == 1311768465173141112
    assert llq["LLQ-LEASE"] == 4278124286


def test_ednsopt_key_tag_empty(dig, ns3):
    """Check that an empty key-tag option is sent and FORMERR is returned."""
    result = dig(f"@{ns3.ip} +ednsopt=key-tag a.example +qr")
    assert Re(r"; KEY-TAG: *$") in result.out
    assert "status: FORMERR" in result.out


def test_ednsopt_key_tag(dig, ns3):
    """Check that a key-tag value list is sent and accepted."""
    result = dig(f"@{ns3.ip} +ednsopt=key-tag:00010002 a.example +qr")
    assert Re(r"; KEY-TAG: 1, 2$") in result.out
    assert "status: FORMERR" not in result.out
    assert check_ttl_range(result.out, "A", 300)


def test_ednsopt_key_tag_yaml(dig, ns3):
    """Check that a key-tag value list prints as a list with +yaml."""
    result = dig(f"@{ns3.ip} +yaml +ednsopt=key-tag:00010002 a.example +qr")
    assert edns_yaml(result.out)["KEY-TAG"] == [1, 2]


def test_ednsopt_key_tag_malformed(dig, ns3):
    """Check that a malformed key-tag value list is sent as raw data and
    FORMERR is returned."""
    result = dig(f"@{ns3.ip} +ednsopt=key-tag:0001000201 a.example +qr")
    assert "; KEY-TAG: 00 01 00 02 01" in result.out
    assert "status: FORMERR" in result.out


@pytest.mark.parametrize("tag", ["client-tag", "server-tag"])
def test_ednsopt_tag(dig, ns3, tag):
    """Check that a valid client/server-tag value is sent and accepted."""
    result = dig(f"@{ns3.ip} +ednsopt={tag}:0001 a.example +qr")
    assert Re(rf"; {tag.upper()}: 1$") in result.out
    assert "status: FORMERR" not in result.out


@pytest.mark.parametrize("tag", ["client-tag", "server-tag"])
def test_ednsopt_tag_yaml(dig, ns3, tag):
    """Check that a client/server-tag value prints as expected with +yaml."""
    result = dig(f"@{ns3.ip} +yaml +ednsopt={tag}:0001 a.example +qr")
    assert edns_yaml(result.out)[tag.upper()] == 1


@pytest.mark.parametrize(
    "value",
    [
        param("01", id="too-short"),
        param("000001", id="too-long"),
    ],
)
@pytest.mark.parametrize("tag", ["client-tag", "server-tag"])
def test_ednsopt_tag_bad_length(dig, ns3, tag, value):
    """Check that FORMERR is returned for a client/server-tag value of the
    wrong length."""
    result = dig(f"@{ns3.ip} +ednsopt={tag}:{value} a.example +qr")
    assert f"; {tag.upper()}" in result.out
    assert "status: FORMERR" in result.out


def test_ednsopt_chain(dig, ns3):
    """Check that the CHAIN option prints special characters escaped."""
    result = dig(rf'@{ns3.ip} +ednsopt=chain:02002200 a.\000" +qr')
    assert r'; CHAIN: "\000\""' in result.out


def test_ednsopt_chain_yaml(dig, ns3):
    """Check that the CHAIN option prints special characters escaped
    with +yaml."""
    result = dig(rf'@{ns3.ip} +yaml +ednsopt=chain:02002200 a.\000" +qr')
    assert edns_yaml(result.out)["CHAIN"] == r"\000\""


def test_expire(dig, ns1):
    """Check that dig processes +expire."""
    result = dig(f"@{ns1.ip} +expire . soa")
    assert "; EXPIRE: 1200 (20 minutes)" in result.out


def test_expire_yaml(dig, ns1):
    """Check that dig processes +expire with +yaml."""
    result = dig(f"@{ns1.ip} +yaml +expire . soa")
    assert edns_yaml(result.out, "response")["EXPIRE"] == 1200
    assert "EXPIRE: 1200 # 20 minutes" in result.out


def test_keepalive(dig, ns1):
    """Check that dig processes +keepalive."""
    result = dig(f"@{ns1.ip} +keepalive . soa +tcp")
    assert "; TCP-KEEPALIVE: 30.0 secs" in result.out


def test_keepalive_yaml(dig, ns1):
    """Check that dig processes +keepalive with +yaml."""
    result = dig(f"@{ns1.ip} +yaml +keepalive . soa +tcp")
    assert edns_yaml(result.out, "response")["TCP-KEEPALIVE"] == "30.0 secs"


@pytest.mark.parametrize(
    "payload,expected",
    [
        param("ede:0000666f6f", "; EDE: 0 (Other): (foo)", id="first-defined-code"),
        param("ede:0018", "; EDE: 24 (Invalid Data)", id="last-defined-code"),
        param("ede:0019666f6f", "; EDE: 25: (foo)", id="undefined-code"),
        param("ede", "; EDE:", id="empty"),
        param("ede:00", '; EDE: 00 (".")', id="too-short"),
    ],
)
def test_ednsopt_ede(dig, ns3, payload, expected):
    """Check that Extended DNS Error options, including invalid ones with
    a too short payload, are printed correctly."""
    result = dig(f"@{ns3.ip} +ednsopt={payload} a.example +qr")
    assert Re("^" + re.escape(expected) + "$") in result.out


@pytest.mark.parametrize(
    "payload,info_code,extra_text",
    [
        param("ede:0000666f6f", "0 (Other)", "foo", id="first-defined-code"),
        param("ede:0018", "24 (Invalid Data)", None, id="last-defined-code"),
        param("ede:0019666f6f", 25, "foo", id="undefined-code"),
    ],
)
def test_ednsopt_ede_yaml(dig, ns3, payload, info_code, extra_text):
    """Check that Extended DNS Error options are printed correctly
    with +yaml."""
    result = dig(f"@{ns3.ip} +yaml +ednsopt={payload} a.example +qr")
    ede = edns_yaml(result.out)["EDE"]
    assert ede["INFO-CODE"] == info_code
    if extra_text is None:
        assert "EXTRA-TEXT" not in ede
    else:
        assert ede["EXTRA-TEXT"] == extra_text


def test_ednsopt_ede_yaml_specials(dig, ns3):
    """Check that EDE extra text with '"' and '\\' specials survives YAML
    quoting."""
    result = dig(f"@{ns3.ip} +yaml +ednsopt=ede:0000666f6f225c a.example +qr")
    assert edns_yaml(result.out)["EDE"]["EXTRA-TEXT"] == 'foo"\\'


@pytest.mark.parametrize(
    "payload,expected",
    [
        param("ede", None, id="empty"),
        param("ede:00", '00 (".")', id="too-short"),
    ],
)
def test_ednsopt_ede_yaml_invalid(dig, ns3, payload, expected):
    """Check that invalid Extended DNS Error options with a too short
    payload are printed correctly with +yaml."""
    result = dig(f"@{ns3.ip} +yaml +ednsopt={payload} a.example +qr")
    assert edns_yaml(result.out)["EDE"] == expected


def test_ednsopt_malformed(dig, ns3):
    """Check that dig handles the malformed option '+ednsopt=:'
    gracefully."""
    result = dig(f"@{ns3.ip} +ednsopt=: a.example", raise_on_exception=False)
    assert result.rc != 0
    assert "ednsopt no code point specified" in result.err


def test_ednsflags_reenables_edns(dig, ns3):
    """Check that +noedns +ednsflags=<nonzero> re-enables EDNS."""
    result = dig(f"@{ns3.ip} +qr +noedns +ednsflags=0x70 a.example")
    assert "; EDNS: version: 0, flags:; MBZ: 0x0070, udp: 1232" in result.out
    assert "; EDNS: version: 0, flags:; udp: 1232" in result.out


def test_showbadvers(dig, ns3):
    """Check that +showbadvers displays the BADVERS response as well as
    the retry without EDNS version 1."""
    result = dig(f"@{ns3.ip} +edns=1 +qr +showbadvers a.example")
    assert "; EDNS: version: 1, flags:; udp: 1232" in result.out
    assert "; EDNS: version: 0, flags:; udp: 1232" in result.out
    assert "status: BADVERS" in result.out
    assert "status: NOERROR" in result.out


def test_subnet(dig, ns2):
    """Check that dig +subnet sends the client subnet."""
    result = dig(f"+tcp @{ns2.ip} +subnet=127.0.0.1 A a.example")
    assert "CLIENT-SUBNET: 127.0.0.1/32/0" in result.out
    assert check_ttl_range(result.out, "A", 300)


def test_subnet_last_wins(dig, ns2):
    """Check that the last of multiple +subnet options wins."""
    result = dig(f"+tcp @{ns2.ip} +subnet=127.0.0.0 +subnet=127.0.0.1 A a.example")
    assert "CLIENT-SUBNET: 127.0.0.1/32/0" in result.out
    assert check_ttl_range(result.out, "A", 300)


@pytest.mark.parametrize("plen", range(1, 25))
def test_subnet_prefix_lengths(dig, ns2, plen):
    """Check that dig +subnet masks the address to various prefix
    lengths."""
    result = dig(f"+tcp @{ns2.ip} +subnet=255.255.255.255/{plen} A a.example")
    addr = ipaddress.ip_address((0xFFFFFFFF << (32 - plen)) & 0xFFFFFFFF)
    assert "FORMERR" not in result.out
    assert f"CLIENT-SUBNET: {addr}/{plen}/0" in result.out
    assert check_ttl_range(result.out, "A", 300)


@pytest.mark.parametrize("plen", range(9, 16))
def test_subnet_prefix_between_byte_boundaries(dig, ns2, plen):
    """Check dig +subnet with prefix lengths between byte boundaries."""
    result = dig(f"+tcp @{ns2.ip} +subnet=10.53/{plen} A a.example")
    assert "FORMERR" not in result.out
    assert Re(rf"CLIENT-SUBNET.*/{plen}/0") in result.out
    assert check_ttl_range(result.out, "A", 300)


ZERO_SUBNETS = [
    param("+subnet=0/0", "0.0.0.0/0/0"),
    param("+subnet=0", "0.0.0.0/0/0"),
    param("+subnet=::/0", "::/0/0"),
]


@pytest.mark.parametrize("option,subnet", ZERO_SUBNETS)
def test_subnet_zero(dig, ns2, option, subnet):
    """Check that a zero-length client subnet is sent and answered."""
    result = dig(f"+tcp @{ns2.ip} {option} A a.example")
    assert "status: NOERROR" in result.out
    assert f"CLIENT-SUBNET: {subnet}" in result.out
    assert "10.0.0.1" in result.out
    assert check_ttl_range(result.out, "A", 300)


@pytest.mark.parametrize("option,subnet", ZERO_SUBNETS)
def test_subnet_zero_yaml(dig, ns2, option, subnet):
    """Check that a zero-length client subnet is echoed in the +yaml
    response."""
    result = dig(f"+yaml +tcp @{ns2.ip} {option} A a.example")
    assert edns_yaml(result.out, "response")["CLIENT-SUBNET"] == subnet


def test_subnet_yaml(dig, ns2):
    """Check that +subnet=dead::/16 is shown in the +yaml query."""
    result = dig(f"+yaml +tcp @{ns2.ip} +qr +subnet=dead::/16 A a.example")
    assert edns_yaml(result.out)["CLIENT-SUBNET"] == "dead::/16/0"


def test_subnet_raw_zero(dig, ns2):
    """Check that a raw zero-length ECS option (family 0, source 0,
    scope 0) is rejected by the server with FORMERR."""
    result = dig(f"+tcp @{ns2.ip} +ednsopt=8:00000000 A a.example")
    assert "status: FORMERR" in result.out
    assert "CLIENT-SUBNET" not in result.out


def test_subnet_raw_unknown_family(dig, ns2):
    """Check that a raw ECS option with an unknown family (3) is sent
    as-is and rejected by the server with FORMERR."""
    result = dig(f"+qr +tcp @{ns2.ip} +ednsopt=8:00030000 A a.example")
    assert "status: FORMERR" in result.out
    assert len(result.out.grep("CLIENT-SUBNET: 00 03 00 00")) == 1


def test_origin_preserved_on_tcp_retries(dig, ans4):
    """Check that dig preserves the search origin when retrying over
    TCP."""
    result = dig(
        f"-d +tcp @{ans4.ip} +retry=1 +time=1 +domain=bar foo",
        raise_on_exception=False,
    )
    assert result.rc != 0
    assert len(result.err.grep("trying origin bar")) == 2
    assert "using root origin" not in result.err


def test_4_and_6_mutually_exclusive(dig, ns2):
    """Check that dig rejects -4 combined with -6."""
    result = dig(f"+tcp @{ns2.ip} -4 -6 A a.example", raise_on_exception=False)
    assert result.rc != 0
    assert "only one of -4 and -6 allowed" in result.err


@isctest.mark.with_ipv6
def test_ipv6_server_with_ipv4_only(dig):
    """Check that dig -4 rejects an IPv6 server address."""
    result = dig("+tcp @fd92:7065:b8e:ffff::2 -4 A a.example", raise_on_exception=False)
    assert result.rc != 0
    assert "address family not supported" in result.err


@isctest.mark.with_ipv6
@pytest.mark.parametrize("option", ["+tcp", "+notcp"])
def test_ipv4_server_with_ipv6_only(dig, ns2, option):
    """Check that dig -6 does not use a mapped form of an IPv4 server
    address."""
    result = dig(f"{option} @{ns2.ip} -6 A a.example")
    assert f"SERVER: ::ffff:{ns2.ip}#" not in result.out


@pytest.fixture(name="set_response_sequence")
def set_response_sequence_fixture(dig, ans5):
    """Arm the sequence of AXFR responses served by ans5."""

    def _set(sequence):
        dig(f"@{ans5.ip} {sequence}.response-sequence._control TXT")

    return _set


@pytest.mark.parametrize(
    "sequence,tries,expect_failure,eof_errors",
    [
        param("no-response", 2, True, 2, id="immediate-immediate"),
        param("partial-axfr", 2, True, 2, id="partial-partial"),
        param("no-response.partial-axfr", 2, True, 2, id="immediate-partial"),
        param("partial-axfr.no-response", 2, True, 2, id="partial-immediate"),
        param("no-response.complete-axfr", 2, False, 1, id="immediate-complete"),
        param("partial-axfr.complete-axfr", 2, False, 1, id="partial-complete"),
        param("no-response", 1, True, 1, id="tries-1-no-second-retry"),
    ],
)
def test_axfr_retry_upon_tcp_eof(
    dig, ans5, set_response_sequence, sequence, tries, expect_failure, eof_errors
):
    """Check the exit code and the number of retries for an AXFR retried
    upon TCP EOF."""
    set_response_sequence(sequence)
    result = dig(f"@{ans5.ip} example AXFR +tries={tries}", raise_on_exception=False)
    assert (result.rc != 0) == expect_failure
    # Sanity check: ensure ans5 behaves as expected.
    eof_pattern = Re("communications error.*end of file")
    assert len(result.out.grep(eof_pattern)) == eof_errors


def test_axfr_no_retry_with_retry_0(dig, ans5, set_response_sequence):
    """Check that +retry=0 does not retry upon TCP EOF."""
    set_response_sequence("no-response")
    result = dig(f"@{ans5.ip} example AXFR +retry=0", raise_on_exception=False)
    assert result.rc != 0
    # Sanity check: ensure ans5 behaves as expected.
    eof_pattern = Re("communications error.*end of file")
    assert len(result.out.grep(eof_pattern)) == 1


@pytest.mark.parametrize(
    "option",
    [
        param("", id="udp"),
        param("+tcp", id="tcp"),
    ],
)
def test_timeout_then_servfail(dig, ans7, option):
    """Check that dig handles a timeout followed by a SERVFAIL
    correctly.  See GL #3020 for more information."""
    result = dig(f"+timeout=1 +nofail {option} @{ans7.ip} silent-then-servfail.example")
    assert "status: SERVFAIL" in result.out


def test_comments_retry_comment(dig, ans7):
    """Check that dig +comments emits the retry comment."""
    result = dig(
        f"+timeout=1 +nofail +comments @{ans7.ip} silent-then-servfail.example"
    )
    assert ";; Got SERVFAIL reply from" in result.out


def test_short_comments_suppresses_retry_comment(dig, ans7):
    """Check that dig +short +comments does not leak the ";; " comments
    into the short-form output.  +short normally turns comments off, but
    "+short +comments" re-enables them while short form is still in
    effect; the comment output then belongs to the verbose form and
    would corrupt the short output."""
    result = dig(
        f"+timeout=1 +nofail +short +comments @{ans7.ip} silent-then-servfail.example"
    )
    assert ";; Got SERVFAIL reply from" not in result.out


ERROR_PATTERN = Re("connection refused|timed out|network unreachable|host unreachable")


@pytest.mark.parametrize(
    "option",
    [
        param("", id="udp"),
        param("+tcp", id="tcp"),
    ],
)
def test_next_server_after_network_unreachable(dig, ns3, option):
    """Check that dig tries the next server after a socket network
    unreachable error."""
    result = dig(f"{option} @192.0.2.128 @{ns3.ip} a.example")
    assert len(result.out.grep(ERROR_PATTERN)) == 3
    assert "status: NOERROR" in result.out


def test_next_server_after_udp_read_error(dig, ns3):
    """Check that dig tries the next server after a UDP socket read
    error."""
    result = dig(f"@10.53.0.99 @{ns3.ip} a.example")
    assert "status: NOERROR" in result.out


def test_next_server_after_tcp_read_error(dig, ans7, ns3):
    """Check that dig tries the next server after a TCP socket read
    error."""
    result = dig(f"+tcp @{ans7.ip} @{ns3.ip} close.example")
    assert "status: NOERROR" in result.out


def test_next_server_after_tcp_connection_error(dig, ns3):
    """Check that dig tries the next server after a TCP socket connection
    error/timeout.  The connection error and timeout cases are combined,
    because it is not trivial to simulate the timeout case in a system
    test in Linux without a firewall, but the code which handles error
    cases during connection establishment does not differentiate between
    timeout and other types of errors (unlike during reading), so this
    one check should be sufficient for both cases."""
    result = dig(f"+tcp @10.53.0.99 @{ns3.ip} a.example")
    assert len(result.out.grep(ERROR_PATTERN)) == 3
    assert "status: NOERROR" in result.out


@pytest.mark.parametrize(
    "option",
    [
        param("", id="udp"),
        param("+tcp", id="tcp"),
    ],
)
def test_next_server_after_read_timeout(dig, ans7, ns3, option):
    """Check that dig tries the next server after socket read timeouts."""
    result = dig(f"+timeout=1 {option} @{ans7.ip} @{ns3.ip} silent.example")
    assert "status: NOERROR" in result.out


def test_mapped_ipv6_server_refused(dig, ans7):
    """Check that dig refuses to use a server with an IPv4-mapped IPv6
    address after failing with the regular IP address.  See GL #3248
    for more information."""
    result = dig(f"@{ans7.ip} @::ffff:{ans7.ip} silent.example")
    assert ";; Skipping mapped address" in result.out
    assert ";; No acceptable nameservers" in result.out


def test_qr_and_y_with_failed_query(dig, ns3):
    """Check that dig handles printing query information with +qr and +y
    when multiple queries are involved, including a failed one.  See
    GL #3244 for more information."""
    result = dig(f"+timeout=1 +qr +y @127.0.0.1 @{ns3.ip} a.example")
    assert "IN A 10.0.0.1" in result.out


def test_startup_banner_default(dig, ans7):
    """Check that dig prints the startup banner by default, including on
    the error path.  This makes the absence check with +nocmd
    meaningful."""
    result = dig(
        f"silent.example @{ans7.ip} +notcp +timeout=1 +tries=1",
        raise_on_exception=False,
    )
    assert result.rc != 0
    assert "<<>> DiG" in result.out
    assert "no servers could be reached" in result.out


def test_nocmd_after_query_name(dig, ans7):
    """Check that +nocmd placed after the query name suppresses the
    startup banner, including on the error path.  This regressed because
    the banner was built as soon as the query name was seen, before
    +nocmd had been parsed."""
    result = dig(
        f"silent.example @{ans7.ip} +notcp +timeout=1 +tries=1 +nocmd",
        raise_on_exception=False,
    )
    assert result.rc != 0
    assert "<<>> DiG" not in result.out
    assert "no servers could be reached" in result.out


def test_yaml_valid_when_no_server_reached(dig, ans7):
    """Check that dig +yaml produces valid YAML when no servers could be
    reached; the ";"-prefixed startup banner must not precede the
    DIG_ERROR block.  The query name is deliberately placed before +yaml
    on the command line: that is what makes dig build the banner (while
    +cmd is still in effect) before switching to YAML output, which is
    the ordering that regressed."""
    result = dig(
        f"silent.example @{ans7.ip} +notcp +timeout=1 +tries=1 +yaml",
        raise_on_exception=False,
    )
    assert result.rc != 0
    assert parse_yaml(result.out)[0]["type"] == "DIG_ERROR"


@isctest.mark.with_ipv6
def test_source_address_both_families_no_crash(dig, ns1):
    """Check that dig with an IPv4 source address and a server with both
    IPv4 and IPv6 addresses does not crash.  @localhost is not really
    expected to have an answer for the query; only a crash (termination
    by a signal) is an error.  Without IPv6, @localhost resolves to the
    IPv4 address only and the address-family mismatch under test never
    happens.  See GL #5609 for more information."""
    result = dig(f"@localhost example -b {ns1.ip}", raise_on_exception=False)
    assert result.rc >= 0


def test_yaml_any_output(dig, ns3):
    """Check the structure of dig +yaml output for an ANY query."""
    result = dig(f"+qr +yaml @{ns3.ip} any ns2.example")
    messages = parse_yaml(result.out)
    query = messages[0]["message"]["query_message_data"]
    assert query["status"] == "NOERROR"
    response = messages[1]["message"]["response_message_data"]
    assert response["status"] == "NOERROR"
    assert response["QUESTION_SECTION"][0] == "ns2.example. IN ANY"


def test_yaml_ipv6_trailing_zeroes(dig, ns3):
    """Check dig +yaml output of an IPv6 address ending in zeroes."""
    result = dig(f"+qr +yaml @{ns3.ip} aaaa d.example")
    response = parse_yaml(result.out)[1]["message"]["response_message_data"]
    answer = response["ANSWER_SECTION"][0]
    assert answer == "d.example. 300 IN AAAA fd92:7065:b8e:ffff::0"


@pytest.mark.parametrize(
    "qname", ["yaml", "'.yaml", "[.yaml", "{.yaml", "&.yaml", "#.yaml"]
)
def test_yaml_special_characters_in_qname(dig, ns3, qname):
    """Check that qnames containing characters special to YAML are quoted
    correctly in dig +yaml output."""
    result = dig(f"@{ns3.ip} +yaml {qname}.example TXT +qr")
    query = parse_yaml(result.out)[0]["message"]["query_message_data"]
    question = query["QUESTION_SECTION"][0]
    assert question == f"{qname}.example. IN TXT"
    response = parse_yaml(result.out)[1]["message"]["response_message_data"]
    answer = response["ANSWER_SECTION"][0]
    assert answer == f'{qname}.example. 300 IN TXT "a: b"'


def test_yaml_character_values(dig, ns3):
    """Check the quoting of all 256 character values in dig +yaml TXT
    output."""

    def quoted(i):
        char = chr(i)
        if char in ('"', "\\"):
            return f'"\\{char}"'
        if 32 <= i <= 126:
            return f'"{char}"'
        return f'"\\{i:03d}"'

    result = dig(f"@{ns3.ip} +yaml all.yaml.example TXT +qr")
    response = parse_yaml(result.out)[1]["message"]["response_message_data"]
    answer = response["ANSWER_SECTION"][0]
    strings = " ".join(quoted(i) for i in range(256))
    assert answer == f"all.yaml.example. 300 IN TXT {strings}"
