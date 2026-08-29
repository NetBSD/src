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

PARENT = "f044.test."
CHILD = f"child.{PARENT}"
QUERY = f"q.{PARENT}"
SERVICE = f"svc.{CHILD}"
FORGED_A = "6.6.6.6"
LEGIT_A = "192.0.2.111"
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
    parent_dnskey = "".join(keys[PARENT]["dnskey"].split()[3:])
    return {"PARENT_DNSKEY": parent_dnskey}


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


def _check_rrsig(response, section, owner, rdtype, signer):
    rrsig = _rrset(response, section, owner, dns.rdatatype.RRSIG, covers=rdtype)
    assert rrsig is not None, response.to_text()
    assert rrsig[0].signer == dns.name.from_text(signer), response.to_text()


def test_resolver_rejects_ancestor_signed_additional_replay():
    # MX includes an A record in the additional section, which is
    # under the zone cut but signed by the parent zone. It is cached as
    # pending.
    response = _query(AUTH, QUERY, "MX")
    isctest.check.noerror(response)
    isctest.check.noadflag(response)
    assert _rrset(response, response.answer, QUERY, dns.rdatatype.MX)
    assert _has_a(response, response.additional, SERVICE, FORGED_A), response.to_text()
    _check_rrsig(response, response.additional, SERVICE, dns.rdatatype.A, PARENT)

    # Check that the child chain validates normally
    response = _query(RESOLVER, CHILD, "SOA")
    isctest.check.noerror(response)
    isctest.check.adflag(response)

    # Fetch the MX again (CD=0). The bad answer should now be omitted.
    response = _query(RESOLVER, QUERY, "MX", cd=True)
    isctest.check.noerror(response)
    isctest.check.noadflag(response)
    isctest.check.rr_count_eq(response.additional, 0)
    assert not _has_a(
        response, response.additional, SERVICE, FORGED_A
    ), response.to_text()

    # Query for the A directly; the parent's answer should now be
    # ejected from the cache and the child's answer used instead.
    response = _query(RESOLVER, SERVICE, "A")
    isctest.check.noerror(response)
    isctest.check.adflag(response)
    assert not _has_a(response, response.answer, SERVICE, FORGED_A), response.to_text()
    assert _has_a(response, response.answer, SERVICE, LEGIT_A), response.to_text()
    _check_rrsig(response, response.answer, SERVICE, dns.rdatatype.A, CHILD)
