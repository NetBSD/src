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

import dns.rcode
import dns.rdatatype

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsResponseSend,
    QueryContext,
    ResponseHandler,
)

from ..resolver_ans import rrset, soa_rrset


class EdnsWithOptionsFormerrHandler(ResponseHandler):
    def match(self, qctx: QueryContext) -> bool:
        return qctx.query.edns > -1 and qctx.query.options

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.response.set_rcode(dns.rcode.FORMERR)
        # The test requires that the server echoes back the client cookie
        qctx.response.opt = qctx.query.opt
        yield DnsResponseSend(qctx.response, authoritative=False)


class FallbackHandler(ResponseHandler):
    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        if qctx.qtype == dns.rdatatype.A:
            a_rrset = rrset(qctx.qname, dns.rdatatype.A, "10.53.0.10")
            qctx.response.answer.append(a_rrset)
        elif qctx.qtype == dns.rdatatype.NS:
            ns_rrset = rrset(qctx.qname, dns.rdatatype.NS, ".")
            qctx.response.answer.append(ns_rrset)
        elif qctx.qtype == dns.rdatatype.SOA:
            qctx.response.answer.append(soa_rrset(qctx.qname))
        else:
            qctx.response.authority.append(soa_rrset(qctx.qname))

        yield DnsResponseSend(qctx.response, authoritative=True)


def main() -> None:
    server = AsyncDnsServer(default_rcode=dns.rcode.NOERROR)
    server.install_response_handlers(
        EdnsWithOptionsFormerrHandler(),
        FallbackHandler(),
    )
    server.run()


if __name__ == "__main__":
    main()
