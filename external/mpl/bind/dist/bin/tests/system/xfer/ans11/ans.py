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

import dns.flags
import dns.rcode
import dns.rdataclass
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    AsyncDnsServer,
    AxfrHandler,
    DnsProtocol,
    DnsResponseSend,
    QueryContext,
    ResponseHandler,
    StaticResponseHandler,
)

ZONE = "ixfr-race."
NS_NAME = f"ns.{ZONE}"
FIRST_DIFF_RECORDS = 50
SECOND_DIFF_RECORDS = 200


def rrset(
    owner: str, ttl: int, rdtype: dns.rdatatype.RdataType, rdata: str
) -> dns.rrset.RRset:
    return dns.rrset.from_text(owner, ttl, dns.rdataclass.IN, rdtype, rdata)


def soa(serial: int) -> dns.rrset.RRset:
    return rrset(
        ZONE,
        3600,
        dns.rdatatype.SOA,
        f"{NS_NAME} admin.{ZONE} {serial} 3600 900 604800 86400",
    )


def a(owner: str, address: str) -> dns.rrset.RRset:
    return rrset(owner, 3600, dns.rdatatype.A, address)


def ns() -> dns.rrset.RRset:
    return rrset(ZONE, 3600, dns.rdatatype.NS, NS_NAME)


def host_a_records(prefix: str, second_octet: int, count: int) -> list[dns.rrset.RRset]:
    return [
        a(f"{prefix}-{i}.{ZONE}", f"10.{second_octet}.{(i >> 8) & 0xFF}.{i & 0xFF}")
        for i in range(count)
    ]


def ixfr_diffs() -> list[dns.rrset.RRset]:
    """
    Craft two complete IXFR diffs, but omit the terminating SOA.

    Committing the first diff starts the apply worker.  Parsing the second,
    larger diff gives that worker time to splice the first chunk from the
    shared queue before the second chunk is committed.  The second chunk is
    then left on xfr->diff_head for the following SERVFAIL to expose GL#6114.
    """
    return [
        soa(4),
        soa(1),
        soa(2),
        *host_a_records("first", 1, FIRST_DIFF_RECORDS),
        soa(2),
        soa(3),
        *host_a_records("second", 2, SECOND_DIFF_RECORDS),
        soa(3),
    ]


class TransferState:
    """
    Flag toggled once the initial AXFR (serial 1) has been served.  The SOA
    handlers read it to advertise serial 1 before the transfer and serial 4
    after it, so the secondary's next refresh switches from AXFR to IXFR.
    """

    def __init__(self) -> None:
        self.initial_axfr_served = False


class TransferHandler(ResponseHandler):
    """Base for the handlers that share a single TransferState."""

    def __init__(self, progress: TransferState) -> None:
        super().__init__()
        self._progress = progress


class InitialSoaHandler(TransferHandler, StaticResponseHandler):
    answer = [soa(1)]

    def match(self, qctx: QueryContext) -> bool:
        return (
            qctx.qtype == dns.rdatatype.SOA and not self._progress.initial_axfr_served
        )


class RefreshSoaHandler(TransferHandler, StaticResponseHandler):
    answer = [soa(4)]

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qtype == dns.rdatatype.SOA and self._progress.initial_axfr_served


class InitialAxfrHandler(TransferHandler, AxfrHandler):
    """Serve the initial zone (serial 1) and mark the transfer as done."""

    initial_soa = soa(1)
    zone_contents = [
        ns(),
        a(NS_NAME, "127.0.0.1"),
    ]
    final_soa = soa(1)

    def match(self, qctx: QueryContext) -> bool:
        matched = super().match(qctx)
        if matched:
            self._progress.initial_axfr_served = True
        return matched


class TruncatedIxfrHandler(ResponseHandler):
    """Set TC on an IXFR received over UDP to force the secondary to retry over TCP."""

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qtype == dns.rdatatype.IXFR and qctx.protocol == DnsProtocol.UDP

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.response.flags |= dns.flags.TC
        yield DnsResponseSend(qctx.response)


class RaceIxfrHandler(ResponseHandler):
    """
    Reproduce the IXFR->AXFR use-after-free races (GL#5767 and GL#6114).

    Over TCP, send two valid IXFR diffs.  The first starts the apply worker and
    the second remains queued after that worker splices the first chunk.
    Immediately follow them with a SERVFAIL response that makes
    xfrin_recv_done() defer the AXFR retry until the worker finishes.  Cleanup
    then has to detach the queued second chunk before freeing it.
    """

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qtype == dns.rdatatype.IXFR and qctx.protocol == DnsProtocol.TCP

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        for rrset_ in ixfr_diffs():
            qctx.response.answer.append(rrset_)
        yield DnsResponseSend(qctx.response)

        qctx.prepare_new_response(with_zone_data=False)
        qctx.response.set_rcode(dns.rcode.SERVFAIL)
        yield DnsResponseSend(qctx.response)


def main() -> None:
    server = AsyncDnsServer(default_aa=True, default_rcode=dns.rcode.NOERROR)
    progress = TransferState()
    server.install_response_handlers(
        InitialSoaHandler(progress),
        RefreshSoaHandler(progress),
        InitialAxfrHandler(progress),
        TruncatedIxfrHandler(),
        RaceIxfrHandler(),
    )
    server.run()


if __name__ == "__main__":
    main()
