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

from typing import AsyncGenerator

import dns.flags
import dns.rcode

from isctest.asyncserver import (
    AsyncDnsServer,
    ConnectionReset,
    DnsProtocol,
    DnsResponseSend,
    QueryContext,
    ResponseAction,
    ResponseHandler,
)


class TruncateOnUdpHandler(ResponseHandler):
    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        assert qctx.protocol == DnsProtocol.UDP, "This server only supports UDP"
        qctx.response.flags |= dns.flags.TC
        yield DnsResponseSend(qctx.response)


def main() -> None:
    server = AsyncDnsServer(default_rcode=dns.rcode.NOERROR)
    server.install_connection_handler(ConnectionReset(delay=1.0))
    server.install_response_handler(TruncateOnUdpHandler())
    server.run()


if __name__ == "__main__":
    main()
