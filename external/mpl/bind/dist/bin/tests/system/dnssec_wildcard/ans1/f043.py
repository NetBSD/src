#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0

from collections.abc import AsyncGenerator

import dns.flags
import dns.rcode
import dns.rdatatype
import dns.rrset

from dnssec_wildcard.ans1.common import (
    Key,
    add_dnskey,
    add_signed,
    name,
    rrset,
    soa_rrset,
    wildcard_rrsig,
)
from isctest.asyncserver import DnsResponseSend, DomainHandler, QueryContext

F043_ZONE = "f043.test."
F043_QUERY = f"svc.{F043_ZONE}"
F043_VICTIM = f"victim.{F043_ZONE}"

FORGED_A = "198.51.100.45"
LEGIT_A = "192.0.2.113"


def add_wildcard_a(section: list[dns.rrset.RRset], owner: str, key: Key) -> None:
    section.append(rrset(owner, dns.rdatatype.A, FORGED_A))
    section.append(wildcard_rrsig(owner, FORGED_A, key))


class F043Handler(DomainHandler):  # Additional from wildcard
    domains = [F043_ZONE]

    def __init__(self, keys: dict[str, Key]) -> None:
        super().__init__()
        self.keys = keys

        if F043_ZONE not in keys:
            return

        self.key = keys[F043_ZONE]
        self.zone = name(F043_ZONE)
        self.query = name(F043_QUERY)
        self.victim = name(F043_VICTIM)

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.prepare_new_response(with_zone_data=False)
        qctx.response.flags |= dns.flags.AA
        qctx.response.set_rcode(dns.rcode.NOERROR)

        if qctx.qname == self.zone and qctx.qtype == dns.rdatatype.DNSKEY:
            add_dnskey(qctx.response, self.zone, self.key)
        elif qctx.qname == self.zone and qctx.qtype == dns.rdatatype.SOA:
            add_signed(qctx.response.answer, soa_rrset(self.zone), self.key)
        elif qctx.qname == self.query and qctx.qtype == dns.rdatatype.MX:
            add_signed(
                qctx.response.answer,
                rrset(F043_QUERY, dns.rdatatype.MX, f"10 {F043_VICTIM}"),
                self.key,
            )
            add_wildcard_a(qctx.response.additional, F043_VICTIM, self.key)
        elif qctx.qname == self.victim and qctx.qtype == dns.rdatatype.A:
            add_signed(
                qctx.response.answer,
                rrset(F043_VICTIM, dns.rdatatype.A, LEGIT_A),
                self.key,
            )
        else:
            add_signed(qctx.response.authority, soa_rrset(self.zone), self.key)

        yield DnsResponseSend(qctx.response, authoritative=True)
