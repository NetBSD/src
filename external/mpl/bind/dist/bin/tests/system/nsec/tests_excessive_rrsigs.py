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

import dns.rdataclass
import dns.rdatatype
import dns.rdtypes.ANY.RRSIG
import dns.zone

from isctest.run import EnvCmd

import isctest


def duplicate_rrsig(rdata, i):
    return dns.rdtypes.ANY.RRSIG.RRSIG(
        rdclass=rdata.rdclass,
        rdtype=rdata.rdtype,
        type_covered=rdata.type_covered,
        algorithm=rdata.algorithm,
        labels=rdata.labels,
        # increment orig TTL so the rdataset isn't treated as identical record by dnspython
        original_ttl=rdata.original_ttl + i,
        expiration=rdata.expiration,
        inception=rdata.inception,
        key_tag=rdata.key_tag,
        signer=rdata.signer,
        signature=rdata.signature,
    )


def bootstrap():
    keygen = EnvCmd("KEYGEN", "-a ECDSA256 -Kns2 -q")
    signer = EnvCmd("SIGNER", "-S -g")

    zone = "excessive-nsec-rrsigs"
    infile = f"{zone}.db.in"
    signedfile = f"{zone}.db.signed"

    isctest.log.info(f"{zone}: generate ksk and zsk")
    ksk_name = keygen(f"-f KSK {zone}").out.strip()
    keygen(f"{zone}").out.strip()
    ksk = isctest.kasp.Key(ksk_name, keydir="ns2")

    isctest.log.info(f"{zone}: sign zone")
    signer(f"-P -x -O full -o {zone} -f {signedfile} {infile}", cwd="ns2")

    isctest.log.info(
        f"{zone}: duplicate the RRSIG(NSEC) to exceed max-records-per-type"
    )
    zone = dns.zone.from_file(f"ns2/{signedfile}", origin=zone)
    for node in zone.values():
        rrsig_rdataset = node.find_rdataset(
            dns.rdataclass.IN, dns.rdatatype.RRSIG, dns.rdatatype.NSEC
        )
        orig = rrsig_rdataset[0]
        rrsig_rdataset.add(duplicate_rrsig(orig, 1))
        rrsig_rdataset.add(duplicate_rrsig(orig, 2))
    zone.to_file(f"ns2/{signedfile}", sorted=True)

    return {
        "trust_anchors": [
            ksk.into_ta("static-key"),
        ],
    }


# reproducer for CVE-2026-3104 [GL#5742]
def test_excessive_rrsigs(ns3):
    # the real test is that there is no crash on shutdown - checked by the test
    # framework when the test finishes

    # multiple queries seem more reliable to reproduce the memory leak, using a
    # single query sometimes didn't cause a crash on shutdown
    for i in range(10):
        msg = isctest.query.create(f"x{i}.excessive-nsec-rrsigs", "A")
        res = isctest.query.udp(msg, ns3.ip, attempts=1)
        isctest.check.servfail(res)
