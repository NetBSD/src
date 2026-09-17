"""
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
"""

from collections.abc import AsyncGenerator

import dns.name
import dns.rcode
import dns.rdataclass
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    ControllableAsyncDnsServer,
    DnsResponseSend,
    DomainHandler,
    QnameQtypeHandler,
    QueryContext,
    ResponseHandler,
    StaticResponseHandler,
    ToggleResponsesCommand,
)

SLD = "sld.tld."
NS1 = f"ns1.{SLD}"


def rrset(
    owner: dns.name.Name | str, rdtype: dns.rdatatype.RdataType, rdata: str
) -> dns.rrset.RRset:
    return dns.rrset.from_text(owner, 300, dns.rdataclass.IN, rdtype, rdata)


def a(owner: dns.name.Name | str) -> dns.rrset.RRset:
    return rrset(owner, dns.rdatatype.A, "10.53.0.2")


def aaaa(owner: dns.name.Name | str) -> dns.rrset.RRset:
    return rrset(owner, dns.rdatatype.AAAA, "fd92:7065:b8e:ffff::2")


def ns(owner: dns.name.Name | str) -> dns.rrset.RRset:
    return rrset(owner, dns.rdatatype.NS, NS1)


def soa(owner: dns.name.Name | str) -> dns.rrset.RRset:
    return rrset(owner, dns.rdatatype.SOA, ". . 0 0 0 0 0")


class Ns1AHandler(QnameQtypeHandler, StaticResponseHandler):
    qnames = [NS1]
    qtypes = [dns.rdatatype.A]
    answer = [a(NS1)]
    edns = None


class Ns1AaaaHandler(QnameQtypeHandler, StaticResponseHandler):
    qnames = [NS1]
    qtypes = [dns.rdatatype.AAAA]
    answer = [aaaa(NS1)]
    edns = None


class SldDelegationHandler(DomainHandler):
    """
    Delegate every NS query at or below sld.tld. to ns1.sld.tld., copying the
    owner name from the QNAME.  Together with Ns1AHandler / Ns1AaaaHandler
    (which resolve the delegated nameserver) and NegativeSoaHandler (the
    negative catch-all), this drives named's DS-chasing logic.
    """

    domains = [SLD]

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qtype == dns.rdatatype.NS and super().match(qctx)

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.response.use_edns(None)
        qctx.response.answer.append(ns(qctx.qname))
        yield DnsResponseSend(qctx.response)


class NegativeSoaHandler(ResponseHandler):
    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.response.use_edns(None)
        qctx.response.authority.append(soa(qctx.qname))
        yield DnsResponseSend(qctx.response)


def main() -> None:
    server = ControllableAsyncDnsServer(
        default_rcode=dns.rcode.NOERROR, default_aa=True
    )
    server.install_control_command(ToggleResponsesCommand())
    server.install_response_handlers(
        Ns1AHandler(),
        Ns1AaaaHandler(),
        SldDelegationHandler(),
        NegativeSoaHandler(),
    )
    server.run()


if __name__ == "__main__":
    main()
