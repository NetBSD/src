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

from pathlib import Path
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

from dnssec_py.common import DNSSEC_PY_MARK
from isctest.template import NS2, TrustAnchor, zones
from isctest.zone import Zone, configure_root

import isctest

pytestmark = DNSSEC_PY_MARK


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
    zone = Zone("dnskey-malformed", NS2, signed=True)
    lifetime = 300

    # generate KSK, avoid key tag collision with ZSKs
    while True:
        ksk_private_key, ksk_dnskey = generate_key()
        if dns.dnssec.key_id(ksk_dnskey) != MALFORMED_ZSK_KEY_TAG:
            break
    keys = [(ksk_private_key, ksk_dnskey)]

    # render unsigned zone file
    zone.render()

    # read the rendered zone
    unsigned_path = str(Path(zone.ns.name) / zone.filepath_unsigned)
    signed_path = str(Path(zone.ns.name) / zone.filepath_signed)
    zoneobj = dns.zone.from_file(unsigned_path, origin="dnskey-malformed.")

    # sign the zone (including the malformed ZSKs) with KSK
    with zoneobj.writer() as txn:
        dns.dnssec.sign_zone(
            zone=zoneobj,
            txn=txn,
            keys=keys,
            lifetime=lifetime,
            add_dnskey=True,
            deterministic=False,  # for OpenSSL<3.2.0 compat
        )

    # force use of the malformed ZSKs for invalid-rrsig.dnskey-malformed;
    # the record only has one invalid RRSIG signed with a malformed ZSK
    invalid_rrset = zoneobj.get_rdataset("invalid-rrsig", "RRSIG", "A")
    rr = invalid_rrset.pop()
    invalid_rrset.add(create_malformed_rr(rr))

    # multiple-rrsigs.dnskey-malformed contains a lot of RRSIGs with the same
    # invalid signature using a malformed key, and one valid RRSIG
    multiple_rrset = zoneobj.get_rdataset("multiple-rrsigs", "RRSIG", "A")
    rr = multiple_rrset.pop()
    for i in range(99):
        multiple_rrset.add(create_malformed_rr(rr, i))
    multiple_rrset.add(rr)

    zoneobj.to_file(signed_path)

    root = configure_root([zone])
    ksk_key_b64 = base64.b64encode(ksk_dnskey.key).decode()
    ksk_ta = TrustAnchor("dnskey-malformed", "static-key", f'257 3 14 "{ksk_key_b64}"')

    return {
        "rrset_order_none": ["dnskey-malformed"],
        "trust_anchors": [*root.trust_anchors(), ksk_ta],
        "zones": zones([root, zone]),
    }


def test_malformed_ecdsa(ns9):
    log_validation_failed = Re(
        r"invalid-rrsig\.dnskey-malformed/A\): validation failed"
    )
    log_openssl_failure = Re("EVP_PKEY_fromdata.*failed")
    log_openssl_version = Re("linked to OpenSSL version: OpenSSL ([0-9]+)")

    msg = isctest.query.create("invalid-rrsig.dnskey-malformed", "A")

    openssl_vers = ns9.log.grep(log_openssl_version)
    if (
        openssl_vers
        and int(openssl_vers[0].group(1)) >= 3
        and os.getenv("FEATURE_QUERYTRACE") == "1"
    ):
        # extra check for OpenSSL 3.0.0+
        with ns9.watch_log_from_here() as watcher:
            res = isctest.query.tcp(msg, ns9.ip)

            # check the OpenSSL-specific log message appears just once
            matches = watcher.wait_for_all(
                [
                    log_openssl_failure,
                    log_validation_failed,
                ]
            )
            assert len([m for m in matches if m.re == log_openssl_failure]) == 1
    else:
        res = isctest.query.tcp(msg, ns9.ip)

    isctest.check.servfail(res)


def test_multiple_rrsigs(ns2, ns9):
    log_validation_failed = Re(
        r"multiple-rrsigs\.dnskey-malformed/A\): validation failed"
    )
    log_openssl_failure = Re("EVP_PKEY_fromdata.*failed")
    log_openssl_version = Re("linked to OpenSSL version: OpenSSL ([0-9]+)")

    msg = isctest.query.create("multiple-rrsigs.dnskey-malformed", "A")

    # Check the order of returned RRSIGs from auth. Due to rrset-order none;
    # this should remain constant for the remainder of the test.
    # Ensure the first two RRSIGs are malformed, otherwise skip the test.
    res = isctest.query.tcp(msg, ns2.ip)
    rrsigs = res.get_rrset(
        res.answer,
        dns.name.from_text("multiple-rrsigs.dnskey-malformed."),
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

    openssl_vers = ns9.log.grep(log_openssl_version)
    if (
        openssl_vers
        and int(openssl_vers[0].group(1)) >= 3
        and os.getenv("FEATURE_QUERYTRACE") == "1"
    ):
        # extra check for OpenSSL 3.0.0+
        with ns9.watch_log_from_here() as watcher:
            res = isctest.query.tcp(msg, ns9.ip)

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
        res = isctest.query.tcp(msg, ns9.ip)

    isctest.check.servfail(res)
