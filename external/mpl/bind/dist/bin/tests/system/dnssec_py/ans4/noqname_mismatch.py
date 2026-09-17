#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0

"""
Forged wildcard answers whose NOQNAME proof owner carries both denial types
(NSEC and NSEC3), in the wire orders that made the resolver's
dns_rdataset_addnoqname() and dns_rdataset_getnoqname() disagree about
which proof to cache (#5985, #6369).
"""

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

# Signed NSEC followed by an unsigned NSEC3 at the proof owner (#5985).
ATTACK = f"www.{CHILD}"
# Signed NSEC3 followed by an unsigned NSEC at the proof owner (#6369).
ATTACK_NSEC3 = f"nsec3.{CHILD}"
# Both denial types signed; only the NSEC3 (second in wire order) covers.
ATTACK_BOTH = f"both.{CHILD}"

# The forged answers claim to be synthesized from *.evil.f217.test.
WILDCARD_LABELS = len(dns.name.from_text(CHILD).labels) - 1

# Not a valid NSEC3 hash label, so only the NSEC is usable at this owner.
NSEC_OWNER = f"00000000.{CHILD}"
NSEC_NEXT = f"zzz.{CHILD}"
# A valid (all zero) NSEC3 hash label; the NSEC3 covers every hashed name.
NSEC3_OWNER = f"{'0' * 32}.{CHILD}"
NSEC3_NEXT = "V" * 32
# An NSEC at NSEC3_OWNER with this next name covers none of the qnames.
NSEC_NONCOVERING_NEXT = f"{'0' * 31}1.{CHILD}"

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


def owner_labels(owner: str) -> int:
    """The RRSIG labels field of a non-wildcard record at 'owner'."""
    return len(name(owner).labels) - 1


def add_ds_denial(response: dns.message.Message, key: Key) -> None:
    add_signed(response.authority, soa_rrset(ZONE), key)
    nsec = rrset(CHILD, dns.rdatatype.NSEC, f"ns.{ZONE} NS RRSIG NSEC")
    add_signed(response.authority, nsec, key)


def add_forged_answer(response: dns.message.Message, qname: str) -> None:
    """
    A forged A record at 'qname' with a garbage RRSIG whose labels field
    is that of *.evil.f217.test., so the resolver treats the answer as a
    wildcard expansion and looks for a NOQNAME proof (findnoqname()).

        <qname>  300 IN A     192.0.2.217
        <qname>  300 IN RRSIG A 13 3 300 <exp> <inc> 12345 evil.f217.test. <64 x 0x00>
    """
    response.answer.append(rrset(qname, dns.rdatatype.A, FORGED_A))
    response.answer.append(
        garbage_rrsig(qname, dns.rdatatype.A, WILDCARD_LABELS, CHILD)
    )


def nsec_rrset(owner: str, next_name: str) -> dns.rrset.RRset:
    return rrset(owner, dns.rdatatype.NSEC, f"{next_name} A RRSIG NSEC")


def nsec_rrsig(owner: str) -> dns.rrset.RRset:
    return garbage_rrsig(owner, dns.rdatatype.NSEC, owner_labels(owner), CHILD)


def nsec3_rrset(owner: str) -> dns.rrset.RRset:
    return rrset(owner, dns.rdatatype.NSEC3, f"1 0 0 - {NSEC3_NEXT} A RRSIG")


def nsec3_rrsig(owner: str) -> dns.rrset.RRset:
    return garbage_rrsig(owner, dns.rdatatype.NSEC3, owner_labels(owner), CHILD)


def add_attack_answer(response: dns.message.Message) -> None:
    """
    www.evil.f217.test./A (#5985): the proof owner carries a signed NSEC
    followed by an unsigned NSEC3.  The NSEC3 owner label is not a valid
    hash, so findnoqname() selects the NSEC.

        ;; AUTHORITY (single owner, three rdatasets in this wire order)
        00000000.evil.f217.test.  300 IN NSEC  zzz.evil.f217.test. A RRSIG NSEC
        00000000.evil.f217.test.  300 IN RRSIG NSEC 13 4 300 <exp> <inc> 12345 evil.f217.test. <64 x 0x00>
        00000000.evil.f217.test.  300 IN NSEC3 1 0 0 - VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV A RRSIG
    """
    add_forged_answer(response, ATTACK)
    response.authority.append(nsec_rrset(NSEC_OWNER, NSEC_NEXT))
    response.authority.append(nsec_rrsig(NSEC_OWNER))
    response.authority.append(nsec3_rrset(NSEC_OWNER))


def add_nsec3_attack_answer(response: dns.message.Message) -> None:
    """
    nsec3.evil.f217.test./A (#6369): the proof owner carries a signed NSEC3
    covering the hashed qname, followed by an unsigned NSEC that does not
    cover the qname, so findnoqname() selects the NSEC3.

        ;; AUTHORITY (single owner, three rdatasets in this wire order)
        <32 x 0>.evil.f217.test.  300 IN NSEC3 1 0 0 - VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV A RRSIG
        <32 x 0>.evil.f217.test.  300 IN RRSIG NSEC3 13 4 300 <exp> <inc> 12345 evil.f217.test. <64 x 0x00>
        <32 x 0>.evil.f217.test.  300 IN NSEC  <31 x 0>1.evil.f217.test. A RRSIG NSEC
    """
    add_forged_answer(response, ATTACK_NSEC3)
    response.authority.append(nsec3_rrset(NSEC3_OWNER))
    response.authority.append(nsec3_rrsig(NSEC3_OWNER))
    response.authority.append(nsec_rrset(NSEC3_OWNER, NSEC_NONCOVERING_NEXT))


def add_both_attack_answer(response: dns.message.Message) -> None:
    """
    both.evil.f217.test./A: both denial types are signed at the proof
    owner.  The NSEC comes first in wire order but does not cover the
    qname; the NSEC3 does, so findnoqname() selects the NSEC3 and the
    cache must keep that choice rather than the first signed pair.

        ;; AUTHORITY (single owner, four rdatasets in this wire order)
        <32 x 0>.evil.f217.test.  300 IN NSEC  <31 x 0>1.evil.f217.test. A RRSIG NSEC
        <32 x 0>.evil.f217.test.  300 IN RRSIG NSEC 13 4 300 <exp> <inc> 12345 evil.f217.test. <64 x 0x00>
        <32 x 0>.evil.f217.test.  300 IN NSEC3 1 0 0 - VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV A RRSIG
        <32 x 0>.evil.f217.test.  300 IN RRSIG NSEC3 13 4 300 <exp> <inc> 12345 evil.f217.test. <64 x 0x00>
    """
    add_forged_answer(response, ATTACK_BOTH)
    response.authority.append(nsec_rrset(NSEC3_OWNER, NSEC_NONCOVERING_NEXT))
    response.authority.append(nsec_rrsig(NSEC3_OWNER))
    response.authority.append(nsec3_rrset(NSEC3_OWNER))
    response.authority.append(nsec3_rrsig(NSEC3_OWNER))


class RuntimeCheckHandler(DomainHandler):
    """Serve f217.test. and the forged wildcard answers below evil.f217.test."""

    domains = [ZONE]

    def __init__(self) -> None:
        super().__init__()
        self.key = load_key()
        self.zone = name(ZONE)
        self.child = name(CHILD)
        self.attacks = {
            name(ATTACK): add_attack_answer,
            name(ATTACK_NSEC3): add_nsec3_attack_answer,
            name(ATTACK_BOTH): add_both_attack_answer,
        }

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
        elif qctx.qname in self.attacks and qctx.qtype == dns.rdatatype.A:
            self.attacks[qctx.qname](qctx.response)
        else:
            add_signed(qctx.response.authority, soa_rrset(ZONE), self.key)

        yield DnsResponseSend(qctx.response, authoritative=True)
