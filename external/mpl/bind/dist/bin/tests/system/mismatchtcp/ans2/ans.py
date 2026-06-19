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

"""
Authoritative server that simulates Kaminsky-style off-path spoofing on UDP:
for every UDP query for trigger.example./A it sends one response with a
deliberately flipped DNS message id.  A resolver that escalates to TCP on
the first id mismatch will still get the correct answer over TCP, which
this server serves normally.
"""

from collections.abc import AsyncGenerator

import dns.name
import dns.rdatatype

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsProtocol,
    DnsResponseSend,
    QueryContext,
    ResponseAction,
    ResponseHandler,
)


class MismatchOnUdpHandler(ResponseHandler):
    """
    Spoof UDP queries for trigger.example./A with a properly-formed
    response whose DNS message id does not match the request.  Answer
    the same query normally on TCP using the zone data prepared by the
    framework.
    """

    def __init__(self) -> None:
        self._trigger = dns.name.from_text("trigger.example.")

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qname == self._trigger and qctx.qtype == dns.rdatatype.A

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        if qctx.protocol == DnsProtocol.UDP:
            qctx.response.id = qctx.query.id ^ 0xFFFF
            yield DnsResponseSend(qctx.response)
        else:
            yield DnsResponseSend(qctx.response)


def main() -> None:
    server = AsyncDnsServer()
    server.install_response_handler(MismatchOnUdpHandler())
    server.run()


if __name__ == "__main__":
    main()
