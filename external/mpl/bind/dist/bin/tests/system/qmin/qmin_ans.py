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

import abc

import dns.rcode
import dns.rdatatype

from isctest.asyncserver import (
    DnsResponseSend,
    DomainHandler,
    QueryContext,
    ResponseAction,
)


def log_query(qctx: QueryContext) -> None:
    """
    Log a received DNS query to a text file inspected by `tests.sh`.  AAAA and
    A queries are logged identically because the relative order in which they
    are received does not matter.
    """
    qname = qctx.qname.to_text()
    qtype = dns.rdatatype.to_text(qctx.qtype)
    if qtype in ("A", "AAAA"):
        qtype = "ADDR"

    with open("query.log", "a", encoding="utf-8") as query_log:
        print(f"{qtype} {qname}", file=query_log)


class QueryLogHandler(DomainHandler):
    """
    Log all received DNS queries to a text file.  Use the zone file for
    preparing responses.
    """

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        log_query(qctx)
        yield DnsResponseSend(qctx.response)


class EntRcodeChanger(DomainHandler):
    """
    Log all received DNS queries to a text file.  Use the zone file for
    preparing responses, but override the RCODE returned for empty
    non-terminals (ENTs) to the value specified by the child class.  This
    emulates broken authoritative servers.
    """

    @property
    @abc.abstractmethod
    def rcode(self) -> dns.rcode.Rcode:
        raise NotImplementedError

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        assert qctx.zone

        log_query(qctx)

        if (
            qctx.response.rcode() == dns.rcode.NOERROR
            and not qctx.response.answer
            and qctx.response.authority
            and qctx.response.authority[0].rdtype == dns.rdatatype.SOA
            and not qctx.zone.get_node(qctx.qname)
        ):
            qctx.response.set_rcode(self.rcode)
            yield DnsResponseSend(qctx.response)


class DelayedResponseHandler(DomainHandler):
    """
    Log all received DNS queries to a text file.  Use the zone file for
    preparing responses, but delay sending every answer by the amount of time
    specified (in seconds) by the child class.  This emulates network delays.
    """

    @property
    @abc.abstractmethod
    def delay(self) -> float:
        raise NotImplementedError

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        log_query(qctx)
        yield DnsResponseSend(qctx.response, delay=self.delay)
