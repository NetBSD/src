"""
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
"""

from typing import AsyncGenerator

import dns.rcode
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsResponseSend,
    DomainHandler,
    IgnoreAllQueries,
    QnameHandler,
    QueryContext,
    ResponseHandler,
)


def setup_delegation(qctx: QueryContext, owner: str) -> None:
    ns_name = f"ns.{owner}"
    ns_rrset = dns.rrset.from_text(owner, 300, qctx.qclass, dns.rdatatype.NS, ns_name)
    a_rrset = dns.rrset.from_text(
        ns_name, 300, qctx.qclass, dns.rdatatype.A, "10.53.0.3"
    )
    qctx.response.authority.append(ns_rrset)
    qctx.response.additional.append(a_rrset)


class BadGoodCnameHandler(QnameHandler):
    qnames = [
        "badcname.example.net.",
        "goodcname.example.net.",
    ]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        # Data for CNAME/DNAME filtering.  We need to make one-level
        # delegation to avoid automatic acceptance for subdomain aliases
        setup_delegation(qctx, "example.net.")
        yield DnsResponseSend(qctx.response, authoritative=False)


class Cname1Handler(QnameHandler):
    qnames = ["cname1.example.com."]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        # Data for the "cname + other data / 1" test
        cname_rrset = dns.rrset.from_text(
            qctx.qname, 300, qctx.qclass, dns.rdatatype.CNAME, "cname1.example.com."
        )
        a_rrset = dns.rrset.from_text(
            qctx.qname, 300, qctx.qclass, dns.rdatatype.A, "1.2.3.4"
        )
        qctx.response.answer.append(cname_rrset)
        qctx.response.answer.append(a_rrset)
        yield DnsResponseSend(qctx.response, authoritative=False)


class Cname2Handler(QnameHandler):
    qnames = ["cname2.example.com."]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        # Data for the "cname + other data / 2" test: same RRs in opposite order
        a_rrset = dns.rrset.from_text(
            qctx.qname, 300, qctx.qclass, dns.rdatatype.A, "1.2.3.4"
        )
        cname_rrset = dns.rrset.from_text(
            qctx.qname, 300, qctx.qclass, dns.rdatatype.CNAME, "cname2.example.com."
        )
        qctx.response.answer.append(a_rrset)
        qctx.response.answer.append(cname_rrset)
        yield DnsResponseSend(qctx.response, authoritative=False)


class ExampleHandler(QnameHandler):
    qnames = [
        "www.example.com.",
        "www.example.net.",
        "badcname.example.org.",
        "goodcname.example.org.",
        "foo.badcname.example.org.",
        "foo.goodcname.example.org.",
    ]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        # Data for address/alias filtering.
        if qctx.qtype == dns.rdatatype.A:
            a_rrset = dns.rrset.from_text(
                qctx.qname, 300, qctx.qclass, qctx.qtype, "192.0.2.1"
            )
            qctx.response.answer.append(a_rrset)
        elif qctx.qtype == dns.rdatatype.AAAA:
            aaaa_rrset = dns.rrset.from_text(
                qctx.qname, 300, qctx.qclass, qctx.qtype, "2001:db8:beef::1"
            )
            qctx.response.answer.append(aaaa_rrset)
        yield DnsResponseSend(qctx.response, authoritative=True)


class FooInfoHandler(QnameHandler, IgnoreAllQueries):
    qnames = ["foo.info."]


class NoDataHandler(DomainHandler):
    domains = ["nodata.example.net."]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        yield DnsResponseSend(qctx.response, authoritative=True)


class NxdomainHandler(DomainHandler):
    domains = ["nxdomain.example.net."]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.response.set_rcode(dns.rcode.NXDOMAIN)
        yield DnsResponseSend(qctx.response, authoritative=True)


class SubHandler(DomainHandler):
    domains = ["sub.example.org."]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        # Data for CNAME/DNAME filtering.  The final answers are
        # expected to be accepted regardless of the filter setting.
        setup_delegation(qctx, "sub.example.org.")
        yield DnsResponseSend(qctx.response, authoritative=False)


class FallbackHandler(ResponseHandler):
    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        setup_delegation(qctx, "below.www.example.com.")
        yield DnsResponseSend(qctx.response, authoritative=False)


def main() -> None:
    server = AsyncDnsServer(default_rcode=dns.rcode.NOERROR)
    server.install_response_handlers(
        [
            BadGoodCnameHandler(),
            Cname1Handler(),
            Cname2Handler(),
            ExampleHandler(),
            FooInfoHandler(),
            NoDataHandler(),
            NxdomainHandler(),
            SubHandler(),
            FallbackHandler(),
        ]
    )
    server.run()


if __name__ == "__main__":
    main()
