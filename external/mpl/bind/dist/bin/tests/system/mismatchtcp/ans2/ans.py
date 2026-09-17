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

import dns.rdatatype

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsProtocol,
    DnsResponseSend,
    QnameQtypeHandler,
    QueryContext,
    ResponseAction,
)


class MismatchedIdOnUdpHandler(QnameQtypeHandler):
    """
    Simulate Kaminsky-style off-path spoofing: answer every UDP query for
    trigger.example./A with the correct response prepared from zone data,
    but with a deliberately flipped DNS message id.  TCP queries do not
    match this handler and are answered from zone data as usual, so a
    resolver that escalates to TCP on the first id mismatch still gets
    the correct answer.
    """

    qnames = ["trigger.example."]
    qtypes = [dns.rdatatype.A]

    def match(self, qctx: QueryContext) -> bool:
        return qctx.protocol == DnsProtocol.UDP and super().match(qctx)

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        qctx.response.id = qctx.query.id ^ 0xFFFF
        yield DnsResponseSend(qctx.response)


def main() -> None:
    server = AsyncDnsServer()
    server.install_response_handler(MismatchedIdOnUdpHandler())
    server.run()


if __name__ == "__main__":
    main()
