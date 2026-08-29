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

F004_ZONE = "f004.test."
VICTIM = f"victim.{F004_ZONE}"
POISON_NEXT = f"z.{F004_ZONE}"
VICTIM_A = "203.0.113.1"
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


def _make_key(zone: str):
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
    keys = {F004_ZONE: _make_key(F004_ZONE)}
    Path("ans1/keys.json").write_text(json.dumps(keys, indent=2), encoding="ascii")
    dnskey = "".join(keys[F004_ZONE]["dnskey"].split()[3:])
    return {"DNSKEY": dnskey}


def _query(server, qname, qtype, cd=False, dnssec=True):
    query = isctest.query.create(qname, qtype, cd=cd, dnssec=dnssec)
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


def _check_forged_nsec(response, section):
    nsec = _rrset(response, section, VICTIM, dns.rdatatype.NSEC)
    assert nsec is not None, response.to_text()
    assert nsec[0].next == dns.name.from_text(POISON_NEXT), response.to_text()

    rrsig = _rrset(
        response,
        section,
        VICTIM,
        dns.rdatatype.RRSIG,
        covers=dns.rdatatype.NSEC,
    )
    assert rrsig is not None, response.to_text()
    assert rrsig[0].signer == dns.name.from_text(F004_ZONE), response.to_text()
    assert rrsig[0].key_tag == 9999, response.to_text()


def test_pending_exact_nodata_nsec_cache_poisoning():
    """
    Companion to #5872 that covers the query_coveringnsec() trust check on
    the exact-match NODATA branch (the `if (exists)` path in ns/query.c),
    rather than the covering-NSEC path gated in qpcache.c find_coveringnsec().

    An NSEC owned by the victim name itself, injected at pending trust via a
    CD=1 query, is returned by the cache as a NODATA proof for the exact node.
    It must not be used to synthesize a NODATA answer that would deny the
    victim's real A record.
    """
    # Prime the zone SOA (and the DNSKEY as a subquery) at secure trust.
    soa = _query(RESOLVER, F004_ZONE, "SOA")
    isctest.check.noerror(soa)
    isctest.check.adflag(soa)

    # Inject a forged NSEC owned by the victim name via a CD=1 query; it is
    # cached at pending trust on the victim node.
    poison = _query(RESOLVER, VICTIM, "NSEC", cd=True)
    isctest.check.noerror(poison)
    isctest.check.noadflag(poison)
    _check_forged_nsec(poison, poison.answer)

    # Query the victim's A record. The pending NSEC must not be used to
    # synthesize a NODATA; the resolver fetches and validates the real A.
    response = _query(RESOLVER, VICTIM, "A")
    isctest.check.noerror(response)
    assert _has_a(response, VICTIM, VICTIM_A), response.to_text()
    isctest.check.adflag(response)
    assert _rrset(response, response.answer, VICTIM, dns.rdatatype.NSEC) is None
    assert _rrset(response, response.authority, VICTIM, dns.rdatatype.NSEC) is None
