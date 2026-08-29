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

from datetime import datetime, timedelta, timezone

import shutil

from cryptography.hazmat.primitives.asymmetric import ec

import dns.dnssec
import dns.name
import dns.rdataclass
import dns.rdataset
import dns.rdatatype
import dns.rrset
import dns.zone

from isctest.run import EnvCmd
from isctest.vars.algorithms import Algorithm

import isctest

TTL = 3600


def bootstrap():
    alg = Algorithm.default()
    keygen = EnvCmd("KEYGEN", f"-q -a {alg.number} -b {alg.bits}")
    signer = EnvCmd("SIGNER", "-S -g -O full")

    def sign_regular_zone(ns: str, zone: str, database: str) -> isctest.kasp.Key:
        isctest.log.info(f"{zone}: generate keys")
        keygen(zone, cwd=ns).out.strip()
        ksk = keygen(f"-f KSK {zone}", cwd=ns).out.strip()

        isctest.log.info(f"{zone}: sign zone")
        signer(f"-o {zone} {database}", cwd=ns)

        if ns != "ns1":
            shutil.copy(f"{ns}/dsset-{zone}", f"ns1/dsset-{zone}")
            shutil.copy(f"{ns}/{ksk}.key", f"ns1/{ksk}.key")

        return isctest.kasp.Key(ksk, keydir=ns)

    # dnssec-signzone and `dns.dnssec.sign_zone` correctly disregard the invalid
    # NSEC entries when signing the zone. However, for this test we actualy *want*
    # to serve invalid yet signed zones. To accomplish this we sign the zone and then
    # replace the correct entries with the faulty ones accompanied by its RRSIG.
    #
    # TODO(aydin): move this to `isctest` to sign broken zones
    def sign_rogue_zone(ns: str, zone: str, database: str) -> None:
        # Read zone.
        origin = dns.name.from_text(zone)
        data = dns.zone.from_file(f"{ns}/{database}", origin=origin, relativize=False)

        # Get key for signing.
        isctest.log.info(f"{zone}: generate keys")
        private_key = ec.generate_private_key(ec.SECP256R1())
        dnskey = dns.dnssec.make_dnskey(
            public_key=private_key.public_key(),
            algorithm=dns.dnssec.Algorithm.ECDSAP256SHA256,
            flags=257,
        )

        # Sign zone.
        isctest.log.info(f"{zone}: sign zone")
        now = datetime.now(timezone.utc)
        inception = now - timedelta(hours=1)
        expiration = now + timedelta(days=30)

        for name, node in data.nodes.items():
            owner = name.derelativize(origin)
            rdatasets = list(node.rdatasets)

            for rdataset in rdatasets:
                rrset = dns.rrset.RRset(owner, rdataset.rdclass, rdataset.rdtype)
                rrset.update(rdataset)

                rrsig = dns.dnssec.sign(
                    rrset=rrset,
                    private_key=private_key,
                    signer=origin,
                    dnskey=dnskey,
                    inception=inception,
                    expiration=expiration,
                    deterministic=False,
                )

                rdataset = dns.rdataset.Rdataset(rrset.rdclass, dns.rdatatype.RRSIG)
                rdataset.add(rrsig, rrset.ttl)
                node.replace_rdataset(rdataset)

        # Sign DNSKEY RRset.
        dnskey_rrset = dns.rrset.RRset(origin, dns.rdataclass.IN, dns.rdatatype.DNSKEY)
        dnskey_rrset.add(dnskey, ttl=TTL)

        apex_node = data.nodes[origin]
        apex_node.replace_rdataset(dnskey_rrset)

        rrsig = dns.dnssec.sign(
            rrset=dnskey_rrset,
            private_key=private_key,
            signer=origin,
            dnskey=dnskey,
            inception=inception,
            expiration=expiration,
            deterministic=False,
        )
        rdataset = dns.rdataset.Rdataset(rrset.rdclass, dns.rdatatype.RRSIG)
        rdataset.add(rrsig, dnskey_rrset.ttl)
        apex_node.replace_rdataset(rdataset)

        # Output zone.
        data.to_file(f"{ns}/{database}.signed", relativize=False)

        # Output DS.
        ds = dns.dnssec.make_ds(name=origin, key=dnskey, algorithm="SHA256")
        with open(f"ns1/dsset-{zone}", "w", encoding="utf-8") as f:
            f.write(f"{zone} {TTL} IN DS {ds.to_text()}\n")

    sign_rogue_zone("ns3", "evil.test.", "evil.db")
    sign_regular_zone("ns2", "victim.test.", "victim.db")
    sign_regular_zone("ns1", "test.", "test.db")
    root_ksk = sign_regular_zone("ns1", ".", "root.db")

    return {
        "root": root_ksk.into_ta("static-key"),
    }


def test_out_of_zone_nsec(ns4):
    isctest.log.info("trying to poison aggressive nsec cache")
    msg = isctest.query.create("nx.evil.test", "A")
    res = isctest.query.tcp(msg, ns4.ip)
    isctest.check.noadflag(res)

    isctest.log.info("query victim from recursive")
    msg = isctest.query.create("victim.test", "SOA")
    res = isctest.query.tcp(msg, ns4.ip, attempts=1)
    isctest.check.noerror(res)
    isctest.check.adflag(res)
    isctest.check.rr_count_eq(res.answer, 2)

    isctest.log.info("checking for query history on victim nameserver")
    with open("ns2/named.run", "r", encoding="utf-8") as f:
        assert "(victim.test): query 'victim.test/SOA/IN' approved" in f.read()
