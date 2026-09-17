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
import dns.name
import dns.rcode
import dns.rdata
import dns.rdataclass
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsResponseSend,
    DomainHandler,
    QueryContext,
)

TTL = 300
VICTIM = "victim.example."
ATTACKER = "abc.example."
ATTACK = f"foo.{VICTIM}"
VALID_NXDOMAIN = f"bar.{ATTACKER}"
CONTROL = f"wild1.{VICTIM}"
CONTROL_NODATA = f"wild2.{VICTIM}"
WILDCARD = f"*.{VICTIM}"
VICTIM_NSEC_OWNER = f"a.{VICTIM}"
VICTIM_NSEC_NEXT = f"z.{VICTIM}"
ATTACKER_NSEC_OWNER = f"z.{ATTACKER}"
ATTACKER_VALID_NSEC_OWNER = f"a.{ATTACKER}"
CONTROL_A = "192.0.2.1"


@dataclass(frozen=True)
class Key:
    zone: dns.name.Name
    private_key: object
    dnskey: dns.rdata.Rdata


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
        keys[zone] = Key(name(zone), private_key, dnskey)

    return keys


def rrset(owner: str, rdtype: dns.rdatatype.RdataType, rdata: str) -> dns.rrset.RRset:
    return dns.rrset.from_text(owner, TTL, dns.rdataclass.IN, rdtype, rdata)


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


def add_dnskey(response, key: Key) -> None:
    dnskey = dns.rrset.from_rdata(key.zone, TTL, key.dnskey)
    add_signed(response.answer, dnskey, key)


def soa(zone: str) -> dns.rrset.RRset:
    return rrset(
        zone,
        dns.rdatatype.SOA,
        f"ns.{zone} hostmaster.{zone} 1 3600 600 86400 300",
    )


def nsec(owner: str, next_name: str) -> dns.rrset.RRset:
    return rrset(owner, dns.rdatatype.NSEC, f"{next_name} A RRSIG NSEC")


def add_wildcard_answer(response, key: Key) -> None:
    wildcard = rrset(WILDCARD, dns.rdatatype.A, CONTROL_A)
    rrsig = dns.dnssec.sign(
        wildcard,
        key.private_key,
        key.zone,
        key.dnskey,
        lifetime=86400,
        verify=True,
    )
    response.answer.append(rrset(CONTROL, dns.rdatatype.A, CONTROL_A))
    response.answer.append(dns.rrset.from_rdata(name(CONTROL), TTL, rrsig))


class NsecWildcardWrongZoneHandler(DomainHandler):
    domains = [VICTIM, ATTACKER]

    def __init__(self, keys: dict[str, Key]) -> None:
        super().__init__()
        self.keys = keys
        self.victim = keys[VICTIM]
        self.attacker = keys[ATTACKER]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.prepare_new_response(with_zone_data=False)
        qctx.response.flags |= dns.flags.AA
        qctx.response.set_rcode(dns.rcode.NOERROR)

        if qctx.qtype == dns.rdatatype.DNSKEY and qctx.qname in {
            self.victim.zone,
            self.attacker.zone,
        }:
            add_dnskey(qctx.response, self.keys[qctx.qname.to_text()])
        elif qctx.qtype == dns.rdatatype.SOA and qctx.qname in {
            self.victim.zone,
            self.attacker.zone,
        }:
            zone = qctx.qname.to_text()
            add_signed(qctx.response.answer, soa(zone), self.keys[zone])
        elif qctx.qname == name(ATTACK) and qctx.qtype == dns.rdatatype.A:
            qctx.response.set_rcode(dns.rcode.NXDOMAIN)
            add_signed(qctx.response.authority, soa(VICTIM), self.victim)

            # This secure terminal NSEC from an unrelated zone sorts across
            # the victim wildcard and used to be accepted as NOWILDCARD.
            add_signed(
                qctx.response.authority,
                nsec(ATTACKER_NSEC_OWNER, ATTACKER),
                self.attacker,
            )
            add_signed(
                qctx.response.authority,
                nsec(VICTIM_NSEC_OWNER, VICTIM_NSEC_NEXT),
                self.victim,
            )
        elif qctx.qname == name(VALID_NXDOMAIN) and qctx.qtype == dns.rdatatype.A:
            qctx.response.set_rcode(dns.rcode.NXDOMAIN)
            add_signed(qctx.response.authority, soa(ATTACKER), self.attacker)
            add_signed(
                qctx.response.authority,
                nsec(ATTACKER_VALID_NSEC_OWNER, ATTACKER_NSEC_OWNER),
                self.attacker,
            )
            add_signed(
                qctx.response.authority,
                nsec(ATTACKER, ATTACKER_VALID_NSEC_OWNER),
                self.attacker,
            )
        elif qctx.qname == name(CONTROL) and qctx.qtype == dns.rdatatype.A:
            add_wildcard_answer(qctx.response, self.victim)
            add_signed(
                qctx.response.authority,
                nsec(VICTIM_NSEC_OWNER, VICTIM_NSEC_NEXT),
                self.victim,
            )
        elif qctx.qname == name(CONTROL_NODATA) and qctx.qtype == dns.rdatatype.AAAA:
            add_signed(qctx.response.authority, soa(VICTIM), self.victim)
            add_signed(
                qctx.response.authority,
                nsec(VICTIM_NSEC_OWNER, VICTIM_NSEC_NEXT),
                self.victim,
            )
            add_signed(
                qctx.response.authority,
                nsec(WILDCARD, VICTIM_NSEC_OWNER),
                self.victim,
            )
        else:
            qctx.response.set_rcode(dns.rcode.NXDOMAIN)
            zone = VICTIM if qctx.qname.is_subdomain(self.victim.zone) else ATTACKER
            add_signed(qctx.response.authority, soa(zone), self.keys[zone])

        yield DnsResponseSend(qctx.response, authoritative=True)


def main() -> None:
    server = AsyncDnsServer(default_aa=True)
    server.install_response_handler(NsecWildcardWrongZoneHandler(load_keys()))
    server.run()


if __name__ == "__main__":
    main()
