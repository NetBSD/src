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

import dns.name
import dns.rcode
import dns.rdataclass
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsResponseSend,
    QnameHandler,
    QueryContext,
    ResponseAction,
    ResponseHandler,
    StaticResponseHandler,
)


def rrsig_covering(
    owner: dns.name.Name | str, covered: dns.rdatatype.RdataType
) -> dns.rrset.RRset:
    return dns.rrset.from_text(
        owner,
        3600,
        dns.rdataclass.IN,
        dns.rdatatype.RRSIG,
        f"TYPE{int(covered)} 8 2 3600 20300101000000 20200101000000 "
        "12345 attacker.test. AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
    )


class RrsigCoversRrsigHandler(QnameHandler, StaticResponseHandler):
    """
    An RRSIG covering RRSIG only trips the QP-cache RRSIG-pairing assertion when
    a second RRSIG header shares the owner name, so serve two ordinary RRSIGs
    (covering A and AAAA) alongside the RRSIG-covers-RRSIG poison.  A lone
    RRSIG-covers-RRSIG record is cached harmlessly.
    """

    qnames = ["rrsig.attacker.test."]
    answer = [
        rrsig_covering(qnames[0], dns.rdatatype.A),
        rrsig_covering(qnames[0], dns.rdatatype.AAAA),
        rrsig_covering(qnames[0], dns.rdatatype.RRSIG),
    ]


class RrsigCoversTypeHandler(ResponseHandler):
    """
    Answer any other query with a single RRSIG whose Type-Covered field is the
    leftmost QNAME label parsed as a DNS type, so the resolver can be probed
    with any meta-type (e.g. any.attacker.test., axfr.attacker.test.).
    """

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        covered_label = qctx.qname.labels[0].decode("ascii").upper()
        covered = dns.rdatatype.from_text(covered_label)
        qctx.response.answer.append(rrsig_covering(qctx.qname, covered))
        yield DnsResponseSend(qctx.response)


def main() -> None:
    server = AsyncDnsServer(default_aa=True, default_rcode=dns.rcode.NOERROR)
    server.install_response_handlers(
        RrsigCoversRrsigHandler(),
        RrsigCoversTypeHandler(),
    )
    server.run()


if __name__ == "__main__":
    main()
