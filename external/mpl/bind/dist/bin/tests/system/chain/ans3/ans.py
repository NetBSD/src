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
import dns.rdataclass
import dns.rdatatype
import dns.rrset
import dns.zone

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsResponseSend,
    DomainHandler,
    QueryContext,
    ResponseAction,
)

try:
    dns_namerelation_equal = dns.name.NameRelation.EQUAL
    dns_namerelation_subdomain = dns.name.NameRelation.SUBDOMAIN
except AttributeError:  # dnspython < 2.3.0 compat
    dns_namerelation_equal = dns.name.NAMERELN_EQUAL  # type: ignore
    dns_namerelation_subdomain = dns.name.NAMERELN_SUBDOMAIN  # type: ignore


def get_dname_rrset_at_name(
    zone: dns.zone.Zone, name: dns.name.Name
) -> dns.rrset.RRset:
    node = zone.get_node(name)
    assert node
    dname = node.get_rdataset(dns.rdataclass.IN, dns.rdatatype.DNAME)
    assert dname
    rrset = dns.rrset.RRset(name, dname.rdclass, dname.rdtype)
    rrset.update(dname)
    return rrset


class CnameThenDnameHandler(DomainHandler):
    """
    For certain trigger QNAMEs, insert a DNAME RRset after the CNAME chain
    prepared from zone data.
    """

    domains = ["example.broken."]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        assert qctx.zone
        assert qctx.zone.origin

        relative_qname = qctx.qname.relativize(qctx.zone.origin)
        if relative_qname.labels[-1].endswith(b"-then-dname"):
            last_cname = qctx.response.answer[-1]
            assert last_cname.rdtype == dns.rdatatype.CNAME
            dname_owner = last_cname.name.parent()
            dname_rrset = get_dname_rrset_at_name(qctx.zone, dname_owner)
            qctx.response.answer.append(dname_rrset)

        yield DnsResponseSend(qctx.response)


class Cve202125215(DomainHandler):
    """
    Attempt to trigger the resolver variant of CVE-2021-25215.  A `named`
    instance cannot be used for serving the DNAME records returned by this
    response handler as a version of BIND 9 vulnerable to CVE-2021-25215 would
    crash while answering the queries sent by the tested resolver.
    """

    domains = ["example.dname."]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        assert qctx.zone
        assert qctx.zone.origin

        self_example_dname = dns.name.Name(["self"]).concatenate(qctx.zone.origin)
        dname_rrset = get_dname_rrset_at_name(qctx.zone, self_example_dname)

        relation, _, _ = qctx.qname.fullcompare(self_example_dname)

        if relation in (dns_namerelation_equal, dns_namerelation_subdomain):
            del qctx.response.authority[:]
            qctx.response.set_rcode(dns.rcode.NOERROR)

        if relation == dns_namerelation_subdomain:
            qctx.response.answer.append(dname_rrset)
            cname_rrset = dns.rrset.from_text(
                qctx.qname,
                60,
                qctx.qclass,
                dns.rdatatype.CNAME,
                self_example_dname.to_text(),
            )
            qctx.response.answer.append(cname_rrset)

        yield DnsResponseSend(qctx.response)


def main() -> None:
    server = AsyncDnsServer(acknowledge_manual_dname_handling=True, default_aa=True)
    server.install_response_handlers([CnameThenDnameHandler(), Cve202125215()])
    server.run()


if __name__ == "__main__":
    main()
