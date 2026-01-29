"""
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
"""

import ipaddress
from typing import AsyncGenerator

import dns.rcode
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsResponseSend,
    QueryContext,
    ResponseHandler,
)


class IncrementARecordHandler(ResponseHandler):
    """
    To test the TTL=0 behavior, increment the IPv4 address by one every
    time we get queried.
    """

    def __init__(self):
        self._ip_address = ipaddress.ip_address("192.0.2.0")

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        if qctx.qtype == dns.rdatatype.A:
            rrset = dns.rrset.from_text(
                qctx.qname, 0, qctx.qclass, dns.rdatatype.A, str(self._ip_address)
            )
            qctx.response.answer.append(rrset)
            self._ip_address += 1

        yield DnsResponseSend(qctx.response)


def main() -> None:
    server = AsyncDnsServer(default_aa=True, default_rcode=dns.rcode.NOERROR)
    server.install_response_handler(IncrementARecordHandler())
    server.run()


if __name__ == "__main__":
    main()
