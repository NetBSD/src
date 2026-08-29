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
from typing import final

import abc
import asyncio

import dns.flags
import dns.name
import dns.rcode
import dns.rdataclass
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    ControlCommand,
    ControllableAsyncDnsServer,
    DnsResponseSend,
    QnameHandler,
    QueryContext,
    ResponseAction,
    ResponseHandler,
)


class ReclimitStateHandler(QnameHandler):
    """
    Handler for the "count." and "reset." queries that also holds the state
    shared by all the handlers in one server.
    """

    qnames = ["count.", "reset."]

    def __init__(self, indirect_send_response_default: bool = True) -> None:
        self._indirect_send_response_default = indirect_send_response_default
        self.count = 0
        self.limit = 0
        self.indirect_send_response = indirect_send_response_default
        super().__init__()

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        if f"{qctx.qname}" == "count.":
            self.count += 1
            qctx.response.answer.append(
                dns.rrset.from_text(
                    "count.", 0, dns.rdataclass.IN, dns.rdatatype.TXT, f"{self.count}"
                )
            )
            yield DnsResponseSend(qctx.response, authoritative=True)
        elif f"{qctx.qname}" == "reset.":
            self.reset()
            yield DnsResponseSend(qctx.response, authoritative=False)

    def reset(self) -> None:
        self.count = 0
        self.indirect_send_response = self._indirect_send_response_default


class ReclimitHandler(ResponseHandler):
    """
    Base class for handlers in this test.

    Increments the shared query counter on each query and delegates the actual
    response generation to the `_get_counted_responses()` method.
    """

    def __init__(self, state_handler: ReclimitStateHandler) -> None:
        self._state = state_handler
        super().__init__()

    @final
    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        self._state.count += 1
        async for response in self._get_counted_responses(qctx):
            yield response

    @abc.abstractmethod
    async def _get_counted_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        yield DnsResponseSend(qctx.response)


class LimitControlCommand(ControlCommand):
    control_subdomain = "limit"

    def __init__(self, state_handler: ReclimitStateHandler) -> None:
        self._state_handler = state_handler
        super().__init__()

    def handle(
        self, args: list[str], server: ControllableAsyncDnsServer, qctx: QueryContext
    ) -> str | None:
        if len(args) != 1:
            return "Expected exactly one label"

        try:
            limit = int(args[0])
        except ValueError:
            return "Expected an integer"

        self._state_handler.limit = limit
        return f"Limit set to {limit}"


def a(owner: str | dns.name.Name, ns_number: int) -> dns.rrset.RRset:
    return dns.rrset.from_text(
        f"{owner}", 3600, dns.rdataclass.IN, dns.rdatatype.A, f"10.53.0.{ns_number}"
    )


def ns(owner: str | dns.name.Name, target: str | dns.name.Name) -> dns.rrset.RRset:
    return dns.rrset.from_text(
        f"{owner}", 86400, dns.rdataclass.IN, dns.rdatatype.NS, f"{target}"
    )


class DirectExampleHandler(ReclimitHandler, QnameHandler):
    qnames = ["direct.example.org", "direct.example.net"]

    def __init__(
        self, state_handler: ReclimitStateHandler, local_ns_number: int
    ) -> None:
        self._local_ns_number = local_ns_number
        super().__init__(state_handler)

    async def _get_counted_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        if qctx.qtype == dns.rdatatype.A:
            qctx.response.answer.append(a(qctx.qname, self._local_ns_number))
        yield DnsResponseSend(qctx.response)


class IndirectExampleOrgHandler(ReclimitHandler, QnameHandler):
    qnames = [f"indirect{i}.example.org" for i in range(1, 9)]

    def __init__(
        self, state_handler: ReclimitStateHandler, local_ns_number: int
    ) -> None:
        self._local_ns_number = local_ns_number
        super().__init__(state_handler)

    async def _get_counted_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        if not self._state.indirect_send_response:
            qctx.response.authority.append(ns(f"{qctx.qname}", "ns1.1.example.org."))
            qctx.response.flags &= ~dns.flags.AA
        elif qctx.qtype == dns.rdatatype.A:
            qctx.response.answer.append(a(qctx.qname, self._local_ns_number))
        yield DnsResponseSend(qctx.response)


def is_ns1_example(qname: dns.name.Name, tld: str) -> bool:
    labels = qname.labels
    return (
        len(labels) == 5
        and labels[3] == tld.encode()
        and labels[2] == b"example"
        and labels[1].isdigit()
        and labels[0] == b"ns1"
    )


class Ns1ExampleOrgHandler(ReclimitHandler):
    def __init__(self, state_handler: ReclimitStateHandler) -> None:
        self._second_query_events: dict[dns.name.Name, asyncio.Event] = {}
        super().__init__(state_handler)

    def match(self, qctx: QueryContext) -> bool:
        return is_ns1_example(qctx.qname, "org") and qctx.qtype in (
            dns.rdatatype.A,
            dns.rdatatype.AAAA,
        )

    async def _get_counted_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        ns_number = int(qctx.qname.labels[1])
        next_ns_number = ns_number + 1
        if not self._state.limit or (
            not self._state.indirect_send_response
            and next_ns_number <= self._state.limit
        ):
            qctx.response.authority.append(
                ns(f"{ns_number}.example.org.", f"ns1.{next_ns_number}.example.org.")
            )
            qctx.response.flags &= ~dns.flags.AA
        else:
            self._state.indirect_send_response = True
            if qctx.qtype == dns.rdatatype.A:
                qctx.response.answer.append(a(qctx.qname, 4))

        second_query_event = self._second_query_events.get(qctx.qname)
        if second_query_event is not None:
            # Second query arrived, release the first response.
            second_query_event.set()
            await asyncio.sleep(0)  # Yield to allow the first response to be sent.
            yield DnsResponseSend(qctx.response)
        else:
            # Delay the response until the second query for the same QNAME
            # arrives; give up waiting after 500 ms.
            second_query_event = asyncio.Event()
            self._second_query_events[qctx.qname] = second_query_event
            try:
                await asyncio.wait_for(second_query_event.wait(), timeout=0.5)
            except asyncio.TimeoutError:
                pass
            finally:
                del self._second_query_events[qctx.qname]
            yield DnsResponseSend(qctx.response)


class FallbackNxdomainHandler(ReclimitHandler):
    async def _get_counted_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.response.set_rcode(dns.rcode.NXDOMAIN)
        yield DnsResponseSend(qctx.response)
