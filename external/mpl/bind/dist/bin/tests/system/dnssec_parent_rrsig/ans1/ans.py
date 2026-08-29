#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0

from collections.abc import AsyncGenerator
from dataclasses import dataclass
from pathlib import Path

import json

from cryptography.hazmat.primitives import serialization

import dns.dnssec
import dns.flags
import dns.message
import dns.name
import dns.rcode
import dns.rdata
import dns.rdataclass
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsResponseSend,
    QueryContext,
    ResponseHandler,
)

TTL = 300
PARENT = "f044.test."
CHILD = f"child.{PARENT}"
QUERY = f"q.{PARENT}"
SERVICE = f"svc.{CHILD}"
FORGED_A = "6.6.6.6"
LEGIT_A = "192.0.2.111"


@dataclass(frozen=True)
class Key:
    zone: dns.name.Name
    private_key: object
    dnskey: dns.rdata.Rdata
    ds: dns.rdata.Rdata


def name(text: str) -> dns.name.Name:
    return dns.name.from_text(text)


def load_keys() -> dict[str, Key]:
    path = Path(__file__).resolve().parent / "keys.json"
    with path.open(encoding="utf-8") as keys_file:
        raw_keys = json.load(keys_file)

    keys = {}
    for zone, raw_key in raw_keys.items():
        private_key = serialization.load_pem_private_key(
            raw_key["private_pem"].encode("ascii"),
            password=None,
        )
        dnskey = dns.rdata.from_text(
            dns.rdataclass.IN, dns.rdatatype.DNSKEY, raw_key["dnskey"]
        )
        ds = dns.rdata.from_text(dns.rdataclass.IN, dns.rdatatype.DS, raw_key["ds"])
        keys[zone] = Key(name(zone), private_key, dnskey, ds)
    return keys


def rrset(owner: str, rdtype: dns.rdatatype.RdataType, *rdatas: str) -> dns.rrset.RRset:
    return dns.rrset.from_text(owner, TTL, dns.rdataclass.IN, rdtype, *rdatas)


def rrset_from_rdata(owner: str, rdata: dns.rdata.Rdata) -> dns.rrset.RRset:
    return dns.rrset.from_rdata(name(owner), TTL, rdata)


def add_signed(
    section: list[dns.rrset.RRset], covered: dns.rrset.RRset, signer: Key
) -> None:
    rrsig = dns.dnssec.sign(
        covered,
        signer.private_key,
        signer.zone,
        signer.dnskey,
        lifetime=86400,
        verify=True,
    )
    section.append(covered)
    section.append(dns.rrset.from_rdata(covered.name, covered.ttl, rrsig))


def soa_rrset(zone: str) -> dns.rrset.RRset:
    return rrset(
        zone,
        dns.rdatatype.SOA,
        f"ns.{zone} hostmaster.{zone} 1 3600 600 86400 300",
    )


def add_dnskey(response: dns.message.Message, zone: str, key: Key) -> None:
    add_signed(response.answer, rrset_from_rdata(zone, key.dnskey), key)


def add_nodata(response: dns.message.Message, zone: str, key: Key) -> None:
    add_signed(response.authority, soa_rrset(zone), key)


def add_parent_signed_carrier(response: dns.message.Message, parent: Key) -> None:
    # ;; ANSWER SECTION:
    # q.f044.hack.          300 IN MX    10 svc.child.f044.hack.
    # q.f044.hack.          300 IN RRSIG MX 13 3 300 ... 28052 f044.hack. ...
    # ;; ADDITIONAL SECTION:
    # svc.child.f044.hack.  300 IN A     6.6.6.6 (forged)
    # svc.child.f044.hack.  300 IN RRSIG ... f044.hack. (signer is the parent)
    add_signed(response.answer, rrset(QUERY, dns.rdatatype.MX, f"10 {SERVICE}"), parent)
    add_signed(response.additional, rrset(SERVICE, dns.rdatatype.A, FORGED_A), parent)


class AncestorAdditionalHandler(ResponseHandler):
    def __init__(self, keys: dict[str, Key]) -> None:
        self.keys = keys
        self.parent = name(PARENT)
        self.child = name(CHILD)
        self.query = name(QUERY)
        self.service = name(SERVICE)

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qname.is_subdomain(self.parent)

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.prepare_new_response(with_zone_data=False)
        qctx.response.flags |= dns.flags.AA
        qctx.response.set_rcode(dns.rcode.NOERROR)

        parent_key = self.keys[PARENT]
        child_key = self.keys[CHILD]

        if qctx.qname == self.parent and qctx.qtype == dns.rdatatype.DNSKEY:
            # Priming, parent DNSKEY
            add_dnskey(qctx.response, PARENT, parent_key)
        elif qctx.qname == self.parent and qctx.qtype == dns.rdatatype.SOA:
            # Priming, parent SOA
            add_signed(qctx.response.answer, soa_rrset(PARENT), parent_key)
        elif qctx.qname == self.child and qctx.qtype == dns.rdatatype.DS:
            # Priming, child DS
            add_signed(
                qctx.response.answer,
                rrset_from_rdata(CHILD, child_key.ds),
                parent_key,
            )
        elif qctx.qname == self.child and qctx.qtype == dns.rdatatype.DNSKEY:
            # Priming, child DNSKEY
            add_dnskey(qctx.response, CHILD, child_key)
        elif qctx.qname == self.child and qctx.qtype == dns.rdatatype.SOA:
            # Priming, child SOA (baseline)
            add_signed(qctx.response.answer, soa_rrset(CHILD), child_key)
        elif qctx.qname == self.query and qctx.qtype == dns.rdatatype.MX:
            # Malicious response
            add_parent_signed_carrier(qctx.response, parent_key)
        elif qctx.qname == self.service and qctx.qtype == dns.rdatatype.A:
            # Legit response
            add_signed(
                qctx.response.answer,
                rrset(SERVICE, dns.rdatatype.A, LEGIT_A),
                child_key,
            )
        elif qctx.qname.is_subdomain(self.child):
            # The rest is NODATA
            add_nodata(qctx.response, CHILD, child_key)
        else:
            # The rest is NODATA
            add_nodata(qctx.response, PARENT, parent_key)

        yield DnsResponseSend(qctx.response, authoritative=True)


def main() -> None:
    server = AsyncDnsServer(default_aa=True)
    server.install_response_handlers(AncestorAdditionalHandler(load_keys()))
    server.run()


if __name__ == "__main__":
    main()
