"""
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
"""

from collections.abc import AsyncGenerator, Collection, Iterable

import abc

import dns.rcode
import dns.rdataclass
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    ControllableAsyncDnsServer,
    DnsResponseSend,
    QueryContext,
    ResponseHandler,
    SwitchControlCommand,
)


def rrset(owner: str, rdtype: dns.rdatatype.RdataType, rdata: str) -> dns.rrset.RRset:
    return dns.rrset.from_text(
        owner,
        300,
        dns.rdataclass.IN,
        rdtype,
        rdata,
    )


def soa(serial: int, *, owner: str = "nil.") -> dns.rrset.RRset:
    return rrset(
        owner,
        dns.rdatatype.SOA,
        f"ns.nil. root.nil. {serial} 300 300 604800 300",
    )


def ns() -> dns.rrset.RRset:
    return rrset(
        "nil.",
        dns.rdatatype.NS,
        "ns.nil.",
    )


def a(address: str, *, owner: str) -> dns.rrset.RRset:
    return rrset(
        owner,
        dns.rdatatype.A,
        address,
    )


def txt(data: str, *, owner: str = "nil.") -> dns.rrset.RRset:
    return rrset(
        owner,
        dns.rdatatype.TXT,
        f'"{data}"',
    )


class SoaHandler(ResponseHandler):
    def __init__(self, serial: int):
        self._serial = serial

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qtype == dns.rdatatype.SOA

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.response.answer.append(soa(self._serial))
        yield DnsResponseSend(qctx.response)


class AxfrHandler(ResponseHandler):
    @property
    @abc.abstractmethod
    def answers(self) -> Iterable[Collection[dns.rrset.RRset]]:
        """
        Answer sections of response packets sent in response to
        AXFR queries.
        """
        raise NotImplementedError

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qtype == dns.rdatatype.AXFR

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        for answer in self.answers:
            response = qctx.prepare_new_response()
            for rrset_ in answer:
                response.answer.append(rrset_)
            yield DnsResponseSend(response)


class IxfrHandler(ResponseHandler):
    @property
    @abc.abstractmethod
    def answer(self) -> Collection[dns.rrset.RRset]:
        """
        Answer section of a response packet sent in response to
        IXFR queries.
        """
        raise NotImplementedError

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qtype == dns.rdatatype.IXFR

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        for rrset_ in self.answer:
            qctx.response.answer.append(rrset_)
        yield DnsResponseSend(qctx.response)


class InitialAxfrHandler(AxfrHandler):
    answers = (
        (soa(1),),
        (
            ns(),
            txt("initial AXFR"),
            a("10.0.0.61", owner="a.nil."),
            a("10.0.0.62", owner="b.nil."),
        ),
        (soa(1),),
    )


class InitialIxfrHandler(IxfrHandler):
    answer = (
        soa(1),
        ns(),
        txt("initial AXFR"),
        a("10.0.0.61", owner="a.nil."),
        a("10.0.0.62", owner="b.nil."),
        soa(1),
    )


class UnchangedIxfrHandler(IxfrHandler):
    """
    IXFR from serial 1 -> 2.

    The diff deletes nothing for the A rrset at a.nil., but re-adds
    "a.nil. A 10.0.0.61" which already exists. This causes the merge
    to find no new records, triggering DNS_R_UNCHANGED.

    We also add a new TXT record so the IXFR has at least one real
    change (the SOA serial bump + TXT addition).
    """

    answer = (
        soa(2),
        soa(1),
        soa(2),
        txt("unchanged ixfr test"),
        a("10.0.0.61", owner="a.nil."),  # already exists -> DNS_R_UNCHANGED
        soa(2),
    )


def main() -> None:
    server = ControllableAsyncDnsServer(
        default_aa=True, default_rcode=dns.rcode.NOERROR
    )
    switch_command = SwitchControlCommand(
        {
            "initial_axfr": (
                SoaHandler(1),
                InitialIxfrHandler(),
                InitialAxfrHandler(),
            ),
            "unchanged_ixfr": (
                SoaHandler(2),
                UnchangedIxfrHandler(),
            ),
        }
    )
    server.install_control_command(switch_command)
    server.run()


if __name__ == "__main__":
    main()
