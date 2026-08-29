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

from collections.abc import AsyncGenerator
from datetime import datetime, timedelta, timezone

import base64

import dns.flags
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
F004_ZONE = "f004.test."
ATTACKER = "attacker.f004.test."
VICTIM = "victim.f004.test."
VICTIM_A = "203.0.113.1"
POISON_NEXT = f"b.{VICTIM}"
VICTIM_NODATA_NEXT = f"z.{F004_ZONE}"


def attacker_nsec_rrset() -> dns.rrset.RRset:
    return rrset(
        ATTACKER,
        dns.rdatatype.NSEC,
        f"{POISON_NEXT} NS SOA RRSIG NSEC DNSKEY",
    )


def victim_nodata_nsec_rrset() -> dns.rrset.RRset:
    # An NSEC owned by the victim name itself, whose type bitmap omits A but
    # includes the NSEC and RRSIG types query_coveringnsec requires. If it
    # were trusted, it would prove a NODATA for victim/A and hide the real
    # A record below.
    return rrset(
        VICTIM,
        dns.rdatatype.NSEC,
        f"{VICTIM_NODATA_NEXT} TXT RRSIG NSEC",
    )


def garbage_rrsig(covered: dns.rrset.RRset, signer: Key) -> dns.rrset.RRset:
    now = datetime.now(timezone.utc)
    inception = (now - timedelta(hours=1)).strftime("%Y%m%d%H%M%S")
    expiration = (now + timedelta(days=1)).strftime("%Y%m%d%H%M%S")
    signature = base64.b64encode(bytes(64)).decode("ascii")
    text = (
        f"{dns.rdatatype.to_text(covered.rdtype)} "
        f"{signer.dnskey.algorithm} 3 {covered.ttl} "
        f"{expiration} {inception} 9999 {F004_ZONE} {signature}"
    )
    rdata = dns.rdata.from_text(dns.rdataclass.IN, dns.rdatatype.RRSIG, text)
    return dns.rrset.from_rdata(covered.name, covered.ttl, rdata)


class F004Handler(DomainHandler):
    domains = [F004_ZONE]

    def __init__(self, keys: dict[str, Key]) -> None:
        super().__init__()
        self.keys = keys

        if F004_ZONE not in keys:
            return

        self.key = keys[F004_ZONE]
        self.parent = name(F004_ZONE)
        self.attacker = name(ATTACKER)
        self.victim = name(VICTIM)

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        response = prepare_response(qctx)

        if qctx.qname == self.parent and qctx.qtype == dns.rdatatype.DNSKEY:
            add_signed(
                response.answer,
                rrset_from_rdata(F004_ZONE, self.key.dnskey),
                self.key,
            )
        elif qctx.qname == self.parent and qctx.qtype == dns.rdatatype.SOA:
            add_signed(response.answer, soa_rrset(F004_ZONE), self.key)
        elif qctx.qname == self.attacker and qctx.qtype == dns.rdatatype.NSEC:
            if qctx.query.flags & dns.flags.CD:
                nsec = attacker_nsec_rrset()
                response.answer.append(nsec)
                response.answer.append(garbage_rrsig(nsec, self.key))
            else:
                response.set_rcode(dns.rcode.REFUSED)
        elif qctx.qname == self.victim and qctx.qtype == dns.rdatatype.NSEC:
            if qctx.query.flags & dns.flags.CD:
                nsec = victim_nodata_nsec_rrset()
                response.answer.append(nsec)
                response.answer.append(garbage_rrsig(nsec, self.key))
            else:
                response.set_rcode(dns.rcode.REFUSED)
        elif qctx.qname == self.victim and qctx.qtype == dns.rdatatype.A:
            add_signed(
                response.answer,
                rrset(VICTIM, dns.rdatatype.A, VICTIM_A),
                self.key,
            )
        else:
            response.set_rcode(dns.rcode.NXDOMAIN)
            add_signed(response.authority, soa_rrset(F004_ZONE), self.key)

        yield DnsResponseSend(response, authoritative=True)
