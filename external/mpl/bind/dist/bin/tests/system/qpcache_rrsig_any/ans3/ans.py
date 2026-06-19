"""
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.

For any query, returns a hand-crafted RRSIG whose Type-Covered field
is selected by the leftmost label of QNAME. The label is parsed as a
DNS type via `dns.rdatatype.from_text()`, so the resolver can be
probed with any meta-type by querying e.g. `any.attacker.test.`,
`axfr.attacker.test.`, `tsig.attacker.test.`, etc.
"""

from collections.abc import AsyncGenerator

import dns.flags
import dns.rcode
import dns.rdataclass
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsResponseSend,
    QueryContext,
    ResponseHandler,
)


class RrsigCoversHandler(ResponseHandler):
    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        covers_label = qctx.qname.labels[0].decode("ascii").upper()
        covers = dns.rdatatype.from_text(covers_label)
        rrset = dns.rrset.from_text(
            qctx.qname,
            3600,
            dns.rdataclass.IN,
            dns.rdatatype.RRSIG,
            f"TYPE{int(covers)} 8 2 3600 20300101000000 20200101000000 "
            "12345 attacker.test. AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
        )
        qctx.response.set_rcode(dns.rcode.NOERROR)
        qctx.response.flags |= dns.flags.AA
        qctx.response.answer.append(rrset)
        yield DnsResponseSend(qctx.response)


def main() -> None:
    server = AsyncDnsServer()
    server.install_response_handler(RrsigCoversHandler())
    server.run()


if __name__ == "__main__":
    main()
