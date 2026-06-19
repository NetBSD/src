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

from collections.abc import AsyncGenerator

import dns.flags
import dns.name
import dns.rcode
import dns.rdatatype
import dns.rrset
import dns.zone

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsResponseSend,
    DomainHandler,
    QueryContext,
)

# 'example.' answers DNSKEY/NSEC/NSEC3/RRSIG queries with a CNAME (the
# meta-types whose CNAME answer the resolver and validator must cope with).
EXAMPLE = dns.zone.from_file("example.signed.db", origin="example.", relativize=False)

# 'secure.' is served faithfully but answers DS queries with an unsigned
# CNAME: the input that drove the validator's insecurity proof into a
# self-join deadlock (GL#5878).  Served correctly otherwise so the resolver
# can validate down to the zone and reach the DS query.
SECURE = dns.zone.from_file("secure.signed.db", origin="secure.", relativize=False)


def _append_rrset_with_rrsig(
    zone: dns.zone.Zone,
    section: list,
    name: dns.name.Name,
    qclass: int,
    rdtype: int,
    rds,
) -> None:
    rrset = dns.rrset.RRset(name, qclass, rdtype)
    rrset.update(rds)
    section.append(rrset)

    node = zone.get_node(name)
    if node is None:
        return
    rrsig_rds = node.get_rdataset(qclass, dns.rdatatype.RRSIG, covers=rdtype)
    if rrsig_rds is None:
        return
    rrsig_rrset = dns.rrset.RRset(name, qclass, dns.rdatatype.RRSIG, covers=rdtype)
    rrsig_rrset.update(rrsig_rds)
    section.append(rrsig_rrset)


class CnameZoneHandler(DomainHandler):
    """Serve a signed zone faithfully, but answer queries for the configured
    rdata types with a CNAME instead of the real records."""

    def __init__(self, zone: dns.zone.Zone, cname_qtypes) -> None:
        self.zone = zone
        self.cname_qtypes = frozenset(cname_qtypes)
        super().__init__()

    @property
    def domains(self) -> list:
        return [self.zone.origin.to_text()]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.prepare_new_response(with_zone_data=False)
        qctx.response.flags |= dns.flags.AA

        if qctx.qtype in self.cname_qtypes:
            cname_target = f"cname-target.{qctx.qname.to_text()}"
            cname_rrset = dns.rrset.from_text(
                qctx.qname,
                300,
                qctx.qclass,
                dns.rdatatype.CNAME,
                cname_target,
            )
            qctx.response.answer.append(cname_rrset)
            yield DnsResponseSend(qctx.response)
            return

        node = self.zone.get_node(qctx.qname)
        soa_rds = self.zone.get_rdataset(self.zone.origin, dns.rdatatype.SOA)

        if node is None:
            qctx.response.set_rcode(dns.rcode.NXDOMAIN)
            _append_rrset_with_rrsig(
                self.zone,
                qctx.response.authority,
                self.zone.origin,
                qctx.qclass,
                dns.rdatatype.SOA,
                soa_rds,
            )
            yield DnsResponseSend(qctx.response)
            return

        rds = node.get_rdataset(qctx.qclass, qctx.qtype)
        if rds is None:
            _append_rrset_with_rrsig(
                self.zone,
                qctx.response.authority,
                self.zone.origin,
                qctx.qclass,
                dns.rdatatype.SOA,
                soa_rds,
            )
            yield DnsResponseSend(qctx.response)
            return

        _append_rrset_with_rrsig(
            self.zone,
            qctx.response.answer,
            qctx.qname,
            qctx.qclass,
            qctx.qtype,
            rds,
        )
        yield DnsResponseSend(qctx.response)


class LoneRecordHandler(DomainHandler):
    """Answer any query with a single unrelated A record (no RRSIG and no
    alias).  An RRSIG query is handled by the resolver as a subset of ANY,
    and such an answer used to be dropped entirely, leaving the fetch
    waiting for a validator that was never started."""

    domains = ["lone-a.example."]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.prepare_new_response(with_zone_data=False)
        qctx.response.flags |= dns.flags.AA
        a_rrset = dns.rrset.from_text(
            qctx.qname, 300, qctx.qclass, dns.rdatatype.A, "192.0.2.1"
        )
        qctx.response.answer.append(a_rrset)
        yield DnsResponseSend(qctx.response)


def main() -> None:
    server = AsyncDnsServer(default_rcode=dns.rcode.NOERROR, default_aa=True)
    server.install_response_handlers(
        LoneRecordHandler(),
        CnameZoneHandler(
            EXAMPLE,
            {
                dns.rdatatype.DNSKEY,
                dns.rdatatype.NSEC,
                dns.rdatatype.NSEC3,
                dns.rdatatype.RRSIG,
            },
        ),
        CnameZoneHandler(SECURE, {dns.rdatatype.DS}),
    )
    server.run()


if __name__ == "__main__":
    main()
