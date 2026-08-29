#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0

from collections.abc import AsyncGenerator
from datetime import datetime, timedelta, timezone

import base64

import dns.message
import dns.rcode
import dns.rdata
import dns.rdataclass
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import DnsResponseSend, DomainHandler, QueryContext
from nsec_synthesis.ans1.common import (
    Key,
    add_signed,
    name,
    prepare_response,
    rrset,
    rrset_from_rdata,
    soa_rrset,
)

TTL = 300
F023_ZONE = "f023.test."
EVIL = f"evil.{F023_ZONE}"
POISON = f"0.{EVIL}"
POISON_NEXT = f"zzz.{F023_ZONE}"
TARGET = f"target.{F023_ZONE}"
TARGET_A = "192.0.2.77"


def nsec_rrset(owner: str, next_name: str, *types: str) -> dns.rrset.RRset:
    return rrset(owner, dns.rdatatype.NSEC, f"{next_name} {' '.join(types)}")


def parent_apex_nsec() -> dns.rrset.RRset:
    return nsec_rrset(F023_ZONE, EVIL, "NS", "SOA", "RRSIG", "NSEC", "DNSKEY")


def evil_delegation_nsec() -> dns.rrset.RRset:
    return nsec_rrset(EVIL, f"ns.{F023_ZONE}", "NS", "RRSIG", "NSEC")


def malicious_nsec() -> dns.rrset.RRset:
    return nsec_rrset(POISON, POISON_NEXT, "A", "RRSIG", "NSEC")


def garbage_rrsig(covered: dns.rrset.RRset) -> dns.rrset.RRset:
    now = datetime.now(timezone.utc)
    inception = (now - timedelta(hours=1)).strftime("%Y%m%d%H%M%S")
    expiration = (now + timedelta(days=1)).strftime("%Y%m%d%H%M%S")
    signature = base64.b64encode(bytes(64)).decode("ascii")
    labels = len(covered.name.labels) - 1
    text = (
        f"{dns.rdatatype.to_text(covered.rdtype)} "
        f"13 {labels} {covered.ttl} {expiration} {inception} "
        f"12345 {F023_ZONE} {signature}"
    )
    rdata = dns.rdata.from_text(dns.rdataclass.IN, dns.rdatatype.RRSIG, text)
    return dns.rrset.from_rdata(covered.name, covered.ttl, rdata)


def add_parent_denial(
    response: dns.message.Message, signer: Key, nsec: dns.rrset.RRset
) -> None:
    add_signed(response.authority, soa_rrset(F023_ZONE), signer)
    add_signed(response.authority, nsec, signer)


class F023Handler(DomainHandler):
    domains = [F023_ZONE, EVIL]

    def __init__(self, keys: dict[str, Key]) -> None:
        super().__init__()
        self.keys = keys

        if F023_ZONE not in keys:
            return

        self.key = keys[F023_ZONE]
        self.parent = name(F023_ZONE)
        self.evil = name(EVIL)
        self.poison = name(POISON)
        self.target = name(TARGET)

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        response = prepare_response(qctx)

        if qctx.qname == self.parent and qctx.qtype == dns.rdatatype.DNSKEY:
            add_signed(
                response.answer,
                rrset_from_rdata(F023_ZONE, self.key.dnskey),
                self.key,
            )
        elif qctx.qname == self.parent and qctx.qtype == dns.rdatatype.SOA:
            add_signed(response.answer, soa_rrset(F023_ZONE), self.key)
        elif qctx.qname == self.parent and qctx.qtype == dns.rdatatype.NSEC:
            add_signed(response.answer, parent_apex_nsec(), self.key)
        elif qctx.qname == self.evil and qctx.qtype == dns.rdatatype.DS:
            add_parent_denial(response, self.key, evil_delegation_nsec())
        elif qctx.qname == self.poison and qctx.qtype == dns.rdatatype.NSEC:
            nsec = malicious_nsec()
            response.answer.append(nsec)
            response.answer.append(garbage_rrsig(nsec))
        elif qctx.qname == self.target and qctx.qtype == dns.rdatatype.A:
            add_signed(
                response.answer,
                rrset(TARGET, dns.rdatatype.A, TARGET_A),
                self.key,
            )
        else:
            response.set_rcode(dns.rcode.NXDOMAIN)
            add_parent_denial(response, self.key, parent_apex_nsec())

        yield DnsResponseSend(response, authoritative=True)
