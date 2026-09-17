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

import asyncio

import dns.rcode
import dns.rdatatype

from isctest.asyncserver import (
    ControlCommand,
    ControllableAsyncDnsServer,
    QueryContext,
    ResponseAction,
    ResponseHandler,
)

from ..serve_stale_ans import (
    StaticHandler,
    a_handler,
    fallback_handler,
    ns_handler,
    other_types_handler,
    soa,
    soa_handler,
    txt,
)

ANS9_ADDR = "10.53.0.9"


class SlowdownState:
    """
    One-shot slowdown flag, armed by the "slowdown" control query.

    The first data.slow query consumes it: the response to that query is
    delayed by three seconds and the slowdown is turned off again.
    """

    def __init__(self) -> None:
        self.armed = False

    def consume(self) -> float:
        delay = 3.0 if self.armed else 0.0
        self.armed = False
        return delay


class SlowdownControlCommand(ControlCommand):
    control_subdomain = "slowdown"

    def __init__(self, slowdown: SlowdownState) -> None:
        self._slowdown = slowdown
        super().__init__()

    def handle(
        self, args: list[str], server: ControllableAsyncDnsServer, qctx: QueryContext
    ) -> str | None:
        if args:
            return "Expected no extra labels"

        self._slowdown.armed = True
        return "slowdown armed"


class SerializedHandler(ResponseHandler):
    """
    Answer queries one at a time.

    All handlers of this server share one lock which is held until the
    response is sent, so a response delayed by the slowdown holds back the
    responses to all queries which arrive in the meantime.
    """

    def __init__(self, inner: ResponseHandler, lock: asyncio.Lock) -> None:
        self._inner = inner
        self._lock = lock
        super().__init__()

    def match(self, qctx: QueryContext) -> bool:
        return self._inner.match(qctx)

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        # This generator is suspended at the yield below while the response
        # is being delayed and sent, so the lock is held until the send
        # completes.
        async with self._lock:
            async for action in self._inner.get_responses(qctx):
                yield action

    def __str__(self) -> str:
        return f"Serialized({self._inner})"


def handlers(slowdown: SlowdownState) -> list[ResponseHandler]:
    return [
        a_handler("NsSlow", "ns.slow.", address=ANS9_ADDR),
        other_types_handler("NsSlow", "ns.slow.", "slow."),
        ns_handler("SlowZone", "slow.", "ns.slow.", address=ANS9_ADDR),
        soa_handler("SlowZone", "slow."),
        other_types_handler("SlowZone", "slow.", "slow."),
        StaticHandler(
            "DataSlowTxtHandler",
            "data.slow.",
            [dns.rdatatype.TXT],
            answer=[txt("data.slow.", "A slow text record with a 2 second ttl", ttl=2)],
            delay=slowdown.consume,
        ),
        StaticHandler(
            "DataSlowFallbackHandler",
            "data.slow.",
            authority=[soa("slow.", ttl=2)],
            delay=slowdown.consume,
        ),
        fallback_handler("slow."),
    ]


def main() -> None:
    server = ControllableAsyncDnsServer(
        default_aa=True, default_rcode=dns.rcode.NOERROR
    )
    slowdown = SlowdownState()
    lock = asyncio.Lock()
    server.install_response_handlers(
        *(SerializedHandler(handler, lock) for handler in handlers(slowdown))
    )
    server.install_control_command(SlowdownControlCommand(slowdown))
    server.run()


if __name__ == "__main__":
    main()
