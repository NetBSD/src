"""
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
"""

import dns.rcode
import dns.rdataclass
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    AsyncDnsServer,
    QnameQtypeHandler,
    QueryContext,
    StaticResponseHandler,
)

DNS64_TRIGGER = "nodata.test."


def rrset(name: str, rdtype: dns.rdatatype.RdataType, *rdata: str) -> dns.rrset.RRset:
    return dns.rrset.from_text(name, 60, dns.rdataclass.IN, rdtype, *rdata)


def soa() -> dns.rrset.RRset:
    return rrset("test.", dns.rdatatype.SOA, "ns.test. hostmaster.test. 1 2 3 4 5")


def a() -> dns.rrset.RRset:
    return rrset(DNS64_TRIGGER, dns.rdatatype.A, "192.0.2.1")


def aaaa() -> dns.rrset.RRset:
    return rrset(DNS64_TRIGGER, dns.rdatatype.AAAA, "2001:db8::1")


class NodataOnceHandler(QnameQtypeHandler, StaticResponseHandler):
    """
    Answer only the first AAAA query, with NODATA, so the DNS64 resolver looks
    up an A record; the delayed A lookup lets filter-a re-enter and re-query
    AAAA (answered by AaaaHandler), triggering the bug.
    """

    qnames = [DNS64_TRIGGER]
    qtypes = [dns.rdatatype.AAAA]
    authority = [soa()]

    def __init__(self) -> None:
        super().__init__()
        self._answered = False

    def match(self, qctx: QueryContext) -> bool:
        if self._answered:
            return False
        self._answered = True
        return super().match(qctx)


class AaaaHandler(QnameQtypeHandler, StaticResponseHandler):
    qnames = [DNS64_TRIGGER]
    qtypes = [dns.rdatatype.AAAA]
    answer = [aaaa()]


class DelayedAHandler(QnameQtypeHandler, StaticResponseHandler):
    qnames = [DNS64_TRIGGER]
    qtypes = [dns.rdatatype.A]
    answer = [a()]
    delay = 2.0


class FallbackHandler(StaticResponseHandler):
    rcode = dns.rcode.NXDOMAIN
    authority = [soa()]


def main() -> None:
    server = AsyncDnsServer(default_aa=True, default_rcode=dns.rcode.NOERROR)
    server.install_response_handlers(
        NodataOnceHandler(),
        AaaaHandler(),
        DelayedAHandler(),
        FallbackHandler(),
    )
    server.run()


if __name__ == "__main__":
    main()
