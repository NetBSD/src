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

import dns.rcode
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    ControllableAsyncDnsServer,
    DnsResponseSend,
    DomainHandler,
    QueryContext,
    ResponseAction,
    ToggleResponsesCommand,
)


class AXFRServer(DomainHandler):
    """
    Yield SOA and AXFR responses. Every new AXFR response increments the SOA
    version.
    """

    domains = ["xfr-and-reconfig"]

    def __init__(self) -> None:
        super().__init__()
        self.soa_version = 0

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        # This is oversimplified because I am lazy - we are appending the SOA
        # RRset to the ANSWER section for _every_ QTYPE.  named is only
        # expected to send a SOA query over UDP and then an AXFR query over
        # TCP.  Responses to both of those start with a SOA RRset in the ANSWER
        # section :-)
        soa_message = qctx.response
        soa_rrset = dns.rrset.from_text(
            qctx.qname,
            300,
            qctx.qclass,
            dns.rdatatype.SOA,
            f". . {self.soa_version} 0 0 0 0",
        )
        soa_message.answer.append(soa_rrset)

        yield DnsResponseSend(soa_message)

        if qctx.qtype == dns.rdatatype.SOA:
            # If QTYPE=SOA, the SOA record is the complete response.
            return

        if qctx.qtype != dns.rdatatype.AXFR:
            # If QTYPE=AXFR, we will continue cramming RRsets into the ANSWER
            # section of a subsequent DNS message below.
            #
            # If QTYPE was not SOA or AXFR, abort.  Yeah, we just sent a broken
            # response by yielding DnsResponseSend() with a SOA RRset in the
            # ANSWER section above.  We will have to carry that burden for the
            # rest of our lives.
            return

        # Send just the obligatory NS RRset at zone apex in the next message.
        # This is stupidly inefficient, but makes looping below simpler as we
        # will already have been done with the mandatory stuff by then.
        ns_message = qctx.prepare_new_response()
        ns_rrset = dns.rrset.from_text(
            qctx.qname, 300, qctx.qclass, dns.rdatatype.NS, "."
        )
        ns_message.answer.append(ns_rrset)

        yield DnsResponseSend(ns_message)

        # Generate the AXFR with a txt rrset.
        txt_message = qctx.prepare_new_response()
        txt_rrset = dns.rrset.from_text(
            qctx.qname,
            300,
            qctx.qclass,
            dns.rdatatype.TXT,
            "foo bar",
        )
        txt_message.answer.append(txt_rrset)

        yield DnsResponseSend(txt_message)

        # Finish the AXFR transaction by sending the second SOA RRset.
        yield DnsResponseSend(soa_message)

        # This makes sure that the next SOA request causes a new zone transfer
        self.soa_version += 1


if __name__ == "__main__":
    server = ControllableAsyncDnsServer(
        default_aa=True, default_rcode=dns.rcode.NOERROR
    )
    server.install_control_command(ToggleResponsesCommand())
    server.install_response_handler(AXFRServer())
    server.run()
