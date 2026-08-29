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

from isctest.asyncserver import (
    ControllableAsyncDnsServer,
    DnsResponseSend,
    QueryContext,
)

from ..reclimit_ans import (
    DirectExampleHandler,
    FallbackNxdomainHandler,
    IndirectExampleOrgHandler,
    LimitControlCommand,
    Ns1ExampleOrgHandler,
    ReclimitHandler,
    ReclimitStateHandler,
    a,
    is_ns1_example,
    ns,
)


class Ns1ExampleNetHandler(ReclimitHandler):
    def match(self, qctx: QueryContext) -> bool:
        return is_ns1_example(qctx.qname, "net")

    async def _get_counted_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        current_ns_number = int(qctx.qname.labels[1])
        next_ns_block_start = (current_ns_number + 1) * 16
        for offset in range(1, 16):
            target_ns_number = next_ns_block_start + offset
            qctx.response.authority.append(
                ns(
                    f"{current_ns_number}.example.net.",
                    f"ns1.{target_ns_number}.example.net.",
                )
            )
            qctx.response.additional.append(
                a(f"ns1.{target_ns_number}.example.net.", 7)
            )

        yield DnsResponseSend(qctx.response, authoritative=False)


def main() -> None:
    server = ControllableAsyncDnsServer(
        default_aa=True, default_rcode=dns.rcode.NOERROR
    )
    server.install_response_handlers(
        state_handler := ReclimitStateHandler(indirect_send_response_default=False),
        DirectExampleHandler(state_handler, 2),
        IndirectExampleOrgHandler(state_handler, 2),
        Ns1ExampleOrgHandler(state_handler),
        Ns1ExampleNetHandler(state_handler),
        FallbackNxdomainHandler(state_handler),
    )
    server.install_control_command(LimitControlCommand(state_handler))
    server.run()


if __name__ == "__main__":
    main()
