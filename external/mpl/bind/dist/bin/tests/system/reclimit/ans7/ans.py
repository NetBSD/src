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

from isctest.asyncserver import AsyncDnsServer, DnsResponseSend, QueryContext

from ..reclimit_ans import ReclimitHandler, ReclimitStateHandler


class FallbackRefusedHandler(ReclimitHandler):
    async def _get_counted_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.response.set_rcode(dns.rcode.REFUSED)
        yield DnsResponseSend(qctx.response)


def main() -> None:
    server = AsyncDnsServer(default_aa=True, default_rcode=dns.rcode.NOERROR)
    server.install_response_handlers(
        state_handler := ReclimitStateHandler(),
        FallbackRefusedHandler(state_handler),
    )
    server.run()


if __name__ == "__main__":
    main()
