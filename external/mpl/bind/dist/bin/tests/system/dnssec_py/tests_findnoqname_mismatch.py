#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0

"""
The resolver must cache the NOQNAME proof that findnoqname() selected, and
serve it back, when the proof owner carries both NSEC and NSEC3 records in
any wire order (#5985, #6369).
"""

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ec

import dns.dnssec
import dns.name
import dns.rdataclass
import dns.rdatatype

from dnssec_py.common import DNSSEC_PY_MARK
from isctest.template import NS4, NS9, Nameserver, zones
from isctest.zone import PythonZoneKey, Zone

import isctest
import isctest.mark

ZONE = "f217.test."
CHILD = f"evil.{ZONE}"
ATTACK = f"www.{CHILD}"
ATTACK_NSEC3 = f"nsec3.{CHILD}"
ATTACK_BOTH = f"both.{CHILD}"
WILDCARD_LABELS = 3
NSEC_OWNER = f"00000000.{CHILD}"
NSEC3_OWNER = f"{'0' * 32}.{CHILD}"
FORGED_A = "192.0.2.217"
AUTH = "10.53.0.4"
RESOLVER = "10.53.0.9"

pytestmark = [isctest.mark.with_ecdsa_deterministic, DNSSEC_PY_MARK]


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
    # for a static stub zones, NS name is the stub and NS IP is the target
    ans = Nameserver(NS9.name, NS9.num, NS4.ip, NS4.ip6)

    zone = Zone(ZONE, ans, signed=False, zone_type="static-stub")
    zonekey = PythonZoneKey.generate(zone)
    zonekey.write_private_key_pem(f"ans4/{ZONE}.pem")
    zone.keys = [zonekey]

    ta = zonekey.into_ta("static-key")

    return {
        "zones": zones([zone]),
        "trust_anchors": [ta],
    }


def _query(server, qname, qtype):
    query = isctest.query.create(qname, qtype)
    return isctest.query.tcp(query, server, attempts=1, timeout=5)


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


def _check_proof(response, owner, rdtype, signed):
    """Check the denial type 'rdtype' is present at 'owner', signed or not."""
    assert _rrset(response, response.authority, owner, rdtype), response.to_text()
    rrsig = _rrset(response, response.authority, owner, dns.rdatatype.RRSIG, rdtype)
    if signed:
        _check_rrsig(response, response.authority, owner, rdtype, CHILD)
    else:
        assert rrsig is None, response.to_text()


def _check_forged_answer(server, qname):
    response = _query(server, qname, "A")
    isctest.check.noerror(response)
    assert _has_a(response, response.answer, qname, FORGED_A), response.to_text()
    _check_rrsig(
        response, response.answer, qname, dns.rdatatype.A, CHILD, WILDCARD_LABELS
    )
    return response


def test_malicious_findnoqname_addnoqname_mismatch():
    # #5985: signed NSEC followed by unsigned NSEC3
    response = _check_forged_answer(AUTH, ATTACK)
    _check_proof(response, NSEC_OWNER, dns.rdatatype.NSEC, signed=True)
    _check_proof(response, NSEC_OWNER, dns.rdatatype.NSEC3, signed=False)


def test_malicious_nsec3_then_unsigned_nsec():
    # #6369: signed NSEC3 followed by unsigned NSEC
    response = _check_forged_answer(AUTH, ATTACK_NSEC3)
    _check_proof(response, NSEC3_OWNER, dns.rdatatype.NSEC3, signed=True)
    _check_proof(response, NSEC3_OWNER, dns.rdatatype.NSEC, signed=False)


def test_malicious_both_signed():
    # both denial types signed, non-covering NSEC first
    response = _check_forged_answer(AUTH, ATTACK_BOTH)
    _check_proof(response, NSEC3_OWNER, dns.rdatatype.NSEC, signed=True)
    _check_proof(response, NSEC3_OWNER, dns.rdatatype.NSEC3, signed=True)


def _check_cached_proof(qname, owner, selected, other):
    # The trigger query caches the forged answer along with the NOQNAME
    # proof that findnoqname() selected...
    _check_forged_answer(RESOLVER, qname)

    # ...and the cached answer is served with that same proof.
    response = _check_forged_answer(RESOLVER, qname)
    _check_proof(response, owner, selected, signed=True)
    assert (
        _rrset(response, response.authority, owner, other) is None
    ), response.to_text()

    # named is still alive
    response = _query(RESOLVER, ZONE, "SOA")
    isctest.check.noerror(response)


def test_resolver_findnoqname_addnoqname_mismatch():
    # #5985: signed NSEC followed by unsigned NSEC3
    _check_cached_proof(ATTACK, NSEC_OWNER, dns.rdatatype.NSEC, dns.rdatatype.NSEC3)


def test_resolver_nsec3_then_unsigned_nsec():
    # #6369: signed NSEC3 followed by unsigned NSEC
    _check_cached_proof(
        ATTACK_NSEC3, NSEC3_OWNER, dns.rdatatype.NSEC3, dns.rdatatype.NSEC
    )


def test_resolver_keeps_selected_proof():
    # both denial types signed: the covering NSEC3 was selected, not the
    # NSEC that comes first in wire order
    _check_cached_proof(
        ATTACK_BOTH, NSEC3_OWNER, dns.rdatatype.NSEC3, dns.rdatatype.NSEC
    )
