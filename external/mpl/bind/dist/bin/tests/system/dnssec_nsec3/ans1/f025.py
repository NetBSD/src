#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0

from collections.abc import AsyncGenerator
from dataclasses import dataclass

import base64

import dns.dnssec
import dns.flags
import dns.message
import dns.rcode
import dns.rdatatype
import dns.rrset

from dnssec_nsec3.ans1.common import (
    Key,
    add_signed,
    name,
    nsec3_hash,
    nsec3_rrset,
    rrset,
    rrset_from_rdata,
    soa_rrset,
)
from isctest.asyncserver import DnsResponseSend, DomainHandler, QueryContext

TTL = 300
PARENT = "f025.test."
CHILD = f"evil.{PARENT}"
PARENT_NS = f"ns.{PARENT}"
CHILD_NS = f"ns.{CHILD}"
CLOSEST = f"victim2.{CHILD}"
ATTACK = f"b.{CLOSEST}"
LEGIT = f"legit.{CHILD}"
WILDCARD = f"*.{CHILD}"
FORGED_A = "6.6.6.6"


@dataclass(frozen=True)
class Nsec3Entry:
    owner: str
    owner_hash: str
    types: tuple[str, ...]


def base32hex_add(hash_text: str, delta: int) -> str:
    raw = bytearray(base64.b32hexdecode(hash_text.upper()))
    value = int.from_bytes(raw, "big") + delta
    value %= 1 << (8 * len(raw))
    return base64.b32hexencode(value.to_bytes(len(raw), "big")).decode("ascii")


class Nsec3Chain:
    def __init__(self, zone: str, entries: list[tuple[str, tuple[str, ...]]]) -> None:
        self.zone = zone
        self.entries = sorted(
            [Nsec3Entry(owner, nsec3_hash(owner), types) for owner, types in entries],
            key=lambda entry: entry.owner_hash,
        )

    def rrset_for_entry(self, entry: Nsec3Entry) -> dns.rrset.RRset:
        index = self.entries.index(entry)
        next_hash = self.entries[(index + 1) % len(self.entries)].owner_hash
        return nsec3_rrset(self.zone, entry.owner_hash, next_hash, 0, *entry.types)

    def rrsets(self) -> list[dns.rrset.RRset]:
        return [self.rrset_for_entry(entry) for entry in self.entries]


def add_nsec3_chain(
    section: list[dns.rrset.RRset], chain: Nsec3Chain, signer: Key
) -> None:
    for covered in chain.rrsets():
        add_signed(section, covered, signer)


def add_tight_parent_nsec3(section: list[dns.rrset.RRset], parent: Key) -> None:
    target_hash = nsec3_hash(f"{CLOSEST}")
    covered = nsec3_rrset(
        PARENT,
        base32hex_add(target_hash, -1),
        base32hex_add(target_hash, 1),
        0,
        "TXT",
        "RRSIG",
    )
    add_signed(section, covered, parent)


def wildcard_rrsig(owner: str, child: Key) -> dns.rrset.RRset:
    wildcard = rrset(WILDCARD, dns.rdatatype.A, FORGED_A)
    rrsig = dns.dnssec.sign(
        wildcard,
        child.private_key,
        child.zone,
        child.dnskey,
        lifetime=86400,
        verify=True,
    )
    return dns.rrset.from_rdata(name(owner), wildcard.ttl, rrsig)


def add_wildcard_answer(response: dns.message.Message, owner: str, child: Key) -> None:
    response.answer.append(rrset(owner, dns.rdatatype.A, FORGED_A))
    response.answer.append(wildcard_rrsig(owner, child))


class F025Handler(DomainHandler):
    domains = [PARENT, CHILD]

    def __init__(self, keys: dict[str, Key]) -> None:
        super().__init__()
        self.keys = keys

        self.parent = name(PARENT)
        self.child = name(CHILD)
        self.parent_ns = name(PARENT_NS)
        self.child_ns = name(CHILD_NS)
        self.child_nsec3 = Nsec3Chain(
            CHILD,
            [
                (CHILD, ("NS", "SOA", "RRSIG", "DNSKEY", "NSEC3PARAM")),
                (WILDCARD, ("A", "RRSIG")),
                (CHILD_NS, ("A", "RRSIG")),
            ],
        )

    def _add_extra_nsec3(self, response: dns.message.Message, qname: str) -> None:
        parent_key = self.keys[PARENT]
        child_key = self.keys[CHILD]
        if "victim2." in qname:
            add_tight_parent_nsec3(response.authority, parent_key)
        else:
            add_nsec3_chain(response.authority, self.child_nsec3, child_key)

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.prepare_new_response(with_zone_data=False)
        qctx.response.flags |= dns.flags.AA
        qctx.response.set_rcode(dns.rcode.NOERROR)

        parent_key = self.keys[PARENT]
        child_key = self.keys[CHILD]
        qname = qctx.qname.to_text()

        if qctx.qname == self.parent and qctx.qtype == dns.rdatatype.DNSKEY:
            # Priming, parent DNSKEY
            add_signed(
                qctx.response.answer,
                rrset_from_rdata(PARENT, parent_key.dnskey),
                parent_key,
            )
        elif qctx.qname == self.parent and qctx.qtype == dns.rdatatype.SOA:
            # Priming, parent SOA
            add_signed(qctx.response.answer, soa_rrset(PARENT), parent_key)
        elif qctx.qname == self.parent and qctx.qtype == dns.rdatatype.NS:
            # Priming, parent NS
            add_signed(
                qctx.response.answer,
                rrset(PARENT, dns.rdatatype.NS, PARENT_NS),
                parent_key,
            )
        elif qctx.qname == self.parent_ns and qctx.qtype == dns.rdatatype.A:
            # Priming, parent glue
            add_signed(
                qctx.response.answer,
                rrset(PARENT_NS, dns.rdatatype.A, "10.53.0.1"),
                parent_key,
            )
        elif qctx.qname == self.child and qctx.qtype == dns.rdatatype.DS:
            # Priming, child DS
            add_signed(
                qctx.response.answer,
                rrset_from_rdata(CHILD, child_key.ds),
                parent_key,
            )
        elif qctx.qname == self.child and qctx.qtype == dns.rdatatype.DNSKEY:
            # Priming, child DNSKEY
            add_signed(
                qctx.response.answer,
                rrset_from_rdata(CHILD, child_key.dnskey),
                child_key,
            )
        elif qctx.qname == self.child and qctx.qtype == dns.rdatatype.SOA:
            # Priming, child SOA
            add_signed(qctx.response.answer, soa_rrset(CHILD), child_key)
        elif qctx.qname == self.child and qctx.qtype == dns.rdatatype.NS:
            # Priming, child NS
            add_signed(
                qctx.response.answer,
                rrset(CHILD, dns.rdatatype.NS, CHILD_NS),
                child_key,
            )
        elif qctx.qname == self.child_ns and qctx.qtype == dns.rdatatype.A:
            # Priming, child glue
            add_signed(
                qctx.response.answer,
                rrset(CHILD_NS, dns.rdatatype.A, "10.53.0.1"),
                child_key,
            )
        elif qctx.qname.is_subdomain(self.child):
            if qctx.qtype == dns.rdatatype.A:
                add_wildcard_answer(qctx.response, qname, child_key)
            else:
                add_signed(qctx.response.authority, soa_rrset(CHILD), child_key)
            # Adding malicious NSEC3
            self._add_extra_nsec3(qctx.response, qname)
        else:
            # Everything else is NODATA
            add_signed(qctx.response.authority, soa_rrset(PARENT), parent_key)

        yield DnsResponseSend(qctx.response, authoritative=True)
