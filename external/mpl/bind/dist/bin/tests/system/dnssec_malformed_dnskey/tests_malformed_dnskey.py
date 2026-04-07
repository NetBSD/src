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

from re import compile as Re

import base64
import os

from cryptography.hazmat.primitives.asymmetric import ec
from dns.rdtypes.dnskeybase import Flag

import dns.dnssec
import dns.name
import dns.rdataclass
import dns.rdatatype
import dns.rdtypes.ANY.RRSIG
import dns.zone
import pytest

import isctest


def generate_key():
    algorithm = dns.dnssec.Algorithm.ECDSAP384SHA384
    ksk_private_key = ec.generate_private_key(ec.SECP384R1())
    try:
        ksk_dnskey = dns.dnssec.make_dnskey(
            public_key=ksk_private_key.public_key(),
            algorithm=algorithm,
            flags=Flag.ZONE | Flag.SEP,
        )
    except ImportError as exc:
        # if the cryptography package is too old, the make_dnskey() function
        # will raise ImportError at runtime
        pytest.skip(f"{exc}")
    return ksk_private_key, ksk_dnskey


MALFORMED_ZSK_KEY_TAG = 20071


def create_malformed_rr(rr, n=0):
    malformed_rr = dns.rdtypes.ANY.RRSIG.RRSIG(
        rdclass=rr.rdclass,
        rdtype=rr.rdtype,
        type_covered=rr.type_covered,
        algorithm=rr.algorithm,
        labels=rr.labels,
        original_ttl=rr.original_ttl - n,  # edit TTL so multiple RRSIGs can be added
        expiration=rr.expiration,
        inception=rr.inception,
        key_tag=MALFORMED_ZSK_KEY_TAG,  # overwrite with the malformed ZSKs
        signer=rr.signer,
        signature=rr.signature,
    )
    return malformed_rr


def bootstrap():
    zone = dns.zone.from_file("ns2/example.db.in", origin="example.")
    lifetime = 300

    # geneate KSK, avoid key tag collision with ZSKs
    while True:
        ksk_private_key, ksk_dnskey = generate_key()
        if dns.dnssec.key_id(ksk_dnskey) != MALFORMED_ZSK_KEY_TAG:
            break
    keys = [(ksk_private_key, ksk_dnskey)]

    # sign the zone (including the malformed ZSKs) with KSK
    with zone.writer() as txn:
        dns.dnssec.sign_zone(
            zone=zone,
            txn=txn,
            keys=keys,
            lifetime=lifetime,
            add_dnskey=True,
            deterministic=False,  # for OpenSSL<3.2.0 compat
        )

    # force use of the malformed ZSKs for dnssec verification
    # malformed-dnskey.example. has only one invalid RRSIG and is only signed
    # with malformed ZSKs
    malformed_rrset = zone.get_rdataset("malformed-dnskey", "RRSIG", "A")
    rr = malformed_rrset.pop()
    malformed_rrset.add(create_malformed_rr(rr))

    # multiple-rrsigs.example. contains a lot of RRSIGS with the same invalid
    # signature using malformed RRSIG, and one valid RRSIG
    multiple_rrset = zone.get_rdataset("multiple-rrsigs", "RRSIG", "A")
    rr = multiple_rrset.pop()
    for i in range(99):
        multiple_rrset.add(create_malformed_rr(rr, i))
    multiple_rrset.add(rr)

    zone.to_file("ns2/example.db.signed.malformed")

    return {
        "ksk_public_key": base64.b64encode(ksk_dnskey.key).decode(),
    }


def test_malformed_ecdsa(ns3):
    log_validation_failed = Re(r"malformed-dnskey\.example/A\): validation failed")
    log_openssl_failure = Re("EVP_PKEY_fromdata.*failed")
    log_openssl_version = Re("linked to OpenSSL version: OpenSSL ([0-9]+)")

    msg = isctest.query.create("malformed-dnskey.example", "A")

    openssl_vers = ns3.log.grep(log_openssl_version)
    if (
        openssl_vers
        and int(openssl_vers[0].group(1)) >= 3
        and os.getenv("FEATURE_QUERYTRACE") == "1"
    ):
        # extra check for OpenSSL 3.0.0+
        with ns3.watch_log_from_here() as watcher:
            res = isctest.query.tcp(msg, "10.53.0.3")

            # check the OpenSSL-specific log message appears just once
            matches = watcher.wait_for_all(
                [
                    log_openssl_failure,
                    log_validation_failed,
                ]
            )
            assert len([m for m in matches if m.re == log_openssl_failure]) == 1
    else:
        res = isctest.query.tcp(msg, "10.53.0.3")

    isctest.check.servfail(res)


def test_multiple_rrsigs(ns3):
    log_validation_failed = Re(r"multiple-rrsigs\.example/A\): validation failed")
    log_openssl_failure = Re("EVP_PKEY_fromdata.*failed")
    log_openssl_version = Re("linked to OpenSSL version: OpenSSL ([0-9]+)")

    msg = isctest.query.create("multiple-rrsigs.example", "A")

    # Check the order of returned RRSIGs from auth. Due to rrset-order none;
    # this should remain constant for the remainder of the test.
    # Ensure the first two RRSIGs are malformed, otherwise skip the test.
    res = isctest.query.tcp(msg, "10.53.0.2")
    rrsigs = res.get_rrset(
        res.answer,
        dns.name.from_text("multiple-rrsigs.example."),
        dns.rdataclass.IN,
        dns.rdatatype.RRSIG,
        dns.rdatatype.A,
    )
    assert len(rrsigs) > 2
    if (
        rrsigs[0].key_tag != MALFORMED_ZSK_KEY_TAG
        or rrsigs[1].key_tag != MALFORMED_ZSK_KEY_TAG
    ):
        pytest.skip("valid RRSIG listed first in response, re-run test")

    openssl_vers = ns3.log.grep(log_openssl_version)
    if (
        openssl_vers
        and int(openssl_vers[0].group(1)) >= 3
        and os.getenv("FEATURE_QUERYTRACE") == "1"
    ):
        # extra check for OpenSSL 3.0.0+
        with ns3.watch_log_from_here() as watcher:
            res = isctest.query.tcp(msg, "10.53.0.3")

            # check the OpenSSL-specific log message appears exactly twice:
            # one failure is allowed by setting max-validation-failures-per-fetch 1;
            matches = watcher.wait_for_all(
                [
                    log_openssl_failure,
                    log_validation_failed,
                ]
            )
            assert len([m for m in matches if m.re == log_openssl_failure]) == 2
    else:
        res = isctest.query.tcp(msg, "10.53.0.3")

    isctest.check.servfail(res)


def test_truncated_dnskey():
    msg = isctest.query.create("a.truncated.selfsigned.", "A")
    res = isctest.query.tcp(msg, "10.53.0.3")
    isctest.check.servfail(res)
