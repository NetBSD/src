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
    IgnoreAllQueries,
    QnameHandler,
    QueryContext,
    ResponseHandler,
)


class FooInfoHandler(QnameHandler, IgnoreAllQueries):
    qnames = ["foo.info."]


class FallbackHandler(ResponseHandler):
    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        name = "below.www.example.com."
        ns_name = f"ns.{name}"
        ns_rrset = dns.rrset.from_text(
            name, 300, qctx.qclass, dns.rdatatype.NS, ns_name
        )
        a_rrset = dns.rrset.from_text(
            ns_name, 300, qctx.qclass, dns.rdatatype.A, "10.53.0.3"
        )
        qctx.response.authority.append(ns_rrset)
        qctx.response.additional.append(a_rrset)
        yield DnsResponseSend(qctx.response)


def main() -> None:
    server = AsyncDnsServer(default_rcode=dns.rcode.NOERROR, default_aa=False)
    server.install_response_handlers(FooInfoHandler(), FallbackHandler())
    server.run()


if __name__ == "__main__":
    main()
