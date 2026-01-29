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

import dns.message
import dns.name
import dns.rcode
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsResponseSend,
    DomainHandler,
    QueryContext,
    ResponseAction,
)

from qmin_ans import (
    DelayedResponseHandler,
    EntRcodeChanger,
    QueryLogHandler,
    log_query,
)


class QueryLogger(QueryLogHandler):
    domains = ["1.0.0.2.ip6.arpa.", "fwd.", "good."]


class BadHandler(EntRcodeChanger):
    domains = ["bad."]
    rcode = dns.rcode.NXDOMAIN


class UglyHandler(EntRcodeChanger):
    domains = ["ugly."]
    rcode = dns.rcode.FORMERR


class SlowHandler(DelayedResponseHandler):
    domains = ["slow."]
    delay = 0.2


def send_delegation(
    qctx: QueryContext, zone_cut: dns.name.Name, target_addr: str
) -> ResponseAction:
    """
    Delegate `zone_cut` to a single in-bailiwick name server, `ns.<zone_cut>`,
    with a single IPv4 glue record (provided in `target_addr`) included in the
    ADDITIONAL section.
    """
    ns_name = "ns." + zone_cut.to_text()
    ns_rrset = dns.rrset.from_text(zone_cut, 2, qctx.qclass, dns.rdatatype.NS, ns_name)
    a_rrset = dns.rrset.from_text(ns_name, 2, qctx.qclass, dns.rdatatype.A, target_addr)

    response = dns.message.make_response(qctx.query)
    response.set_rcode(dns.rcode.NOERROR)
    response.authority.append(ns_rrset)
    response.additional.append(a_rrset)

    return DnsResponseSend(response, authoritative=False)


class StaleHandler(DomainHandler):
    """
    `a.b.stale` is a subdomain of `b.stale` and these two subdomains need to be
    delegated to different name servers.  Therefore, their delegations cannot
    be placed in the zone file because the zone cut at `b.stale` would occlude
    the one at `a.b.stale`.  Generate these delegations dynamically depending
    on the QNAME.
    """

    domains = ["stale."]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        log_query(qctx)
        a_b_stale = dns.name.from_text("a.b.stale.")
        b_stale = dns.name.from_text("b.stale.")
        if qctx.qname.is_subdomain(a_b_stale):
            yield send_delegation(qctx, a_b_stale, "10.53.0.3")
        elif qctx.qname.is_subdomain(b_stale):
            yield send_delegation(qctx, b_stale, "10.53.0.4")


def main() -> None:
    server = AsyncDnsServer()
    server.install_response_handlers(
        [
            QueryLogger(),
            BadHandler(),
            UglyHandler(),
            SlowHandler(),
            StaleHandler(),
        ]
    )
    server.run()


if __name__ == "__main__":
    main()
