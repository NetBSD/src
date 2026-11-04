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

import abc

import dns.flags
import dns.message
import dns.rcode
import dns.rdatatype

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsProtocol,
    DnsResponseSend,
    DomainHandler,
    QnameHandler,
    QnameQtypeHandler,
    QueryContext,
    ResponseHandler,
    StaticResponseHandler,
)

from ..resolver_ans import rrset


class HeaderOnlyHandler(ResponseHandler):
    """
    Return an empty DNS message with only header flags set.
    """

    @property
    @abc.abstractmethod
    def flags(self) -> dns.flags.Flag:
        raise NotImplementedError

    @property
    def rcode(self) -> dns.rcode.Rcode:
        return dns.rcode.NOERROR

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        message = dns.message.Message(id=qctx.query.id)
        message.use_edns(False)
        message.flags = self.flags
        message.set_rcode(self.rcode)
        yield DnsResponseSend(message, acknowledge_hand_rolled_response=True)


class RefusedOnTcpHandler(QnameHandler, HeaderOnlyHandler):
    qnames = ["tcpalso.no-questions."]
    flags = dns.flags.QR
    rcode = dns.rcode.REFUSED

    def match(self, qctx: QueryContext) -> bool:
        return qctx.protocol == DnsProtocol.TCP and super().match(qctx)


class TcpFallbackHandler(ResponseHandler):
    def match(self, qctx: QueryContext) -> bool:
        return qctx.protocol == DnsProtocol.TCP

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.response.answer.append(rrset(qctx.qname, dns.rdatatype.A, "1.2.3.4"))
        yield DnsResponseSend(qctx.response)


class FormerrToAllHandler(DomainHandler, StaticResponseHandler):
    domains = ["formerr-to-all."]
    rcode = dns.rcode.FORMERR


class NoQuestionsNSHandler(QnameQtypeHandler, StaticResponseHandler):
    qnames = ["no-questions."]
    qtypes = [dns.rdatatype.NS]
    answer = [rrset(qnames[0], dns.rdatatype.NS, f"ns.{qnames[0]}")]
    additional = [rrset(f"ns.{qnames[0]}", dns.rdatatype.A, "10.53.0.8")]


class NsNoQuestionsAHandler(QnameHandler):
    qnames = ["ns.no-questions."]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        if qctx.qtype == dns.rdatatype.A:
            a_rrset = rrset(qctx.qname, dns.rdatatype.A, "10.53.0.8")
            qctx.response.answer.append(a_rrset)
        yield DnsResponseSend(qctx.response)


class TcpalsoNoQuestionsHandler(QnameHandler, HeaderOnlyHandler):
    qnames = ["tcpalso.no-questions."]
    flags = dns.flags.QR | dns.flags.TC
    rcode = dns.rcode.REFUSED


class TruncatedNoQuestionsHandler(QnameHandler, HeaderOnlyHandler):
    qnames = ["truncated.no-questions."]
    flags = dns.flags.QR | dns.flags.AA | dns.flags.TC


class FallbackHandler(HeaderOnlyHandler):
    flags = dns.flags.QR | dns.flags.AA


def main() -> None:
    server = AsyncDnsServer(default_aa=True, default_rcode=dns.rcode.NOERROR)

    # Install TCP handlers first so they take precedence
    server.install_response_handlers(
        RefusedOnTcpHandler(),
        TcpFallbackHandler(),
    )

    # Install UDP handlers
    server.install_response_handlers(
        FormerrToAllHandler(),
        NoQuestionsNSHandler(),
        NsNoQuestionsAHandler(),
        TcpalsoNoQuestionsHandler(),
        TruncatedNoQuestionsHandler(),
    )
    server.install_response_handler(FallbackHandler())

    server.run()


if __name__ == "__main__":
    main()
