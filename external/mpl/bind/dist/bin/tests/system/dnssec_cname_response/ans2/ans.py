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
import dns.rdataset
import dns.rdatatype
import dns.rrset
import dns.zone

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsResponseSend,
    DomainHandler,
    QueryContext,
    ResponseHandler,
)

# 'example.' answers DNSKEY/NSEC/NSEC3/RRSIG queries with a CNAME (the
# meta-types whose CNAME answer the resolver and validator must cope with).
EXAMPLE = dns.zone.from_file("example.signed.db", origin="example.", relativize=False)

# 'secure.' is served faithfully but answers DS queries with an unsigned
# CNAME: the input that drove the validator's insecurity proof into a
# self-join deadlock (GL#5878).  Served correctly otherwise so the resolver
# can validate down to the zone and reach the DS query.
SECURE = dns.zone.from_file("secure.signed.db", origin="secure.", relativize=False)

# 'stuffed.' is served faithfully for existing data, but NXDOMAIN answers
# contain every NSEC3 RRset in the zone to exercise negative-proof filtering.
STUFFED = dns.zone.from_file("stuffed.signed.zone", origin="stuffed.", relativize=False)


def a(owner: dns.name.Name, address: str) -> dns.rrset.RRset:
    return dns.rrset.from_text(owner, 300, dns.rdataclass.IN, dns.rdatatype.A, address)


def cname(owner: dns.name.Name, target: str) -> dns.rrset.RRset:
    return dns.rrset.from_text(
        owner, 300, dns.rdataclass.IN, dns.rdatatype.CNAME, target
    )


def rrset_with_rrsig(
    zone: dns.zone.Zone,
    name: dns.name.Name,
    rdtype: dns.rdatatype.RdataType,
) -> list[dns.rrset.RRset]:
    covered = zone.get_rrset(name, rdtype)
    assert covered is not None
    rrsets = [covered]
    rrsig = zone.get_rrset(name, dns.rdatatype.RRSIG, covers=rdtype)
    if rrsig is not None:
        rrsets.append(rrsig)
    return rrsets


class LoneAHandler(DomainHandler):
    """
    Answer any query with a single unrelated A record (no RRSIG and no
    alias).  An RRSIG query is handled by the resolver as a subset of ANY,
    and such an answer used to be dropped entirely, leaving the fetch waiting
    for a validator that was never started.
    """

    domains = ["lone-a.example."]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.prepare_new_response(with_zone_data=False)
        qctx.response.answer.append(a(qctx.qname, "192.0.2.1"))
        yield DnsResponseSend(qctx.response)


class CnameHandler(ResponseHandler):
    """
    Answer with a CNAME instead of the real records.
    """

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.prepare_new_response(with_zone_data=False)
        target = f"cname-target.{qctx.qname.to_text()}"
        qctx.response.answer.append(cname(qctx.qname, target))
        yield DnsResponseSend(qctx.response)


class ExampleMetatypeCnameHandler(DomainHandler, CnameHandler):
    domains = ["example."]
    _qtypes = frozenset(
        {
            dns.rdatatype.DNSKEY,
            dns.rdatatype.NSEC,
            dns.rdatatype.NSEC3,
            dns.rdatatype.RRSIG,
        }
    )

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qtype in self._qtypes and super().match(qctx)


class SecureDsCnameHandler(DomainHandler, CnameHandler):
    domains = ["secure."]

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qtype == dns.rdatatype.DS and super().match(qctx)


class SignedZoneHandler(DomainHandler):
    """
    Serve a signed zone faithfully.
    """

    def __init__(self, zone: dns.zone.Zone) -> None:
        self._zone = zone
        super().__init__()

    @property
    def domains(self) -> list[str]:
        return [self._zone.origin.to_text()]

    def _soa(self) -> list[dns.rrset.RRset]:
        return rrset_with_rrsig(self._zone, self._zone.origin, dns.rdatatype.SOA)

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.prepare_new_response(with_zone_data=False)

        node = self._zone.get_node(qctx.qname)
        if node is None:
            qctx.response.set_rcode(dns.rcode.NXDOMAIN)
            qctx.response.authority.extend(self._soa())
            yield DnsResponseSend(qctx.response)
            return

        rds = node.get_rdataset(dns.rdataclass.IN, qctx.qtype)
        if rds is None:
            qctx.response.authority.extend(self._soa())
            yield DnsResponseSend(qctx.response)
            return

        qctx.response.answer.extend(
            rrset_with_rrsig(self._zone, qctx.qname, qctx.qtype)
        )
        yield DnsResponseSend(qctx.response)


class StuffedNxdomainHandler(DomainHandler):
    """
    Answer NXDOMAIN with every NSEC3 RRset from a signed zone.
    """

    def __init__(self, zone: dns.zone.Zone) -> None:
        self._zone = zone
        super().__init__()
        self._nsec3_authority = self._build_authority()

    @property
    def domains(self) -> list[str]:
        return [self._zone.origin.to_text()]

    def _build_authority(self) -> list[dns.rrset.RRset]:
        authority = rrset_with_rrsig(self._zone, self._zone.origin, dns.rdatatype.SOA)
        for name, _ in self._zone.iterate_rdatasets(dns.rdatatype.NSEC3):
            authority.extend(rrset_with_rrsig(self._zone, name, dns.rdatatype.NSEC3))
        return authority

    def match(self, qctx: QueryContext) -> bool:
        return super().match(qctx) and self._answer_rds(qctx) is None

    def _answer_rds(self, qctx: QueryContext) -> dns.rdataset.Rdataset | None:
        node = self._zone.get_node(qctx.qname)
        if node is None:
            return None
        return node.get_rdataset(dns.rdataclass.IN, qctx.qtype)

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.prepare_new_response(with_zone_data=False)
        qctx.response.set_rcode(dns.rcode.NXDOMAIN)
        qctx.response.authority.extend(self._nsec3_authority)
        yield DnsResponseSend(qctx.response)


def main() -> None:
    server = AsyncDnsServer(default_rcode=dns.rcode.NOERROR, default_aa=True)
    server.install_response_handlers(
        LoneAHandler(),
        ExampleMetatypeCnameHandler(),
        SignedZoneHandler(EXAMPLE),
        SecureDsCnameHandler(),
        SignedZoneHandler(SECURE),
        StuffedNxdomainHandler(STUFFED),
        SignedZoneHandler(STUFFED),
    )
    server.run()


if __name__ == "__main__":
    main()
