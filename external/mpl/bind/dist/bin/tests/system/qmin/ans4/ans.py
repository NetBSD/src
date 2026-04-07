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

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsResponseSend,
    DomainHandler,
    QueryContext,
    ResponseAction,
)

from ..qmin_ans import (
    DelayedResponseHandler,
    EntRcodeChanger,
    QueryLogHandler,
    log_query,
)


class QueryLogger(QueryLogHandler):
    domains = [
        "1.1.1.1.8.2.6.0.1.0.0.2.ip6.arpa.",
        "icky.ptang.zoop.boing.good.",
    ]


class StaleHandler(DomainHandler):
    """
    The test code relies on this server returning non-minimal (i.e. including
    address records in the ADDITIONAL section) responses to NS queries for
    `b.stale` and `a.b.stale`.  While this logic (returning non-minimal
    responses to NS queries) could be implemented in AsyncDnsServer itself,
    doing so breaks a lot of other checks in this system test.  Therefore, only
    these two zones behave in this particular way, thanks to a custom response
    handler implemented below.
    """

    domains = ["b.stale", "a.b.stale"]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        log_query(qctx)

        if qctx.qtype == dns.rdatatype.NS:
            assert qctx.zone
            assert qctx.response.answer[0]

            for nameserver in qctx.response.answer[0]:
                if not nameserver.target.is_subdomain(qctx.response.answer[0].name):
                    continue
                glue_a = qctx.zone.get_rrset(nameserver.target, dns.rdatatype.A)
                if glue_a:
                    qctx.response.additional.append(glue_a)
                glue_aaaa = qctx.zone.get_rrset(nameserver.target, dns.rdatatype.AAAA)
                if glue_aaaa:
                    qctx.response.additional.append(glue_aaaa)

        yield DnsResponseSend(qctx.response)


class IckyPtangZoopBoingBadHandler(EntRcodeChanger):
    domains = ["icky.ptang.zoop.boing.bad."]
    rcode = dns.rcode.NXDOMAIN


class IckyPtangZoopBoingUglyHandler(EntRcodeChanger):
    domains = ["icky.ptang.zoop.boing.ugly."]
    rcode = dns.rcode.FORMERR


class IckyPtangZoopBoingSlowHandler(DelayedResponseHandler):
    domains = ["icky.ptang.zoop.boing.slow."]
    delay = 0.4


def main() -> None:
    server = AsyncDnsServer()
    server.install_response_handlers(
        QueryLogger(),
        StaleHandler(),
        IckyPtangZoopBoingBadHandler(),
        IckyPtangZoopBoingUglyHandler(),
        IckyPtangZoopBoingSlowHandler(),
    )
    server.run()


if __name__ == "__main__":
    main()
