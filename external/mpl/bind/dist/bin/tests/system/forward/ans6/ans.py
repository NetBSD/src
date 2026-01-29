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

import dns.name
import dns.rcode
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    ControllableAsyncDnsServer,
    DnsResponseSend,
    QueryContext,
    ResponseAction,
    ResponseHandler,
    ToggleResponsesCommand,
)


class ChaseDsHandler(ResponseHandler):
    """
    Yield responses triggering DS chasing logic in `named`.  These responses
    cannot be served from a static zone file because most of them need to be
    generated dynamically so that the owner name of the returned RRset is
    copied from the QNAME sent by the client:

      - A/AAAA queries for `ns1.sld.tld.` elicit responses with IP addresses,
      - all NS queries below `sld.tld.` elicit a delegation to `ns1.sld.tld.`,
      - all other queries elicit a negative response with a common SOA record.
    """

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        ns1_sld_tld = dns.name.from_text("ns1.sld.tld.")
        sld_tld = dns.name.from_text("sld.tld.")

        if qctx.qname == ns1_sld_tld and qctx.qtype == dns.rdatatype.A:
            response_type = dns.rdatatype.A
            response_rdata = "10.53.0.2"
            response_section = qctx.response.answer
        elif qctx.qname == ns1_sld_tld and qctx.qtype == dns.rdatatype.AAAA:
            response_type = dns.rdatatype.AAAA
            response_rdata = "fd92:7065:b8e:ffff::2"
            response_section = qctx.response.answer
        elif qctx.qname.is_subdomain(sld_tld) and qctx.qtype == dns.rdatatype.NS:
            response_type = dns.rdatatype.NS
            response_rdata = "ns1.sld.tld."
            response_section = qctx.response.answer
        else:
            response_type = dns.rdatatype.SOA
            response_rdata = ". . 0 0 0 0 0"
            response_section = qctx.response.authority

        qctx.response.use_edns(None)

        response_rrset = dns.rrset.from_text(
            qctx.qname, 300, qctx.qclass, response_type, response_rdata
        )
        response_section.append(response_rrset)

        yield DnsResponseSend(qctx.response)


def main() -> None:
    server = ControllableAsyncDnsServer(
        default_rcode=dns.rcode.NOERROR, default_aa=True
    )
    server.install_control_command(ToggleResponsesCommand())
    server.install_response_handler(ChaseDsHandler())
    server.run()


if __name__ == "__main__":
    main()
