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

import dns.rcode
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsResponseSend,
    QueryContext,
    ResponseDrop,
    ResponseHandler,
)


class ReplyA(ResponseHandler):
    def match(self, qctx: QueryContext) -> bool:
        return qctx.qtype == dns.rdatatype.A

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        a_rrset = dns.rrset.from_text(
            qctx.qname, 300, qctx.qclass, dns.rdatatype.A, "10.53.0.5"
        )
        qctx.response.answer.append(a_rrset)
        yield DnsResponseSend(qctx.response)


class IgnoreNs(ResponseHandler):
    def match(self, qctx: QueryContext) -> bool:
        return qctx.qtype == dns.rdatatype.NS

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseDrop, None]:
        yield ResponseDrop()


def main() -> None:
    server = AsyncDnsServer(default_aa=True, default_rcode=dns.rcode.NOERROR)
    server.install_response_handlers(ReplyA(), IgnoreNs())
    server.run()


if __name__ == "__main__":
    main()
