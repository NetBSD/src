#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0

from pathlib import Path

import json

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ec

import dns.dnssec
import dns.name
import dns.rdataclass
import dns.rdatatype
import pytest

import isctest
import isctest.mark

VICTIM = "victim.example."
ATTACKER = "abc.example."
ATTACK = f"foo.{VICTIM}"
VALID_NXDOMAIN = f"bar.{ATTACKER}"
CONTROL = f"wild1.{VICTIM}"
CONTROL_NODATA = f"wild2.{VICTIM}"
VICTIM_NSEC_OWNER = f"a.{VICTIM}"
ATTACKER_NSEC_OWNER = f"z.{ATTACKER}"
CONTROL_A = "192.0.2.1"
AUTH = "10.53.0.1"
RESOLVER = "10.53.0.2"

pytestmark = [
    isctest.mark.with_ecdsa_deterministic,
    pytest.mark.extra_artifacts(
        [
            "ans*/ans.run",
            "ans*/keys.json",
        ]
    ),
]


def _make_key():
    private_key = ec.generate_private_key(ec.SECP256R1())
    dnskey = dns.dnssec.make_dnskey(
        private_key.public_key(),
        algorithm="ECDSAP256SHA256",
        flags=257,
    )
    private_pem = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.PKCS8,
        encryption_algorithm=serialization.NoEncryption(),
    ).decode("ascii")
    return {
        "private_pem": private_pem,
        "dnskey": dnskey.to_text(),
    }


def bootstrap():
    keys = {zone: _make_key() for zone in [VICTIM, ATTACKER]}
    Path("ans1/keys.json").write_text(json.dumps(keys, indent=2), encoding="ascii")
    return {
        "VICTIM_DNSKEY": "".join(keys[VICTIM]["dnskey"].split()[3:]),
        "ATTACKER_DNSKEY": "".join(keys[ATTACKER]["dnskey"].split()[3:]),
    }


def _query(server, qname, qtype):
    query = isctest.query.create(qname, qtype)
    return isctest.query.tcp(query, server)


def _rrset(response, section, owner, rdtype, covers=None):
    if covers is None:
        return response.get_rrset(
            section,
            dns.name.from_text(owner),
            dns.rdataclass.IN,
            rdtype,
        )
    return response.get_rrset(
        section,
        dns.name.from_text(owner),
        dns.rdataclass.IN,
        rdtype,
        covers=covers,
    )


def _check_signer(response, section, owner, covered, signer):
    rrsig = _rrset(
        response,
        section,
        owner,
        dns.rdatatype.RRSIG,
        covers=covered,
    )
    assert rrsig is not None, response.to_text()
    assert rrsig[0].signer == dns.name.from_text(signer), response.to_text()


def test_forged_response_contains_cross_zone_nsec():
    response = _query(AUTH, ATTACK, "A")
    isctest.check.nxdomain(response)

    assert _rrset(response, response.authority, ATTACKER_NSEC_OWNER, dns.rdatatype.NSEC)
    _check_signer(
        response,
        response.authority,
        ATTACKER_NSEC_OWNER,
        dns.rdatatype.NSEC,
        ATTACKER,
    )

    assert _rrset(response, response.authority, VICTIM_NSEC_OWNER, dns.rdatatype.NSEC)
    _check_signer(
        response,
        response.authority,
        VICTIM_NSEC_OWNER,
        dns.rdatatype.NSEC,
        VICTIM,
    )


def test_resolver_rejects_cross_zone_nowildcard_proof():
    response = _query(RESOLVER, ATTACKER, "SOA")
    isctest.check.noerror(response)
    isctest.check.adflag(response)

    response = _query(RESOLVER, ATTACK, "A")
    isctest.check.servfail(response)
    isctest.check.noadflag(response)


def test_valid_same_zone_nxdomain_still_validates():
    response = _query(RESOLVER, VALID_NXDOMAIN, "A")
    isctest.check.nxdomain(response)
    isctest.check.adflag(response)


def test_valid_wildcard_still_validates():
    response = _query(RESOLVER, CONTROL, "A")
    isctest.check.noerror(response)
    isctest.check.adflag(response)
    answer = _rrset(response, response.answer, CONTROL, dns.rdatatype.A)
    assert answer is not None, response.to_text()
    assert any(rdata.address == CONTROL_A for rdata in answer), response.to_text()


def test_valid_wildcard_nodata_still_validates():
    response = _query(RESOLVER, CONTROL_NODATA, "AAAA")
    isctest.check.noerror(response)
    isctest.check.adflag(response)
    isctest.check.rr_count_eq(response.answer, 0)
