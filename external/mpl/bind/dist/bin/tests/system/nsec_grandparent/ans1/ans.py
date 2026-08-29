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
PARENT = "p031.test."
CHILD = f"c.{PARENT}"
GRANDCHILD = f"grand.{CHILD}"
GRANDCHILD3 = f"grand3.{CHILD}"
ATTACK = f"www-bind.{GRANDCHILD}"
ATTACK3 = f"www-bind.{GRANDCHILD3}"
FORGED_A = "6.6.6.60"
CHILD_DS = "12345 13 2 abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789"


@dataclass(frozen=True)
class Key:
    zone: dns.name.Name
    private_key: object
    dnskey: dns.rdata.Rdata


def name(text: str) -> dns.name.Name:
    return dns.name.from_text(text)


def load_key() -> Key:
    path = Path(__file__).resolve().parent / "keys.json"
    with path.open(encoding="utf-8") as keys_file:
        raw_key = json.load(keys_file)[PARENT]

    private_key = serialization.load_pem_private_key(
        raw_key["private_pem"].encode("ascii"),
        password=None,
    )
    dnskey = dns.rdata.from_text(
        dns.rdataclass.IN, dns.rdatatype.DNSKEY, raw_key["dnskey"]
    )
    return Key(name(PARENT), private_key, dnskey)


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


def soa_rrset() -> dns.rrset.RRset:
    return rrset(
        PARENT,
        dns.rdatatype.SOA,
        f"ns.{PARENT} hostmaster.{PARENT} 1 3600 600 86400 300",
    )


def nsec_rrset(owner: str, next_name: str, *types: str) -> dns.rrset.RRset:
    return rrset(owner, dns.rdatatype.NSEC, f"{next_name} {' '.join(types)}")


def child_ds_rrset() -> dns.rrset.RRset:
    return rrset(CHILD, dns.rdatatype.DS, CHILD_DS)


def grandchild_nsec_lie() -> dns.rrset.RRset:
    return nsec_rrset(GRANDCHILD, f"grandz.{CHILD}", "NS", "RRSIG", "NSEC")


def grandchild3_nsec3_lie() -> dns.rrset.RRset:
    # An NSEC3 owned by the grandparent zone P that matches the hash of the
    # grandchild name and shows an (insecure) delegation: NS bit set, DS bit
    # clear.  Same forgery as grandchild_nsec_lie(), but expressed as NSEC3 so
    # that the resolver reaches is_insecure_referral()'s trynsec3 arm.
    digest = dns.dnssec.nsec3_hash(name(GRANDCHILD3), None, 0, 1).lower()
    owner = f"{digest}.{PARENT}"
    return rrset(owner, dns.rdatatype.NSEC3, f"1 0 0 - {digest} NS")


def add_parent_nodata(
    response: dns.message.Message, parent_key: Key, nsec: dns.rrset.RRset
) -> None:
    add_signed(response.authority, soa_rrset(), parent_key)
    add_signed(response.authority, nsec, parent_key)


def prepare_response(qctx: QueryContext) -> dns.message.Message:
    qctx.prepare_new_response(with_zone_data=False)
    qctx.response.flags |= dns.flags.AA
    qctx.response.set_rcode(dns.rcode.NOERROR)
    return qctx.response


class GrandparentNsecHandler(ResponseHandler):
    def __init__(self, parent_key: Key) -> None:
        self.parent_key = parent_key
        self.parent = name(PARENT)
        self.child = name(CHILD)
        self.grandchild = name(GRANDCHILD)
        self.grandchild3 = name(GRANDCHILD3)

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qname.is_subdomain(self.parent)

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        response = prepare_response(qctx)

        if qctx.qname == self.parent and qctx.qtype == dns.rdatatype.DNSKEY:
            # Priming, parent DNSKEY
            add_signed(
                response.answer,
                rrset_from_rdata(PARENT, self.parent_key.dnskey),
                self.parent_key,
            )
        elif qctx.qname == self.parent and qctx.qtype == dns.rdatatype.SOA:
            # Priming, parent SOA
            add_signed(response.answer, soa_rrset(), self.parent_key)
        elif qctx.qname == self.child and qctx.qtype == dns.rdatatype.DS:
            # Priming, child DS
            add_signed(response.answer, child_ds_rrset(), self.parent_key)
        elif qctx.qname == self.grandchild and qctx.qtype == dns.rdatatype.DS:
            # Forge no data for grand child DS (NSEC variant)
            add_parent_nodata(response, self.parent_key, grandchild_nsec_lie())
        elif qctx.qname == self.grandchild3 and qctx.qtype == dns.rdatatype.DS:
            # Forge no data for grand child DS (NSEC3 variant)
            add_parent_nodata(response, self.parent_key, grandchild3_nsec3_lie())
        elif (
            qctx.qname.is_subdomain(self.grandchild)
            or qctx.qname.is_subdomain(self.grandchild3)
        ) and qctx.qtype == dns.rdatatype.A:
            # Attack query
            response.answer.append(
                rrset(qctx.qname.to_text(), dns.rdatatype.A, FORGED_A)
            )
        else:
            response.set_rcode(dns.rcode.NXDOMAIN)

        yield DnsResponseSend(response, authoritative=True)


def main() -> None:
    server = AsyncDnsServer(default_aa=True)
    server.install_response_handlers(GrandparentNsecHandler(load_key()))
    server.run()


if __name__ == "__main__":
    main()
