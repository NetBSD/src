#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0

from collections.abc import AsyncGenerator
from dataclasses import dataclass
from datetime import datetime, timedelta, timezone
from pathlib import Path

import base64

from cryptography.hazmat.primitives import serialization
from dns.rdtypes.dnskeybase import Flag

import dns.dnssec
import dns.flags
import dns.message
import dns.name
import dns.rcode
import dns.rdata
import dns.rdataclass
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import DnsResponseSend, DomainHandler, QueryContext

TTL = 300
ZONE = "f217.test."
PEM_PATH = Path(f"{ZONE}.pem")
CHILD = f"evil.{ZONE}"
ATTACK = f"www.{CHILD}"
NSEC_OWNER = f"00000000.{CHILD}"
NSEC_NEXT = f"zzz.{CHILD}"
FORGED_A = "192.0.2.217"


@dataclass(frozen=True)
class Key:
    zone: dns.name.Name
    private_key: object
    dnskey: dns.rdata.Rdata


def name(text: str) -> dns.name.Name:
    return dns.name.from_text(text)


def load_key() -> Key:
    private_key = serialization.load_pem_private_key(
        PEM_PATH.read_bytes(), password=None
    )

    dnskey = dns.dnssec.make_dnskey(
        private_key.public_key(),
        dns.dnssec.Algorithm.ECDSAP256SHA256,
        flags=Flag.ZONE | Flag.SEP,
    )

    return Key(name(ZONE), private_key, dnskey)


def rrset(owner: str, rdtype: dns.rdatatype.RdataType, *rdatas: str) -> dns.rrset.RRset:
    return dns.rrset.from_text(owner, TTL, dns.rdataclass.IN, rdtype, *rdatas)


def rrset_from_rdata(owner: str, rdata: dns.rdata.Rdata) -> dns.rrset.RRset:
    return dns.rrset.from_rdata(name(owner), TTL, rdata)


def add_signed(
    section: list[dns.rrset.RRset], covered: dns.rrset.RRset, signer: Key
) -> None:
    rrsig = dns.dnssec.sign(
        covered,
        signer.private_key,
        signer.zone,
        signer.dnskey,
        lifetime=86400,
        verify=True,
    )
    section.append(covered)
    section.append(dns.rrset.from_rdata(covered.name, covered.ttl, rrsig))


def soa_rrset(zone: str) -> dns.rrset.RRset:
    return rrset(
        zone,
        dns.rdatatype.SOA,
        f"ns.{ZONE} hostmaster.{ZONE} 1 7200 3600 1209600 300",
    )


def garbage_rrsig(
    owner: str, covered: dns.rdatatype.RdataType, labels: int, signer: str
) -> dns.rrset.RRset:
    now = datetime.now(timezone.utc)
    inception = (now - timedelta(hours=1)).strftime("%Y%m%d%H%M%S")
    expiration = (now + timedelta(days=1)).strftime("%Y%m%d%H%M%S")
    signature = base64.b64encode(bytes(64)).decode("ascii")
    text = (
        f"{dns.rdatatype.to_text(covered)} 13 {labels} {TTL} "
        f"{expiration} {inception} 12345 {signer} {signature}"
    )
    rdata = dns.rdata.from_text(dns.rdataclass.IN, dns.rdatatype.RRSIG, text)
    return dns.rrset.from_rdata(name(owner), TTL, rdata)


def add_ds_denial(response: dns.message.Message, key: Key) -> None:
    add_signed(response.authority, soa_rrset(ZONE), key)
    nsec = rrset(CHILD, dns.rdatatype.NSEC, f"ns.{ZONE} NS RRSIG NSEC")
    add_signed(response.authority, nsec, key)


def add_attack_answer(response: dns.message.Message) -> None:
    """
    Crafted authoritative response to <q>.evil.f217.hack./A

        ;; ANSWER
        <q>.evil.f217.hack.        300 IN A     192.0.2.217
        <q>.evil.f217.hack.        300 IN RRSIG A 13 1 300 <exp> <inc> 12345 evil.f217.hack. <base64 of 64×0x00>
                                                    ^^^ Labels = 1, qname has 4 labels, wildcard heuristic fires

        ;; AUTHORITY (single owner, three rdatasets in this wire order)
        00000000.evil.f217.hack.   300 IN NSEC  zzz.evil.f217.hack. A RRSIG NSEC
        00000000.evil.f217.hack.   300 IN RRSIG NSEC 13 4 300 <exp> <inc> 12345 evil.f217.hack. <base64 of 64×0x00>
        00000000.evil.f217.hack.   300 IN NSEC3 1 0 0 - VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV A RRSIG
    """
    # A + RRSIG
    response.answer.append(rrset(ATTACK, dns.rdatatype.A, FORGED_A))
    response.answer.append(garbage_rrsig(ATTACK, dns.rdatatype.A, 1, CHILD))
    # NSEC
    nsec = rrset(
        NSEC_OWNER,
        dns.rdatatype.NSEC,
        f"{NSEC_NEXT} A RRSIG NSEC",
    )
    response.authority.append(nsec)
    # RRSIG(NSEC)
    response.authority.append(
        garbage_rrsig(
            NSEC_OWNER,
            dns.rdatatype.NSEC,
            len(name(NSEC_OWNER).labels) - 1,
            CHILD,
        )
    )
    # NSEC3
    nsec3 = rrset(
        NSEC_OWNER,
        dns.rdatatype.NSEC3,
        "1 0 0 - VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV A RRSIG",
    )
    response.authority.append(nsec3)


class RuntimeCheckHandler(DomainHandler):
    """Serve attacker.rrsig-labels-signer. with crafted wildcard RRSIG."""

    domains = [ZONE]

    def __init__(self) -> None:
        super().__init__()
        self.key = load_key()
        self.zone = name(ZONE)
        self.child = name(CHILD)
        self.attack = name(ATTACK)

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qname.is_subdomain(self.zone)

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.prepare_new_response(with_zone_data=False)
        qctx.response.flags |= dns.flags.AA
        qctx.response.set_rcode(dns.rcode.NOERROR)

        if qctx.qname == self.zone and qctx.qtype == dns.rdatatype.DNSKEY:
            add_signed(
                qctx.response.answer,
                rrset_from_rdata(ZONE, self.key.dnskey),
                self.key,
            )
        elif qctx.qname == self.zone and qctx.qtype == dns.rdatatype.SOA:
            add_signed(qctx.response.answer, soa_rrset(ZONE), self.key)
        elif qctx.qname == self.child and qctx.qtype == dns.rdatatype.DS:
            add_ds_denial(qctx.response, self.key)
        elif qctx.qname == self.child and qctx.qtype == dns.rdatatype.DNSKEY:
            qctx.response.authority.append(soa_rrset(CHILD))
        elif qctx.qname == self.attack and qctx.qtype == dns.rdatatype.A:
            add_attack_answer(qctx.response)
        else:
            add_signed(qctx.response.authority, soa_rrset(ZONE), self.key)

        yield DnsResponseSend(qctx.response, authoritative=True)
