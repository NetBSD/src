#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0

from pathlib import Path

import json

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ec

import dns.dnssec
import dns.flags
import dns.name
import dns.rdataclass
import dns.rdatatype
import pytest

import isctest
import isctest.mark

PARENT = "p22.hack."
CHILD = f"c.{PARENT}"
TRIGGER = f"www.{CHILD}"
PRIME_NX = f"0.{PARENT}"
VICTIM = f"victim.{PARENT}"
AAC = f"aac.{PARENT}"
STUFFED_ENT = f"t.{PARENT}"
STUFFED_RANGE = f"aab.{PARENT}"
CHILD_A = "192.0.2.50"
VICTIM_A = "192.0.2.99"
AAC_A = "192.0.2.77"
AUTH_PARENT = "10.53.0.1"
AUTH_CHILD = "10.53.0.2"
RESOLVER = "10.53.0.3"

pytestmark = [
    isctest.mark.with_ecdsa_deterministic,
    pytest.mark.extra_artifacts(
        [
            "ans*/ans.run",
            "ans*/keys.json",
            "ns2/managed-keys.bind.jnl",
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
    keys = {PARENT: _make_key()}

    Path("ans1/keys.json").write_text(json.dumps(keys, indent=2), encoding="ascii")

    parent_dnskey = "".join(keys[PARENT]["dnskey"].split()[3:])
    return {"PARENT_DNSKEY": parent_dnskey}


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


def _has_a(response, owner, address):
    rrset = _rrset(response, response.answer, owner, dns.rdatatype.A)
    return rrset is not None and any(rdata.address == address for rdata in rrset)


def _has_nsec(response, owner, section=None):
    if section is None:
        section = response.authority
    return _rrset(response, section, owner, dns.rdatatype.NSEC) is not None


def _has_rrsig(response, owner, section, covers):
    return (
        _rrset(
            response,
            section,
            owner,
            dns.rdatatype.RRSIG,
            covers=covers,
        )
        is not None
    )


def prime_resolver():
    # Caches parent SOA at dns_trust_secure
    # DNSKEY is queried and cached as part of this.
    soa = _query(RESOLVER, PARENT, "SOA")
    isctest.check.noerror(soa)
    isctest.check.adflag(soa)

    # Caches real apex parent NSEC at dns_trust_secure
    nx = _query(RESOLVER, PRIME_NX, "A")
    isctest.check.nxdomain(nx)
    isctest.check.adflag(nx)
    assert _has_nsec(nx, PARENT), nx


def test_malicious_referral():
    referral = _query(AUTH_PARENT, TRIGGER, "A")
    isctest.check.noerror(referral)
    assert referral.flags & dns.flags.AA
    assert _has_nsec(referral, CHILD), referral.to_text()
    assert _has_rrsig(referral, CHILD, referral.authority, dns.rdatatype.NSEC)
    assert _has_nsec(referral, STUFFED_ENT), referral.to_text()
    assert _has_nsec(referral, STUFFED_RANGE), referral.to_text()
    assert _has_rrsig(referral, STUFFED_ENT, referral.authority, dns.rdatatype.NSEC)
    assert _has_rrsig(referral, STUFFED_RANGE, referral.authority, dns.rdatatype.NSEC)
    assert _rrset(referral, referral.answer, TRIGGER, dns.rdatatype.A) is None

    child = _query(AUTH_CHILD, TRIGGER, "A")
    isctest.check.noerror(child)
    assert _has_a(child, TRIGGER, CHILD_A), child.to_text()


@pytest.mark.parametrize(
    "qname,address",
    [
        (VICTIM, VICTIM_A),
        (AAC, AAC_A),
    ],
    ids=["nodata", "nxdomain"],
)
def test_nsec_piggyback_cache_poisoning(qname, address):
    """
    Reproducer for #5977:
    F-022 NODATA/NXDOMAIN synthesises from piggy-backed NSEC RRs
    """
    # Prime the cache.
    prime_resolver()

    # Trigger the malicious referral.
    trigger = _query(RESOLVER, TRIGGER, "A")
    isctest.check.noerror(trigger)
    isctest.check.noadflag(trigger)
    assert _has_a(trigger, TRIGGER, CHILD_A), trigger.to_text()

    # Probing query.
    response = _query(RESOLVER, qname, "A")
    isctest.check.noerror(response)
    assert _has_a(response, qname, address), response.to_text()
    isctest.check.adflag(response)
