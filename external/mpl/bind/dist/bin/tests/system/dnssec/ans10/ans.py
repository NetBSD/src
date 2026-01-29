# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

from typing import AsyncGenerator

import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsResponseSend,
    QueryContext,
    ResponseHandler,
)


class AddRrsigToAHandler(ResponseHandler):
    def match(self, qctx: QueryContext) -> bool:
        return qctx.qtype == dns.rdatatype.A

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        rrsig = (
            f"A 13 2 60 {2**32-1} 0 12345 {qctx.qname.to_text()} "
            "gB+eISXAhSPZU2i/II0W9ZUhC2SCIrb94mlNvP5092WAeXxqN/vG43/1nmDly2Qs7y5VCjSMOGn85bnaMoAc7w=="
        )
        rrsig_rrset = dns.rrset.from_text(
            qctx.qname, 1, qctx.qclass, dns.rdatatype.RRSIG, rrsig
        )
        qctx.response.answer.append(rrsig_rrset)
        yield DnsResponseSend(qctx.response)


class AddNsecToTxtHandler(ResponseHandler):
    def match(self, qctx: QueryContext) -> bool:
        return qctx.qtype == dns.rdatatype.TXT

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        nsec = f"{qctx.qname.to_text()} A NS SOA RRSIG NSEC"
        nsec_rrset = dns.rrset.from_text(
            qctx.qname, 1, qctx.qclass, dns.rdatatype.NSEC, nsec
        )
        qctx.response.authority.append(nsec_rrset)
        yield DnsResponseSend(qctx.response)


def main() -> None:
    server = AsyncDnsServer()
    server.install_response_handlers([AddRrsigToAHandler(), AddNsecToTxtHandler()])
    server.run()


if __name__ == "__main__":
    main()
