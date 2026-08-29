#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0


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
NSEC_OWNER = f"00000000.{CHILD}"
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


def test_malicious_findnoqname_addnoqname_mismatch():
    response = _query(AUTH, ATTACK, "A")
    isctest.check.noerror(response)
    assert _has_a(response, response.answer, ATTACK, FORGED_A), response.to_text()
    _check_rrsig(response, response.answer, ATTACK, dns.rdatatype.A, CHILD, labels=1)

    # Has NSEC
    assert _rrset(response, response.authority, NSEC_OWNER, dns.rdatatype.NSEC)
    _check_rrsig(response, response.authority, NSEC_OWNER, dns.rdatatype.NSEC, CHILD)
    # Has NSEC3
    assert _rrset(response, response.authority, NSEC_OWNER, dns.rdatatype.NSEC3)
    assert (
        _rrset(
            response,
            response.authority,
            NSEC_OWNER,
            dns.rdatatype.RRSIG,
            covers=dns.rdatatype.NSEC3,
        )
        is None
    )


def test_resolver_findnoqname_addnoqname_mismatch():
    # Send one trigger query
    _query(RESOLVER, ATTACK, "A")
    response = _query(RESOLVER, ZONE, "SOA")
    isctest.check.noerror(response)
