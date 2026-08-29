#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0

from collections.abc import AsyncGenerator

import dns.flags
import dns.message
import dns.rcode
import dns.rdatatype

from dnssec_wildcard.ans1.common import (
    Key,
    add_dnskey,
    add_signed,
    name,
    rrset,
    rrset_from_rdata,
    soa_rrset,
    wildcard_rrsig,
)
from isctest.asyncserver import DnsResponseSend, DomainHandler, QueryContext

TTL = 300
FORGED_A = "198.51.100.45"
LEGIT_A = "192.0.2.113"


def add_ds(response: dns.message.Message, zone: str, child: Key, parent: Key) -> None:
    add_signed(response.answer, rrset_from_rdata(zone, child.ds), parent)


def add_nodata(response: dns.message.Message, zone: str, key: Key) -> None:
    add_signed(response.authority, soa_rrset(zone), key)


# A signed zone P, chained to a configured trust anchor, that
# (a) publishes a wildcard *.P A or AAAA record and
# (b) delegates at least one separately-signed child C.P (own DS in P)
#     operated by a distinct principal.
F045_PARENT = "f045.test."
F045_CHILD = f"child.{F045_PARENT}"
F045_QUERY = f"q.{F045_PARENT}"
F045_SERVICE = f"svc.{F045_CHILD}"


def add_parent_mx_with_forged_additional(
    response: dns.message.Message, parent: Key
) -> None:
    add_signed(
        response.answer,
        rrset(F045_QUERY, dns.rdatatype.MX, f"10 {F045_SERVICE}"),
        parent,
    )
    response.additional.append(rrset(F045_SERVICE, dns.rdatatype.A, FORGED_A))
    response.additional.append(wildcard_rrsig(F045_SERVICE, FORGED_A, parent))


def add_child_a(response: dns.message.Message, child: Key) -> None:
    add_signed(response.answer, rrset(F045_SERVICE, dns.rdatatype.A, LEGIT_A), child)


class F045Handler(DomainHandler):
    domains = [F045_PARENT, F045_CHILD]

    def __init__(self, keys: dict[str, Key]) -> None:
        super().__init__()
        self.keys = keys

        if F045_PARENT not in keys or F045_CHILD not in keys:
            return

        self.parent = name(F045_PARENT)
        self.child = name(F045_CHILD)
        self.query = name(F045_QUERY)
        self.service = name(F045_SERVICE)

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.prepare_new_response(with_zone_data=False)
        qctx.response.flags |= dns.flags.AA
        qctx.response.set_rcode(dns.rcode.NOERROR)

        parent_key = self.keys[F045_PARENT]
        child_key = self.keys[F045_CHILD]

        if qctx.qname == self.parent and qctx.qtype == dns.rdatatype.DNSKEY:
            # Priming, DNSKEY RRset
            add_dnskey(qctx.response, F045_PARENT, parent_key)
        elif qctx.qname == self.parent and qctx.qtype == dns.rdatatype.SOA:
            # Priming, SOA RRset
            add_signed(qctx.response.answer, soa_rrset(F045_PARENT), parent_key)
        elif qctx.qname == self.query and qctx.qtype == dns.rdatatype.MX:
            # Trigger query.
            add_parent_mx_with_forged_additional(qctx.response, parent_key)
        elif qctx.qname == self.child and qctx.qtype == dns.rdatatype.DS:
            # Chain of trust, DS of child.
            add_ds(qctx.response, F045_CHILD, child_key, parent_key)
        elif qctx.qname == self.child and qctx.qtype == dns.rdatatype.DNSKEY:
            # Chain of trust, DNSKEY of child.
            add_dnskey(qctx.response, F045_CHILD, child_key)
        elif qctx.qname == self.child and qctx.qtype == dns.rdatatype.SOA:
            # SOA of child.
            add_signed(qctx.response.answer, soa_rrset(F045_CHILD), child_key)
        elif qctx.qname == self.service and qctx.qtype == dns.rdatatype.A:
            # Zone data at child.
            add_child_a(qctx.response, child_key)
        elif qctx.qname.is_subdomain(self.child):
            # No data at child.
            add_nodata(qctx.response, F045_CHILD, child_key)
        elif qctx.qname.is_subdomain(self.parent):
            # No data at parent.
            add_nodata(qctx.response, F045_PARENT, parent_key)
        else:
            qctx.response.set_rcode(dns.rcode.NXDOMAIN)

        yield DnsResponseSend(qctx.response, authoritative=True)
