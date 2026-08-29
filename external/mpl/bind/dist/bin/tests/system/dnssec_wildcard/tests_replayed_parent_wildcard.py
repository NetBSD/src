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

PARENT = "f045.test."
CHILD = f"child.{PARENT}"
QUERY = f"q.{PARENT}"
SERVICE = f"svc.{CHILD}"
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


def _has_a(response, section, owner, address):
    rrset = _rrset(response, section, owner, dns.rdatatype.A)
    return rrset is not None and any(rdata.address == address for rdata in rrset)


def _check_signed_rrset(response, section, owner, rdtype, signer, labels=None):
    rrsig = _rrset(
        response,
        section,
        owner,
        dns.rdatatype.RRSIG,
        covers=rdtype,
    )
    assert rrsig is not None, response.to_text()
    assert rrsig[0].signer == dns.name.from_text(signer), response.to_text()
    if labels is not None:
        assert rrsig[0].labels == labels, response.to_text()


def prime_parent_soa():
    response = _query(RESOLVER, PARENT, "SOA")
    isctest.check.noerror(response)
    isctest.check.adflag(response)
    assert _rrset(response, response.answer, PARENT, dns.rdatatype.SOA) is not None
    _check_signed_rrset(response, response.answer, PARENT, dns.rdatatype.SOA, PARENT)


def test_malicious_replay():
    # Trigger query.
    response = _query(AUTH, QUERY, "MX")
    isctest.check.noerror(response)
    isctest.check.aaflag(response)

    assert _rrset(response, response.answer, QUERY, dns.rdatatype.MX)
    _check_signed_rrset(response, response.answer, QUERY, dns.rdatatype.MX, PARENT)

    # The reply carries in ADDITIONAL.  Note Labels=2, signer=f045.test.
    assert _has_a(response, response.additional, SERVICE, FORGED_A), response.to_text()
    _check_signed_rrset(
        response,
        response.additional,
        SERVICE,
        dns.rdatatype.A,
        signer=PARENT,
        labels=2,
    )

    # Victim query — any later client, with CD=0:
    child = _query(AUTH, SERVICE, "A")
    isctest.check.noerror(child)

    assert _has_a(child, child.answer, SERVICE, LEGIT_A), child.to_text()
    assert not _has_a(child, child.answer, SERVICE, FORGED_A), child.to_text()
    _check_signed_rrset(child, child.answer, SERVICE, dns.rdatatype.A, CHILD)


def test_replayed_parent_wildcard():
    # Prime the parent's DNSKEY.
    prime_parent_soa()

    # Trigger query — the on-path injection happens on the upstream side of
    # this single fetch.  Requires CD=1.
    response = _query(RESOLVER, QUERY, "MX", cd=True)
    isctest.check.noerror(response)
    assert _rrset(response, response.answer, QUERY, dns.rdatatype.MX)

    # Victim query — any later client, with CD=0:
    response = _query(RESOLVER, SERVICE, "A")
    isctest.check.noerror(response)
    isctest.check.adflag(response)

    assert not _has_a(response, response.answer, SERVICE, FORGED_A)
    assert _has_a(response, response.answer, SERVICE, LEGIT_A), response.to_text()
    _check_signed_rrset(response, response.answer, SERVICE, dns.rdatatype.A, CHILD)
