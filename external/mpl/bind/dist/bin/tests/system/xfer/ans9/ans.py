"""
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
"""

from collections.abc import AsyncGenerator, Collection

import dns.name
import dns.rcode
import dns.rdataclass
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    AxfrHandler,
    ControllableAsyncDnsServer,
    DnsResponseSend,
    QueryContext,
    ResponseAction,
    ResponseHandler,
    ToggleResponsesCommand,
)

TTL = 300

RECONFIG_ZONE = "xfr-and-reconfig."
OVERRUN_ZONE = "private-dns-overrun."

# The malformed DNSKEY's algorithm identifier finishes on a 00 byte in the
# record that follows it in the same message.  That following record, the
# well-formed DNSKEY, starts with a compression pointer followed by the type,
# which starts with 00.
OVERRUN_DNSKEY = "\\# 12 00 00 00 fd 09 00 00 00 00 00 00 00"
WELL_FORMED_DNSKEY = "\\# 12 00 00 00 fd 06 00 00 00 00 00 00 00"


def rrset(
    owner: str | dns.name.Name, rdtype: dns.rdatatype.RdataType, rdata: str
) -> dns.rrset.RRset:
    return dns.rrset.from_text(owner, TTL, dns.rdataclass.IN, rdtype, rdata)


def soa(owner: str | dns.name.Name, serial: int) -> dns.rrset.RRset:
    return rrset(owner, dns.rdatatype.SOA, f". . {serial} 0 0 0 0")


def ns(owner: str | dns.name.Name) -> dns.rrset.RRset:
    return rrset(owner, dns.rdatatype.NS, ".")


def txt(owner: str | dns.name.Name) -> dns.rrset.RRset:
    return rrset(owner, dns.rdatatype.TXT, "foo bar")


def dnskey(owner: str | dns.name.Name, keydata: str) -> dns.rrset.RRset:
    return rrset(owner, dns.rdatatype.DNSKEY, keydata)


class SerialCounter:
    """
    Shared SOA serial advanced by one after every completed AXFR, so each
    secondary refresh sees a newer serial and re-transfers the zone.
    """

    def __init__(self) -> None:
        self.serial = 0


class SerialCounted(ResponseHandler):
    def __init__(self, serials: SerialCounter) -> None:
        super().__init__()
        self._serials = serials


class SoaHandler(SerialCounted):
    def match(self, qctx: QueryContext) -> bool:
        return qctx.qtype == dns.rdatatype.SOA

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.response.answer.append(soa(qctx.qname, self._serials.serial))
        yield DnsResponseSend(qctx.response)


class ZoneAxfrHandler(AxfrHandler, SerialCounted):
    """
    Serve an AXFR for a single zone whose SOA serial is drawn from a shared
    SerialCounter, bumping it once the transfer completes.  Subclasses set
    `zone` and define `zone_contents`.
    """

    zone: str

    def match(self, qctx: QueryContext) -> bool:
        return super().match(qctx) and qctx.qname == dns.name.from_text(self.zone)

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        async for action in super().get_responses(qctx):
            yield action
        self._serials.serial += 1

    @property
    def initial_soa(self) -> dns.rrset.RRset:
        return soa(self.zone, self._serials.serial)

    @property
    def final_soa(self) -> dns.rrset.RRset:
        return soa(self.zone, self._serials.serial)


class ReconfigAxfrHandler(ZoneAxfrHandler):
    zone = RECONFIG_ZONE

    @property
    def zone_contents(self) -> Collection[dns.rrset.RRset]:
        return [
            ns(self.zone),
            txt(self.zone),
        ]


class OverrunAxfrHandler(ZoneAxfrHandler):
    """Serve the malformed PRIVATEDNS DNSKEY overrun; see OVERRUN_DNSKEY."""

    zone = OVERRUN_ZONE

    @property
    def zone_contents(self) -> Collection[dns.rrset.RRset]:
        return [
            ns(self.zone),
            txt(self.zone),
            dnskey(self.zone, OVERRUN_DNSKEY),
            dnskey(self.zone, WELL_FORMED_DNSKEY),
        ]


def main() -> None:
    server = ControllableAsyncDnsServer(
        default_aa=True, default_rcode=dns.rcode.NOERROR
    )
    server.install_control_command(ToggleResponsesCommand())
    serials = SerialCounter()
    server.install_response_handlers(
        SoaHandler(serials),
        OverrunAxfrHandler(serials),
        ReconfigAxfrHandler(serials),
    )
    server.run()


if __name__ == "__main__":
    main()
