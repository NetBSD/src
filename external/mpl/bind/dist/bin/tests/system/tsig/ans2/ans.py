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

import dns.flags

from isctest.asyncserver import (
    AsyncDnsServer,
    BytesResponseSend,
    QueryContext,
    ResponseHandler,
)


class TruncatedWithLastByteDroppedHandler(ResponseHandler):
    """
    Return a TC=1 response with the final byte removed to make
    dns_message_parse() return ISC_R_UNEXPECTEDEND.
    """

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[BytesResponseSend, None]:
        tc_response = qctx.query
        tc_response.flags |= dns.flags.QR
        tc_response.flags |= dns.flags.TC
        tc_response.flags |= dns.flags.RA
        yield BytesResponseSend(tc_response.to_wire()[:-1])


def main() -> None:
    server = AsyncDnsServer(keyring=None)
    server.install_response_handler(TruncatedWithLastByteDroppedHandler())
    server.run()


if __name__ == "__main__":
    main()
