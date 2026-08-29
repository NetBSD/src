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

"""Handler for the attacker.rrsig-labels-signer. zone.

Serves crafted DNSSEC responses that exercise the RRSIG Labels underflow
check: an A query for any name under attacker.rrsig-labels-signer. returns:

  ANSWER:
    <qname> A 192.0.2.1
    <qname> RRSIG A 13 1 ...  (Labels=1, signed over *.rrsig-labels-signer.)

  AUTHORITY:
    <hash-1>.attacker.rrsig-labels-signer. NSEC3 ...  (covers
        hash(attacker.rrsig-labels-signer.) — next-closer proof for BIND)

The RRSIG has Labels=1 because the canonical signing input is
*.rrsig-labels-signer. A 192.0.2.1.  The signer name is
attacker.rrsig-labels-signer. (3 non-root labels), so Labels(1) is less
than signer_labels(3) - 1 = 2 — the condition the fix adds to
dns_dnssec_verify() in lib/dns/dnssec.c.

Key material is written by bootstrap() in tests_rrsig_labels_signer.py to
attacker_rrsig_labels_signer.pem in this directory before any server starts.
"""

from collections.abc import AsyncGenerator
from pathlib import Path

import base64
import hashlib
import logging
import time

from cryptography.hazmat.primitives import serialization
from dns.rdtypes.dnskeybase import Flag

import dns.dnssec
import dns.name
import dns.rcode
import dns.rdata
import dns.rdataclass
import dns.rdatatype
import dns.rdtypes.ANY.NSEC3
import dns.rrset

from isctest.asyncserver import (
    DnsResponseSend,
    DomainHandler,
    QueryContext,
    ResponseAction,
)

ZONE_NAME = "attacker.rrsig-labels-signer."
PARENT_ZONE_NAME = "rrsig-labels-signer."
POISON_IP = "192.0.2.1"
TTL = 300
PEM_PATH = Path("attacker_rrsig_labels_signer.pem")


def _to_b32hex_lower(data: bytes) -> str:
    """Base32hex-encode bytes, lowercase, no padding (NSEC3 owner-name form)."""
    return base64.b32hexencode(data).decode().rstrip("=").lower()


def _nsec3_sha1(name: str) -> bytes:
    """SHA-1 NSEC3 hash of name (0 iterations, empty salt)."""
    wire = dns.name.from_text(name).canonicalize().to_wire()
    return hashlib.sha1(wire).digest()


def _inc_bytes(b: bytes) -> bytes:
    out = bytearray(b)
    for i in range(len(out) - 1, -1, -1):
        out[i] += 1
        if out[i] != 0:
            break
    return bytes(out)


def _dec_bytes(b: bytes) -> bytes:
    out = bytearray(b)
    for i in range(len(out) - 1, -1, -1):
        if out[i] > 0:
            out[i] -= 1
            break
        out[i] = 0xFF
    return bytes(out)


def _type_bitmap(*types: int) -> bytes:
    """Build NSEC / NSEC3 type bitmap bytes (window 0, types 0–255)."""
    size = (max(types) >> 3) + 1
    bm = bytearray(size)
    for t in types:
        bm[t >> 3] |= 1 << (7 - (t & 7))
    return bytes(bm)


class AttackerZoneHandler(DomainHandler):
    """Serve attacker.rrsig-labels-signer. with crafted wildcard RRSIG."""

    domains = [ZONE_NAME]

    def __init__(self) -> None:
        super().__init__()
        self._zone = dns.name.from_text(ZONE_NAME)

        priv = serialization.load_pem_private_key(PEM_PATH.read_bytes(), password=None)

        self._dnskey = dns.dnssec.make_dnskey(
            priv.public_key(),
            dns.dnssec.Algorithm.ECDSAP256SHA256,
            flags=Flag.ZONE | Flag.SEP,
        )
        logging.info("attacker DNSKEY keytag: %d", dns.dnssec.key_id(self._dnskey))

        now = int(time.time())
        inception = now - 3600
        expiration = now + 14 * 86400

        # DNSKEY RRset and self-signature
        self._dnskey_rrset = dns.rrset.RRset(
            self._zone, dns.rdataclass.IN, dns.rdatatype.DNSKEY
        )
        self._dnskey_rrset.update_ttl(TTL)
        self._dnskey_rrset.add(self._dnskey)
        self._dnskey_rrsig = dns.dnssec.sign(
            self._dnskey_rrset,
            priv,
            signer=self._zone,
            dnskey=self._dnskey,
            inception=inception,
            expiration=expiration,
            lifetime=None,
            deterministic=False,  # for OpenSSL<3.2.0 compat
        )

        # Crafted RRSIG for *.rrsig-labels-signer. A  →  Labels=1
        wild_rrset = dns.rrset.RRset(
            dns.name.from_text(f"*.{PARENT_ZONE_NAME}"),
            dns.rdataclass.IN,
            dns.rdatatype.A,
        )
        wild_rrset.update_ttl(TTL)
        wild_rrset.add(
            dns.rdata.from_text(dns.rdataclass.IN, dns.rdatatype.A, POISON_IP)
        )
        self._crafted_a_rrsig = dns.dnssec.sign(
            wild_rrset,
            priv,
            signer=self._zone,
            dnskey=self._dnskey,
            inception=inception,
            expiration=expiration,
            lifetime=None,
            deterministic=False,  # for OpenSSL<3.2.0 compat
        )
        logging.info(
            "crafted A RRSIG labels=%d (expected 1)", self._crafted_a_rrsig.labels
        )

        # NSEC3 covering hash(attacker.rrsig-labels-signer.) for the
        # NEEDNOQNAME proof (validator.c:findnsec3proofs).
        h = _nsec3_sha1(ZONE_NAME)
        h_owner = _dec_bytes(h)  # owner hash just below target
        h_next = _inc_bytes(h)  # next  hash just above target

        owner_name = dns.name.from_text(f"{_to_b32hex_lower(h_owner)}.{ZONE_NAME}")
        nsec3_rdata = dns.rdtypes.ANY.NSEC3.NSEC3(
            rdclass=dns.rdataclass.IN,
            rdtype=dns.rdatatype.NSEC3,
            algorithm=1,  # SHA-1
            flags=0,
            iterations=0,
            salt=b"",
            next=h_next,
            windows=[(0, _type_bitmap(dns.rdatatype.A, dns.rdatatype.RRSIG))],
        )
        self._nsec3_rrset = dns.rrset.RRset(
            owner_name, dns.rdataclass.IN, dns.rdatatype.NSEC3
        )
        self._nsec3_rrset.update_ttl(TTL)
        self._nsec3_rrset.add(nsec3_rdata)

        nsec3_rrsig = dns.dnssec.sign(
            self._nsec3_rrset,
            priv,
            signer=self._zone,
            dnskey=self._dnskey,
            inception=inception,
            expiration=expiration,
            lifetime=None,
            deterministic=False,  # for OpenSSL<3.2.0 compat
        )
        self._nsec3_rrsig_rrset = dns.rrset.RRset(
            owner_name,
            dns.rdataclass.IN,
            dns.rdatatype.RRSIG,
            dns.rdatatype.NSEC3,
        )
        self._nsec3_rrsig_rrset.update_ttl(TTL)
        self._nsec3_rrsig_rrset.add(nsec3_rrsig)

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        qtype = qctx.qtype
        qname = qctx.qname
        response = qctx.prepare_new_response(with_zone_data=False)

        if qtype == dns.rdatatype.DNSKEY and qname == self._zone:
            response.set_rcode(dns.rcode.NOERROR)
            dnskey_rrsig_rrset = dns.rrset.RRset(
                self._zone,
                dns.rdataclass.IN,
                dns.rdatatype.RRSIG,
                dns.rdatatype.DNSKEY,
            )
            dnskey_rrsig_rrset.update_ttl(TTL)
            dnskey_rrsig_rrset.add(self._dnskey_rrsig)
            response.answer.extend([self._dnskey_rrset, dnskey_rrsig_rrset])
            yield DnsResponseSend(response, authoritative=True)

        elif qtype == dns.rdatatype.A and qname.is_subdomain(self._zone):
            response.set_rcode(dns.rcode.NOERROR)
            a_rrset = dns.rrset.RRset(qname, dns.rdataclass.IN, dns.rdatatype.A)
            a_rrset.update_ttl(TTL)
            a_rrset.add(
                dns.rdata.from_text(dns.rdataclass.IN, dns.rdatatype.A, POISON_IP)
            )
            a_rrsig_rrset = dns.rrset.RRset(
                qname,
                dns.rdataclass.IN,
                dns.rdatatype.RRSIG,
                dns.rdatatype.A,
            )
            a_rrsig_rrset.update_ttl(TTL)
            a_rrsig_rrset.add(self._crafted_a_rrsig)
            response.answer.extend([a_rrset, a_rrsig_rrset])
            response.authority.extend([self._nsec3_rrset, self._nsec3_rrsig_rrset])
            yield DnsResponseSend(response, authoritative=True)

        else:
            # REFUSED for anything we don't handle
            yield DnsResponseSend(response)
