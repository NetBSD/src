#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0

from collections.abc import AsyncGenerator
from dataclasses import dataclass
from datetime import datetime, timedelta, timezone
from pathlib import Path

import base64
import json

from cryptography.hazmat.primitives import serialization

import dns.dnssec
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
PARENT = "p22.hack."
CHILD = f"c.{PARENT}"
CHILD_NS = f"ns.{CHILD}"
VICTIM = f"victim.{PARENT}"
AAC = f"aac.{PARENT}"
CHILD_NEXT = f"ns1.{PARENT}"
TRIGGER = f"www.{CHILD}"
PRIME_NX = f"0.{PARENT}"

CHILD_A = "192.0.2.50"
CHILD_NS_A = "10.53.0.2"
VICTIM_A = "192.0.2.99"
AAC_A = "192.0.2.77"


@dataclass(frozen=True)
class Key:
    zone: dns.name.Name
    private_key: object
    dnskey: dns.rdata.Rdata


def name(text: str) -> dns.name.Name:
    return dns.name.from_text(text)


def load_keys() -> dict[str, Key]:
    path = Path("keys.json")
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
        keys[zone] = Key(name(zone), private_key, dnskey)

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


def add_dnskey(response: dns.message.Message, zone: str, key: Key) -> None:
    add_signed(response.answer, rrset_from_rdata(zone, key.dnskey), key)


def soa_rrset(zone: str) -> dns.rrset.RRset:
    return rrset(
        zone,
        dns.rdatatype.SOA,
        f"ns.{zone} hostmaster.{zone} 1 3600 600 86400 300",
    )


def nsec_rrset(owner: str, next_name: str, *types: str) -> dns.rrset.RRset:
    return rrset(owner, dns.rdatatype.NSEC, f"{next_name} {' '.join(types)}")


def nsec_apex() -> dns.rrset.RRset:
    return nsec_rrset(PARENT, AAC, "NS", "SOA", "RRSIG", "NSEC", "DNSKEY")


def nsec_deleg_child() -> dns.rrset.RRset:
    return nsec_rrset(CHILD, CHILD_NEXT, "NS", "RRSIG", "NSEC")


def stuffed_ent_nsec() -> dns.rrset.RRset:
    return nsec_rrset(f"t.{PARENT}", f"sub.{VICTIM}", "A", "RRSIG", "NSEC")


def stuffed_range_nsec() -> dns.rrset.RRset:
    return nsec_rrset(f"aab.{PARENT}", f"az.{PARENT}", "A", "RRSIG", "NSEC")


def garbage_rrsig(covered: dns.rrset.RRset, signer: Key) -> dns.rrset.RRset:
    now = datetime.now(timezone.utc)
    inception = (now - timedelta(hours=1)).strftime("%Y%m%d%H%M%S")
    expiration = (now + timedelta(days=1)).strftime("%Y%m%d%H%M%S")
    signer_name = signer.zone.to_text()
    labels = len(covered.name.labels) - 1
    key_tag = dns.dnssec.key_id(signer.dnskey)
    signature = base64.b64encode(bytes(64)).decode("ascii")
    text = (
        f"{dns.rdatatype.to_text(covered.rdtype)} "
        f"{signer.dnskey.algorithm} {labels} {covered.ttl} "
        f"{expiration} {inception} {key_tag} {signer_name} {signature}"
    )
    rdata = dns.rdata.from_text(dns.rdataclass.IN, dns.rdatatype.RRSIG, text)
    return dns.rrset.from_rdata(covered.name, covered.ttl, rdata)


def add_garbage_signed_nsec(
    section: list[dns.rrset.RRset], covered: dns.rrset.RRset, signer: Key
) -> None:
    section.append(covered)
    section.append(garbage_rrsig(covered, signer))


def prepare_response(qctx: QueryContext) -> dns.message.Message:
    qctx.prepare_new_response(with_zone_data=False)
    qctx.response.set_rcode(dns.rcode.NOERROR)
    return qctx.response


def add_parent_negative(
    response: dns.message.Message, signer: Key, nsec: dns.rrset.RRset
) -> None:
    add_signed(response.authority, soa_rrset(PARENT), signer)
    add_signed(response.authority, nsec, signer)


class ParentHandler(ResponseHandler):
    def __init__(self, keys: dict[str, Key]) -> None:
        self.keys = keys
        self.parent = name(PARENT)
        self.child = name(CHILD)
        self.victim = name(VICTIM)
        self.aac = name(AAC)
        self.prime_nx = name(PRIME_NX)

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qname.is_subdomain(self.parent)

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        response = prepare_response(qctx)
        parent_key = self.keys[PARENT]

        if qctx.qname == self.parent and qctx.qtype == dns.rdatatype.DNSKEY:
            add_dnskey(response, PARENT, parent_key)
        elif qctx.qname == self.parent and qctx.qtype == dns.rdatatype.SOA:
            add_signed(response.answer, soa_rrset(PARENT), parent_key)
        elif qctx.qname == self.parent:
            add_parent_negative(response, parent_key, nsec_apex())
        elif qctx.qname == self.child and qctx.qtype == dns.rdatatype.DS:
            add_parent_negative(response, parent_key, nsec_deleg_child())
        elif qctx.qname == self.prime_nx:
            response.set_rcode(dns.rcode.NXDOMAIN)
            add_parent_negative(response, parent_key, nsec_apex())
        elif qctx.qname == self.child or qctx.qname.is_subdomain(self.child):
            response.authority.append(rrset(CHILD, dns.rdatatype.NS, CHILD_NS))
            add_signed(response.authority, nsec_deleg_child(), parent_key)
            add_garbage_signed_nsec(response.authority, stuffed_ent_nsec(), parent_key)
            add_garbage_signed_nsec(
                response.authority, stuffed_range_nsec(), parent_key
            )
            response.additional.append(rrset(CHILD_NS, dns.rdatatype.A, CHILD_NS_A))
        elif qctx.qname == self.victim and qctx.qtype == dns.rdatatype.A:
            add_signed(
                response.answer,
                rrset(VICTIM, dns.rdatatype.A, VICTIM_A),
                parent_key,
            )
        elif qctx.qname == self.aac and qctx.qtype == dns.rdatatype.A:
            add_signed(response.answer, rrset(AAC, dns.rdatatype.A, AAC_A), parent_key)
        else:
            response.set_rcode(dns.rcode.NXDOMAIN)
            add_parent_negative(response, parent_key, nsec_apex())

        yield DnsResponseSend(response, authoritative=True)


def main() -> None:
    server = AsyncDnsServer(default_aa=True)
    server.install_response_handlers(ParentHandler(load_keys()))
    server.run()


if __name__ == "__main__":
    main()
