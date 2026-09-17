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
# The attacker-controlled sibling zone: a genuine, correctly delegated and
# signed zone under the same parent as CHILD.  Its crafted NSEC3 is made
# to sort first in the ncache via the salt choice below (see _ordering_salts).
SIBLING = f"attacker.{PARENT}"
# #5967 (grandparent-zone NSEC/NSEC3): a grandchild whose forged NSEC/NSEC3
# insecure-delegation proof is owned by its grandparent zone.
GRANDCHILD = f"grand.{CHILD}"
GRANDCHILD3 = f"grand3.{CHILD}"
# #6234 (sibling-zone NSEC3): a grandchild whose forged NSEC3
# insecure-delegation proof is owned by an unrelated but correctly delegated
# and signed sibling zone.
GRANDCHILD3_SIBLING = f"grandsib.{CHILD}"
# #6321 (mixed-signer RRSIG): grandchildren whose grandparent-signed NSEC
# forgery also carries a dummy RRSIG naming the NSEC owner itself as signer,
# in either order relative to the genuine one.
GRANDCHILD_DUMMY_FIRST = f"grand-dummy-first.{CHILD}"
GRANDCHILD_DUMMY_LAST = f"grand-dummy-last.{CHILD}"
# RRSIG count cap: grandchildren whose grandparent-signed NSEC forgery carries
# as many same-signer RRSIGs as ns2 allows validations per fetch, or one fewer.
GRANDCHILD_TOO_MANY = f"grand-too-many.{CHILD}"
GRANDCHILD_ALMOST_TOO_MANY = f"grand-almost-too-many.{CHILD}"
# The names under attack.
ATTACK = f"www-bind.{GRANDCHILD}"
ATTACK3 = f"www-bind.{GRANDCHILD3}"
FORGED_A = "6.6.6.60"
# Not a DNSSEC algorithm; the validator skips RRSIGs using it as unsupported
# rather than rejecting them, which is what the mixed-signer forgery needs.
DUMMY_ALGORITHM = 0
# ns2's max-validations-per-fetch; keep in sync with the test module.
MAX_VALIDATIONS_PER_FETCH = 16


@dataclass(frozen=True)
class Key:
    zone: dns.name.Name
    private_key: object
    dnskey: dns.rdata.Rdata


def name(text: str) -> dns.name.Name:
    return dns.name.from_text(text)


def _ordering_salts(qname: str) -> tuple[str, str]:
    # Pick NSEC3 salts (as hex) so the sibling's owner hash sorts strictly
    # before the child's.  The ncache slab is ordered by wire-format owner
    # name -- i.e. by the leftmost hash label -- and is_insecure_referral()'s
    # trynsec3 arm returns on the *first* exact hash match, so the sibling's
    # crafted NS-set NSEC3 must precede the child's genuine NS-clear NODATA
    # proof.  Deterministic search over one-octet salts keeps this true no
    # matter what GRANDCHILD3_SIBLING is named.
    hashes = sorted(
        (dns.dnssec.nsec3_hash(name(qname), f"{i:02X}", 0, 1).lower(), f"{i:02X}")
        for i in range(256)
    )
    return hashes[0][1], hashes[-1][1]


# Sibling salt yields the smallest hash, child salt the largest.
SIBLING_SALT, CHILD_SALT = _ordering_salts(GRANDCHILD3_SIBLING)


def load_keys() -> dict[str, Key]:
    path = Path(__file__).resolve().parent / "keys.json"
    with path.open(encoding="utf-8") as keys_file:
        raw = json.load(keys_file)

    keys: dict[str, Key] = {}
    for zone, raw_key in raw.items():
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


def sign(covered: dns.rrset.RRset, signer: Key) -> dns.rdata.Rdata:
    return dns.dnssec.sign(
        covered,
        signer.private_key,
        signer.zone,
        signer.dnskey,
        lifetime=86400,
        verify=True,
    )


def add_signed(
    section: list[dns.rrset.RRset], covered: dns.rrset.RRset, signer: Key
) -> None:
    rrsig = sign(covered, signer)
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


def child_soa_rrset() -> dns.rrset.RRset:
    return rrset(
        CHILD,
        dns.rdatatype.SOA,
        f"ns.{CHILD} hostmaster.{CHILD} 1 3600 600 86400 300",
    )


def nsec_lie(owner: str) -> dns.rrset.RRset:
    # An NSEC owned by the grandparent zone P at a name that really belongs
    # to the secure child C, showing an (insecure) delegation: NS bit set,
    # DS bit clear.
    return nsec_rrset(owner, f"grandz.{CHILD}", "NS", "RRSIG", "NSEC")


def grandchild_nsec_lie() -> dns.rrset.RRset:
    return nsec_lie(GRANDCHILD)


def nsec3_ns_lie(
    qname: str, zone: str, salt: str | None, salt_text: str
) -> dns.rrset.RRset:
    # An NSEC3 owned by 'zone' whose owner hash matches 'qname' under this
    # record's own parameters, showing an (insecure) delegation: NS bit set, DS
    # bit clear.  is_insecure_referral()'s trynsec3 arm takes the exact-match
    # branch (order == 0) before it ever consults the "next" field, so reusing
    # the owner digest as the next hash is sufficient for it to parse.
    digest = dns.dnssec.nsec3_hash(name(qname), salt, 0, 1).lower()
    owner = f"{digest}.{zone}"
    return rrset(owner, dns.rdatatype.NSEC3, f"1 0 0 {salt_text} {digest} NS")


def nsec3_nodata(
    qname: str, zone: str, salt: str | None, salt_text: str
) -> dns.rrset.RRset:
    # A genuine matching NSEC3 for 'qname' in 'zone' with the DS bit clear: a
    # legitimate NODATA-DS proof (the name exists as an ordinary, non-delegation
    # node).  The NS bit is clear, so it does not itself assert a delegation.
    digest = dns.dnssec.nsec3_hash(name(qname), salt, 0, 1).lower()
    owner = f"{digest}.{zone}"
    return rrset(owner, dns.rdatatype.NSEC3, f"1 0 0 {salt_text} {digest} TXT RRSIG")


def grandchild3_nsec3_lie() -> dns.rrset.RRset:
    # Same forgery as grandchild_nsec_lie(), but expressed as an NSEC3 signed by
    # the grandparent so that the resolver reaches is_insecure_referral()'s
    # trynsec3 arm.
    return nsec3_ns_lie(GRANDCHILD3, PARENT, None, "-")


def add_parent_nodata(
    response: dns.message.Message, parent_key: Key, nsec: dns.rrset.RRset
) -> None:
    add_signed(response.authority, soa_rrset(), parent_key)
    add_signed(response.authority, nsec, parent_key)


def add_nsec3_nodata_from_sibling(
    response: dns.message.Message, child_key: Key, sibling_key: Key
) -> None:
    # #6234: a genuinely signed NSEC3 owned by an unrelated sibling zone whose
    # owner hash matches the grandchild under the sibling's own parameters and
    # whose NS bit is set.  Its owner hash sorts before the child proof's (the
    # salts are chosen for exactly that, see _ordering_salts), so the ncache
    # iterates it first; trynsec3 matches it and derives the signer as
    # owner-minus-hash-label -> SIBLING (4 labels), which empties
    # closer_secure_ds_exists().  No owner-zone relevance check rejects it.  The
    # child-signed NSEC3 that follows is the real NODATA-DS proof:
    # dns_nsec3_noexistnodata() ignores the sibling record as out-of-zone, so
    # the negative answer still validates normally.
    add_signed(
        response.authority,
        nsec3_ns_lie(GRANDCHILD3_SIBLING, SIBLING, SIBLING_SALT, SIBLING_SALT),
        sibling_key,
    )
    add_signed(
        response.authority,
        nsec3_nodata(GRANDCHILD3_SIBLING, CHILD, CHILD_SALT, CHILD_SALT),
        child_key,
    )
    add_signed(response.authority, child_soa_rrset(), child_key)


def add_mixed_signer_nodata(
    response: dns.message.Message,
    parent_key: Key,
    nsec: dns.rrset.RRset,
    dummy_first: bool,
) -> None:
    """
    The same NODATA lie as add_parent_nodata(), but the NSEC carries two
    RRSIGs: the genuine one from the grandparent P and a dummy one naming
    the NSEC owner itself as signer.  The dummy uses an unsupported
    algorithm, so the validator skips it and the NSEC still authenticates
    through the genuine RRSIG.  All the dummy changes is which signer name
    comes first in the RRSIG rdataset (#6321).
    """
    add_signed(response.authority, soa_rrset(), parent_key)
    genuine = sign(nsec, parent_key)
    dummy = genuine.replace(algorithm=DUMMY_ALGORITHM, signer=nsec.name)
    rrsigs = [dummy, genuine] if dummy_first else [genuine, dummy]

    response.authority.append(nsec)
    # One single-rdata RRset per RRSIG: dnspython shuffles the rdatas of an
    # rdataset when rendering it, and this forgery is all about the order
    # in which the two signatures arrive.  Separate RRsets keep their list
    # order on the wire, and the resolver merges them back into one RRSIG
    # rdataset in that order.
    for rrsig in rrsigs:
        response.authority.append(dns.rrset.from_rdata(nsec.name, nsec.ttl, rrsig))


def add_many_rrsig_nodata(
    response: dns.message.Message,
    parent_key: Key,
    nsec: dns.rrset.RRset,
    count: int,
) -> None:
    """
    The NODATA lie with 'count' RRSIGs over the NSEC, all naming the
    grandparent P as signer: count - 1 unsupported-algorithm dummies with
    distinct key tags and a one-byte signature, then the genuine signature
    last, so the validator has to skip every dummy before the NSEC
    authenticates.  With a uniform signer this exercises only the RRSIG
    count cap in is_insecure_referral(), not the mixed-signer rule.
    """
    add_signed(response.authority, soa_rrset(), parent_key)
    genuine = sign(nsec, parent_key)
    dummies = [
        genuine.replace(algorithm=DUMMY_ALGORITHM, key_tag=tag, signature=b"\0")
        for tag in range(count - 1)
    ]

    response.authority.append(nsec)
    # Separate single-rdata RRsets, for the same wire-order reason as in
    # add_mixed_signer_nodata().
    for rrsig in [*dummies, genuine]:
        response.authority.append(dns.rrset.from_rdata(nsec.name, nsec.ttl, rrsig))


def prepare_response(qctx: QueryContext) -> dns.message.Message:
    qctx.prepare_new_response(with_zone_data=False)
    qctx.response.flags |= dns.flags.AA
    qctx.response.set_rcode(dns.rcode.NOERROR)
    return qctx.response


class GrandparentNsecHandler(ResponseHandler):
    def __init__(self, keys: dict[str, Key]) -> None:
        self.parent_key = keys[PARENT]
        self.child_key = keys[CHILD]
        self.sibling_key = keys[SIBLING]
        self.parent = name(PARENT)
        self.child = name(CHILD)
        self.sibling = name(SIBLING)
        self.grandchild = name(GRANDCHILD)
        self.grandchild3 = name(GRANDCHILD3)
        self.grandchild3_sibling = name(GRANDCHILD3_SIBLING)
        self.grandchild_dummy_first = name(GRANDCHILD_DUMMY_FIRST)
        self.grandchild_dummy_last = name(GRANDCHILD_DUMMY_LAST)
        self.grandchild_too_many = name(GRANDCHILD_TOO_MANY)
        self.grandchild_almost_too_many = name(GRANDCHILD_ALMOST_TOO_MANY)
        self.forged_grandchildren = (
            self.grandchild,
            self.grandchild3,
            self.grandchild3_sibling,
            self.grandchild_dummy_first,
            self.grandchild_dummy_last,
            self.grandchild_too_many,
            self.grandchild_almost_too_many,
        )

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
            # Priming, child DS.
            #
            # A real DS matching the child key, signed by the parent.  It must
            # be real rather than a placeholder because the sibling-zone
            # variant includes an NSEC3 signed by the child, so the child's
            # DNSKEY has to chain to the parent.  It is also the secure DS at
            # CHILD that closer_secure_ds_exists() finds when it refuses the
            # grandparent-signed proofs of the #5967 variants.
            ds = dns.dnssec.make_ds(self.child, self.child_key.dnskey, "SHA256")
            add_signed(
                response.answer,
                dns.rrset.from_rdata(self.child, TTL, ds),
                self.parent_key,
            )
        elif qctx.qname == self.child and qctx.qtype == dns.rdatatype.DNSKEY:
            # Priming, child DNSKEY.
            add_signed(
                response.answer,
                rrset_from_rdata(CHILD, self.child_key.dnskey),
                self.child_key,
            )
        elif qctx.qname == self.sibling and qctx.qtype == dns.rdatatype.DS:
            # Priming, sibling DS.
            #
            # The sibling zone is a genuine secure delegation: real DS signed by
            # the parent, so its own NSEC3 (used in the sibling-zone attack
            # variant) really validates.
            ds = dns.dnssec.make_ds(self.sibling, self.sibling_key.dnskey, "SHA256")
            add_signed(
                response.answer,
                dns.rrset.from_rdata(self.sibling, TTL, ds),
                self.parent_key,
            )
        elif qctx.qname == self.sibling and qctx.qtype == dns.rdatatype.DNSKEY:
            # Priming, sibling DNSKEY.
            add_signed(
                response.answer,
                rrset_from_rdata(SIBLING, self.sibling_key.dnskey),
                self.sibling_key,
            )
        elif qctx.qname == self.grandchild and qctx.qtype == dns.rdatatype.DS:
            # #5967: Forge no data for grand child DS (NSEC variant).
            add_parent_nodata(response, self.parent_key, grandchild_nsec_lie())
        elif qctx.qname == self.grandchild3 and qctx.qtype == dns.rdatatype.DS:
            # #5967: Forge no data for grand child DS (NSEC3 variant).
            add_parent_nodata(response, self.parent_key, grandchild3_nsec3_lie())
        elif qctx.qname == self.grandchild3_sibling and qctx.qtype == dns.rdatatype.DS:
            # #6234: Sibling-zone-signed NSEC3 ahead of the real child proof.
            add_nsec3_nodata_from_sibling(response, self.child_key, self.sibling_key)
        elif (
            qctx.qname == self.grandchild_dummy_first and qctx.qtype == dns.rdatatype.DS
        ):
            # Forge no data for grand child DS, dummy RRSIG before the
            # genuine one (mixed-signer variant, #6321)
            add_mixed_signer_nodata(
                response,
                self.parent_key,
                nsec_lie(GRANDCHILD_DUMMY_FIRST),
                dummy_first=True,
            )
        elif (
            qctx.qname == self.grandchild_dummy_last and qctx.qtype == dns.rdatatype.DS
        ):
            # Same forgery, genuine RRSIG before the dummy one
            add_mixed_signer_nodata(
                response,
                self.parent_key,
                nsec_lie(GRANDCHILD_DUMMY_LAST),
                dummy_first=False,
            )
        elif qctx.qname == self.grandchild_too_many and qctx.qtype == dns.rdatatype.DS:
            # Forge no data for grand child DS with as many RRSIGs as ns2
            # allows validations per fetch (RRSIG count cap variant)
            add_many_rrsig_nodata(
                response,
                self.parent_key,
                nsec_lie(GRANDCHILD_TOO_MANY),
                count=MAX_VALIDATIONS_PER_FETCH,
            )
        elif (
            qctx.qname == self.grandchild_almost_too_many
            and qctx.qtype == dns.rdatatype.DS
        ):
            # Same forgery with one RRSIG fewer, so it stays under the cap
            add_many_rrsig_nodata(
                response,
                self.parent_key,
                nsec_lie(GRANDCHILD_ALMOST_TOO_MANY),
                count=MAX_VALIDATIONS_PER_FETCH - 1,
            )
        elif (
            any(qctx.qname.is_subdomain(g) for g in self.forged_grandchildren)
            and qctx.qtype == dns.rdatatype.A
        ):
            # Attack query
            response.answer.append(
                rrset(qctx.qname.to_text(), dns.rdatatype.A, FORGED_A)
            )
        else:
            response.set_rcode(dns.rcode.NXDOMAIN)

        yield DnsResponseSend(response, authoritative=True)


def main() -> None:
    server = AsyncDnsServer(default_aa=True)
    server.install_response_handlers(GrandparentNsecHandler(load_keys()))
    server.run()


if __name__ == "__main__":
    main()
