"""
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
"""

from typing import AsyncGenerator

import dns.rcode
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    ControllableAsyncDnsServer,
    DnsResponseSend,
    QueryContext,
    ResponseHandler,
    ToggleResponsesCommand,
)


class MaybeDelayedAddressAnswerHandler(ResponseHandler):
    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        if qctx.qtype in (dns.rdatatype.A, dns.rdatatype.AAAA):
            addr = "192.0.2.1" if qctx.qtype == dns.rdatatype.A else "2001:db8:beef::1"
            rrset = dns.rrset.from_text(qctx.qname, 300, qctx.qclass, qctx.qtype, addr)
            qctx.response.answer.append(rrset)

        delay = 0.05 if qctx.qname.labels[0].startswith(b"latency") else 0.00
        yield DnsResponseSend(qctx.response, delay=delay)


def main() -> None:
    server = ControllableAsyncDnsServer(
        default_aa=True, default_rcode=dns.rcode.NOERROR
    )
    server.install_control_command(ToggleResponsesCommand())
    server.install_response_handler(MaybeDelayedAddressAnswerHandler())
    server.run()


if __name__ == "__main__":
    main()
