#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0

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

from isctest.asyncserver import QueryContext

TTL = 300


@dataclass(frozen=True)
class Key:
    zone: dns.name.Name
    private_key: object
    dnskey: dns.rdata.Rdata
    ds: dns.rdata.Rdata


def name(text: str) -> dns.name.Name:
    return dns.name.from_text(text)


def load_keys() -> dict[str, Key]:
    keys = {}

    path = Path(".") / "keys.json"
    if not path.exists():
        return keys

    with path.open(encoding="utf-8") as keys_file:
        raw_keys = json.load(keys_file)

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
    return dns.rrset.from_rdata(owner, TTL, rdata)


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


def soa_rrset(zone) -> dns.rrset.RRset:
    return rrset(
        zone,
        dns.rdatatype.SOA,
        f"ns.{zone} hostmaster.{zone} 1 3600 600 86400 300",
    )


def prepare_response(qctx: QueryContext) -> dns.message.Message:
    qctx.prepare_new_response(with_zone_data=False)
    qctx.response.flags |= dns.flags.AA
    qctx.response.set_rcode(dns.rcode.NOERROR)
    return qctx.response
