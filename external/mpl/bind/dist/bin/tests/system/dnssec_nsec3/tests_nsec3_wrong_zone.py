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

PARENT = "f025.test."
CHILD = f"evil.{PARENT}"
CLOSEST = f"victim2.{CHILD}"
ATTACK = f"b.{CLOSEST}"
LEGIT = f"legit.{CHILD}"
FORGED_A = "6.6.6.6"
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


def _make_key(zone):
    private_key = ec.generate_private_key(ec.SECP256R1())
    dnskey = dns.dnssec.make_dnskey(
        private_key.public_key(),
        algorithm="ECDSAP256SHA256",
        flags=257,
    )
    ds = dns.dnssec.make_ds(dns.name.from_text(zone), dnskey, "SHA256")
    private_pem = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.PKCS8,
        encryption_algorithm=serialization.NoEncryption(),
    ).decode("ascii")
    return {
        "private_pem": private_pem,
        "dnskey": dnskey.to_text(),
        "ds": ds.to_text(),
    }


def bootstrap():
    keys = {zone: _make_key(zone) for zone in [PARENT, CHILD]}
    Path("ans1/keys.json").write_text(json.dumps(keys, indent=2), encoding="ascii")
    zone_dnskey = "".join(keys[PARENT]["dnskey"].split()[3:])
    return {"ZONE_DNSKEY": zone_dnskey}


def _query(server, qname, qtype):
    query = isctest.query.create(qname, qtype)
    return isctest.query.tcp(query, server)


def _rrset(response, section, owner, rdtype, covers=None):
    if covers is None:
        return response.get_rrset(
            section, dns.name.from_text(owner), dns.rdataclass.IN, rdtype
        )
    return response.get_rrset(
        section,
        dns.name.from_text(owner),
        dns.rdataclass.IN,
        rdtype,
        covers=covers,
    )


def _has_a(response, section, owner, address):
    rrset = _rrset(response, section, owner, dns.rdatatype.A)
    return rrset is not None and any(rdata.address == address for rdata in rrset)


def _has_nsec3_signed_by(response, signer):
    signer_name = dns.name.from_text(signer)
    for rrset in response.authority:
        if rrset.rdtype != dns.rdatatype.NSEC3:
            continue
        rrsig = _rrset(
            response,
            response.authority,
            rrset.name.to_text(),
            dns.rdatatype.RRSIG,
            covers=dns.rdatatype.NSEC3,
        )
        if rrsig is not None and rrsig[0].signer == signer_name:
            return True
    return False


def _check_rrsig(response, section, owner, rdtype, signer, labels=None):
    rrsig = _rrset(response, section, owner, dns.rdatatype.RRSIG, covers=rdtype)
    assert rrsig is not None, response.to_text()
    assert rrsig[0].signer == dns.name.from_text(signer), response.to_text()
    if labels is not None:
        assert rrsig[0].labels == labels, response.to_text()


def test_forged_response():
    response = _query(AUTH, ATTACK, "A")
    isctest.check.noerror(response)
    assert _has_a(response, response.answer, ATTACK, FORGED_A), response.to_text()
    _check_rrsig(response, response.answer, ATTACK, dns.rdatatype.A, CHILD, labels=3)
    assert _has_nsec3_signed_by(response, PARENT), response.to_text()


def test_resolver_rejects_wrong_zone_nsec3_noqname_proof():
    child_soa = _query(RESOLVER, CHILD, "SOA")
    isctest.check.noerror(child_soa)
    isctest.check.adflag(child_soa)

    # The verified wildcard answer RRSet in the response provides the validator
    # with a (candidate) closest encloser for QNAME. The validator MUST check that
    # the closest encloser is from the correct zone. If this is not the case,
    # as is with this test case, the response must be treated as bogus.
    response = _query(RESOLVER, ATTACK, "A")
    isctest.check.servfail(response)
    isctest.check.noadflag(response)
    assert not _has_a(response, response.answer, ATTACK, FORGED_A), response.to_text()


def test_resolver_accepts_own_zone_nsec3_wildcard_proof():
    # Send query eligble for wildcard expansion (1-label expansion + correct own-zone NSEC3):
    response = _query(RESOLVER, LEGIT, "A")
    isctest.check.noerror(response)
    isctest.check.adflag(response)
    assert _has_a(response, response.answer, LEGIT, FORGED_A), response.to_text()
    assert _has_nsec3_signed_by(response, CHILD), response.to_text()
