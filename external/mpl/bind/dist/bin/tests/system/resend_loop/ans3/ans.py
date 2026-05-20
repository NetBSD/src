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

import dns.edns
import dns.name
import dns.rcode
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsResponseSend,
    QueryContext,
    ResponseHandler,
)


def _get_cookie(qctx: QueryContext):
    for o in qctx.query.options:
        if o.otype == dns.edns.OptionType.COOKIE:
            cookie = o
            try:
                if len(cookie.server) == 0:
                    cookie.server = b"\x11\x22\x33\x44\x55\x66\x77\x88"
            except AttributeError:  # dnspython<2.7.0 compat
                if len(o.data) == 8:
                    cookie.data *= 2

            return cookie

    return None


class PrimeHandler(ResponseHandler):
    """
    Specifically handle priming query for "." NS (type 2)
    """

    def match(self, qctx: QueryContext) -> bool:
        return len(qctx.qname.labels) == 0 and qctx.qtype == dns.rdatatype.NS

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:

        ns_rrset = dns.rrset.from_text(
            ".", dns.rdatatype.NS, qctx.qclass, "a.root-servers.nil."
        )
        a_rrset = dns.rrset.from_text(
            "a.root-servers.nil.", dns.rdatatype.A, qctx.qclass, "10.53.0.3"
        )

        response = qctx.prepare_new_response(with_zone_data=False)
        response.set_rcode(dns.rcode.NOERROR)
        response.answer.append(ns_rrset)
        response.additional.append(a_rrset)

        yield DnsResponseSend(response, authoritative=True)


class CookieHandler(ResponseHandler):
    def match(self, qctx: QueryContext) -> bool:
        example = dns.name.from_text("example")
        return qctx.qname.is_subdomain(example)

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:

        qctx.prepare_new_response()

        # Check for client cookie
        cookie = _get_cookie(qctx)

        # If missing cookie entirely, just return SERVFAIL
        if cookie is None:
            qctx.response.set_rcode(dns.rcode.SERVFAIL)
            yield DnsResponseSend(qctx.response, authoritative=True)

        # If there is a client cookie, mock BADCOOKIE to trigger
        # the resend loop logic.
        qctx.response.use_edns(options=[cookie])
        qctx.response.set_rcode(dns.rcode.BADCOOKIE)
        yield DnsResponseSend(qctx.response, authoritative=True)


class NoErrorHandler(ResponseHandler):
    """
    If the query is NOT a subdomain of example, respond with standard NOERROR empty answer
    """

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:

        qctx.prepare_new_response()
        qctx.response.set_rcode(dns.rcode.NOERROR)
        yield DnsResponseSend(qctx.response, authoritative=True)


def resend_server() -> AsyncDnsServer:
    server = AsyncDnsServer(default_aa=True, default_rcode=dns.rcode.NOERROR)
    server.install_response_handlers(
        PrimeHandler(),
        CookieHandler(),
        NoErrorHandler(),
    )
    return server


def main() -> None:
    resend_server().run()


if __name__ == "__main__":
    main()
