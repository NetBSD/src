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

TESTZONE = "f043.test."
QUERY = f"svc.{TESTZONE}"
VICTIM = f"victim.{TESTZONE}"
FORGED_A = "198.51.100.45"
LEGIT_A = "192.0.2.113"
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
    keys = {TESTZONE: _make_key(TESTZONE)}
    Path("ans1/keys.json").write_text(json.dumps(keys, indent=2), encoding="ascii")
    zone_dnskey = "".join(keys[TESTZONE]["dnskey"].split()[3:])
    return {"TESTZONE_DNSKEY": zone_dnskey}


def _query(server, qname, qtype, cd=False):
    query = isctest.query.create(qname, qtype, cd=cd)
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


def _check_rrsig(response, section, owner, rdtype, signer, labels=None):
    rrsig = _rrset(response, section, owner, dns.rdatatype.RRSIG, covers=rdtype)
    assert rrsig is not None, response.to_text()
    assert rrsig[0].signer == dns.name.from_text(signer), response.to_text()
    if labels is not None:
        assert rrsig[0].labels == labels, response.to_text()


def test_direct_fromwildcard_additional_fixture():
    carrier = _query(AUTH, QUERY, "MX")
    isctest.check.noerror(carrier)
    assert _rrset(carrier, carrier.answer, QUERY, dns.rdatatype.MX)
    assert _has_a(carrier, carrier.additional, VICTIM, FORGED_A), carrier.to_text()
    _check_rrsig(
        carrier,
        carrier.additional,
        VICTIM,
        dns.rdatatype.A,
        TESTZONE,
        labels=2,
    )


def test_resolver_rejects_fromwildcard_additional_replay():
    soa = _query(RESOLVER, TESTZONE, "SOA")
    isctest.check.noerror(soa)
    isctest.check.adflag(soa)

    carrier = _query(RESOLVER, QUERY, "MX", cd=True)
    isctest.check.noerror(carrier)

    response = _query(RESOLVER, VICTIM, "A")
    isctest.check.noerror(response)
    isctest.check.adflag(response)
    assert not _has_a(response, response.answer, VICTIM, FORGED_A), response.to_text()
    assert _has_a(response, response.answer, VICTIM, LEGIT_A), response.to_text()
    _check_rrsig(response, response.answer, VICTIM, dns.rdatatype.A, TESTZONE)
