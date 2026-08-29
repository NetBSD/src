#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0

from collections.abc import AsyncGenerator
from dataclasses import dataclass
from datetime import datetime, timedelta, timezone

import base64

import dns.dnssec
import dns.message
import dns.rcode
import dns.rdata
import dns.rdataclass
import dns.rdatatype
import dns.rrset

from dnssec_nsec3.ans1.common import (
    Key,
    add_signed,
    name,
    nsec3_hash,
    nsec3_rrset,
    prepare_response,
    rrset,
    rrset_from_rdata,
    soa_rrset,
)
from isctest.asyncserver import DnsResponseSend, DomainHandler, QueryContext

TTL = 300
F055_ZONE = "f055.test."
SAFE = f"safe.{F055_ZONE}"
CUT = f"cut.{F055_ZONE}"
CONTROL = f"ctl.{F055_ZONE}"
ATTACK = f"a.{CUT}"
CONTROL_ATTACK = f"a.{CONTROL}"
SAFE_A = "198.51.100.55"
FORGED_A = "192.0.2.55"


def garbage_rrsig(covered: dns.rrset.RRset, signer: Key) -> dns.rrset.RRset:
    now = datetime.now(timezone.utc)
    inception = (now - timedelta(hours=1)).strftime("%Y%m%d%H%M%S")
    expiration = (now + timedelta(days=1)).strftime("%Y%m%d%H%M%S")
    signature = base64.b64encode(bytes(64)).decode("ascii")
    labels = len(covered.name.labels) - 1
    text = (
        f"{dns.rdatatype.to_text(covered.rdtype)} "
        f"13 {labels} {covered.ttl} {expiration} {inception} "
        f"{dns.dnssec.key_id(signer.dnskey)} {F055_ZONE} {signature}"
    )
    rdata = dns.rdata.from_text(dns.rdataclass.IN, dns.rdatatype.RRSIG, text)
    return dns.rrset.from_rdata(covered.name, covered.ttl, rdata)


def base32hex_add(hash_text: str, delta: int) -> str:
    raw = bytearray(base64.b32hexdecode(hash_text.upper()))
    value = int.from_bytes(raw, "big") + delta
    value %= 1 << (8 * len(raw))
    return base64.b32hexencode(value.to_bytes(len(raw), "big")).decode("ascii")


def forged_cut_nsec3() -> dns.rrset.RRset:
    cut_hash = nsec3_hash(CUT)
    return nsec3_rrset(
        F055_ZONE,
        base32hex_add(cut_hash, -1),
        base32hex_add(cut_hash, 1),
        1,
        "A",
        "RRSIG",
    )


@dataclass(frozen=True)
class ChainEntry:
    owner: str
    owner_hash: str
    types: tuple[str, ...]


class Nsec3Chain:
    def __init__(self) -> None:
        entries = [
            (F055_ZONE, ("NS", "SOA", "RRSIG", "DNSKEY", "NSEC3PARAM")),
            (SAFE, ("A", "RRSIG")),
            (CUT, ("A", "RRSIG")),
            (CONTROL, ("A", "RRSIG")),
        ]
        self.entries = sorted(
            (ChainEntry(owner, nsec3_hash(owner), types) for owner, types in entries),
            key=lambda entry: entry.owner_hash,
        )
        self.by_owner = {entry.owner: entry for entry in self.entries}

    def rrset_for(self, owner: str) -> dns.rrset.RRset:
        entry = self.by_owner[owner]
        index = self.entries.index(entry)
        next_hash = self.entries[(index + 1) % len(self.entries)].owner_hash
        return nsec3_rrset(F055_ZONE, entry.owner_hash, next_hash, 0, *entry.types)

    def cover_for(self, owner: str) -> dns.rrset.RRset:
        owner_hash = nsec3_hash(owner)
        for index, entry in enumerate(self.entries):
            next_hash = self.entries[(index + 1) % len(self.entries)].owner_hash
            if entry.owner_hash < next_hash:
                if entry.owner_hash < owner_hash < next_hash:
                    return self.rrset_for(entry.owner)
            elif owner_hash > entry.owner_hash or owner_hash < next_hash:
                return self.rrset_for(entry.owner)
        return self.rrset_for(self.entries[-1].owner)


def add_signed_nsec3(
    section: list[dns.rrset.RRset],
    chain: Nsec3Chain,
    owner: str,
    signer: Key,
) -> None:
    add_signed(section, chain.rrset_for(owner), signer)


def add_nsec3_nxdomain(
    response: dns.message.Message,
    chain: Nsec3Chain,
    closest: str,
    qname: str,
    signer: Key,
) -> None:
    response.set_rcode(dns.rcode.NXDOMAIN)
    add_signed(response.authority, soa_rrset(F055_ZONE), signer)
    add_signed_nsec3(response.authority, chain, closest, signer)
    add_signed(response.authority, chain.cover_for(qname), signer)
    add_signed(response.authority, chain.cover_for(f"*.{closest}"), signer)


class F055Handler(DomainHandler):
    domains = [F055_ZONE]

    def __init__(self, keys: dict[str, Key]) -> None:
        super().__init__()
        self.keys = keys

        if F055_ZONE not in keys:
            return

        self.zone_key = keys[F055_ZONE]
        self.chain = Nsec3Chain()
        self.zone = name(F055_ZONE)
        self.safe = name(SAFE)
        self.cut = name(CUT)
        self.control = name(CONTROL)
        self.attack = name(ATTACK)
        self.control_attack = name(CONTROL_ATTACK)

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        response = prepare_response(qctx)
        qname = qctx.qname.to_text()

        if qctx.qname == self.zone and qctx.qtype == dns.rdatatype.DNSKEY:
            # Priming, zone DNSKEY
            add_signed(
                response.answer,
                rrset_from_rdata(F055_ZONE, self.zone_key.dnskey),
                self.zone_key,
            )
        elif qctx.qname == self.zone and qctx.qtype == dns.rdatatype.SOA:
            # Priming, zone SOA
            add_signed(response.answer, soa_rrset(F055_ZONE), self.zone_key)
        elif qctx.qname == self.safe and qctx.qtype == dns.rdatatype.A:
            # Safe, good answer
            add_signed(
                response.answer, rrset(SAFE, dns.rdatatype.A, SAFE_A), self.zone_key
            )
        elif qctx.qname == self.cut and qctx.qtype == dns.rdatatype.DS:
            # Forged DS answer
            forged = forged_cut_nsec3()
            response.authority.append(forged)
            response.authority.append(garbage_rrsig(forged, self.zone_key))
            add_signed_nsec3(response.authority, self.chain, CUT, self.zone_key)
            add_signed(response.authority, soa_rrset(F055_ZONE), self.zone_key)
        elif qctx.qname == self.control and qctx.qtype == dns.rdatatype.DS:
            # Good DS answer
            add_signed_nsec3(response.authority, self.chain, CONTROL, self.zone_key)
            add_signed(response.authority, soa_rrset(F055_ZONE), self.zone_key)
        elif qctx.qname in {self.attack, self.control_attack}:
            # Attack query for CUT/CONTROL
            if qctx.qtype == dns.rdatatype.A:
                response.answer.append(rrset(qname, dns.rdatatype.A, FORGED_A))
            elif qctx.qtype == dns.rdatatype.DS:
                closest = CUT if qctx.qname == self.attack else CONTROL
                add_nsec3_nxdomain(
                    response,
                    self.chain,
                    closest,
                    qname,
                    self.zone_key,
                )
            else:
                response.authority.append(soa_rrset(F055_ZONE))
        elif qctx.qname.is_subdomain(self.cut):
            # NXDOMAIN the rest
            add_nsec3_nxdomain(response, self.chain, CUT, qname, self.zone_key)
        elif qctx.qname.is_subdomain(self.control):
            # NXDOMAIN the rest
            add_nsec3_nxdomain(response, self.chain, CONTROL, qname, self.zone_key)
        else:
            # NXDOMAIN the rest
            add_nsec3_nxdomain(response, self.chain, F055_ZONE, qname, self.zone_key)

        yield DnsResponseSend(response, authoritative=True)
