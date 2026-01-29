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

import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    ControllableAsyncDnsServer,
    DnsResponseSend,
    DomainHandler,
    QueryContext,
    ResponseAction,
    ToggleResponsesCommand,
)


class ExtraAnswersHandler(DomainHandler):
    """
    Answer from zone data, inserting extra RRsets into responses to A queries.
    """

    domains = ["attackSecureDomain.net3."]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        if qctx.qtype == dns.rdatatype.A:
            ns_rrset = dns.rrset.from_text(
                "net3.", 300, qctx.qclass, dns.rdatatype.NS, "local.net3."
            )
            qctx.response.answer.append(ns_rrset)
            a_rrset = dns.rrset.from_text(
                "local.net3.", 300, qctx.qclass, dns.rdatatype.A, "10.53.0.11"
            )
            qctx.response.additional.append(a_rrset)

        yield DnsResponseSend(qctx.response)


def main() -> None:
    server = ControllableAsyncDnsServer()
    server.install_control_command(ToggleResponsesCommand())
    server.install_response_handler(ExtraAnswersHandler())
    server.run()


if __name__ == "__main__":
    main()
