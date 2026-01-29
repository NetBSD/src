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

import os

from datetime import timedelta

import dns
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "*.axfr",
        "*.created",
        "dig.out.*",
        "rndc.reload.*",
        "rndc.signing.*",
        "update.out.*",
        "verify.out.*",
        "ns*/dsset-**",
        "ns*/K*",
        "ns*/settime.out.*",
        "ns*/*.db",
        "ns*/*.jbk",
        "ns*/*.jnl",
        "ns*/*.signed",
        "ns*/keygen.out.*",
        "ns3/named-*.conf",
    ]
)

ALGORITHM = os.environ["DEFAULT_ALGORITHM_NUMBER"]
SIZE = os.environ["DEFAULT_BITS"]

default_config = {
    "dnskey-ttl": timedelta(hours=1),
    "ds-ttl": timedelta(days=1),
    "max-zone-ttl": timedelta(days=1),
    "parent-propagation-delay": timedelta(hours=1),
    "publish-safety": timedelta(hours=1),
    "retire-safety": timedelta(hours=1),
    "signatures-refresh": timedelta(days=5),
    "signatures-validity": timedelta(days=14),
    "zone-propagation-delay": timedelta(minutes=5),
}


def check_auth_nsec(response):
    rrs = []
    for rrset in response.authority:
        if rrset.match(dns.rdataclass.IN, dns.rdatatype.NSEC, dns.rdatatype.NONE):
            rrs.append(rrset)
        assert not rrset.match(
            dns.rdataclass.IN, dns.rdatatype.NSEC3, dns.rdatatype.NONE
        )
    assert len(rrs) != 0, "no NSEC records found in authority section"


def check_auth_nsec3(response, iterations=0, optout=0, salt="-"):
    match = f"IN NSEC3 1 {optout} {iterations} {salt}"
    rrs = []

    for rrset in response.authority:
        if rrset.match(dns.rdataclass.IN, dns.rdatatype.NSEC3, dns.rdatatype.NONE):
            assert match in rrset.to_text()
            rrs.append(rrset)
        assert not rrset.match(
            dns.rdataclass.IN, dns.rdatatype.NSEC, dns.rdatatype.NONE
        )

    assert len(rrs) != 0, "no NSEC3 records found in authority section"


def check_nsec3param(response, match, saltlen):
    rrs = []
    salt = "-"

    for rrset in response.answer:
        if rrset.match(dns.rdataclass.IN, dns.rdatatype.NSEC3PARAM, dns.rdatatype.NONE):
            assert match in rrset.to_text()
            if saltlen == 0:
                assert f"{match} -" in rrset.to_text()
            else:
                assert not f"{match} -" in rrset.to_text()
                salt = rrset.to_text().split()[7]

            rrs.append(rrset)
        else:
            assert rrset.match(
                dns.rdataclass.IN, dns.rdatatype.RRSIG, dns.rdatatype.NSEC3PARAM
            )

    assert len(rrs) != 0

    return salt


def check_nsec3_case(server, params, nsec3=True):
    # Get test parameters.
    zone = params["zone"]
    fqdn = f"{zone}."
    policy = params["policy"]
    keydir = server.identifier
    config = default_config
    ttl = int(config["dnskey-ttl"].total_seconds())
    expected = isctest.kasp.policy_to_properties(ttl=ttl, keys=params["key-properties"])

    # Test case.
    isctest.log.info(f"check nsec case zone {zone} policy {policy}")

    # Key files.
    keys = isctest.kasp.keydir_to_keylist(zone, keydir)
    if "external-keys" in params:
        expected2 = isctest.kasp.policy_to_properties(ttl, keys=params["external-keys"])
        for ek in expected2:
            ek.private = False  # noqa
            ek.legacy = True  # noqa
        expected = expected + expected2
        assert "external-keydir" in params
        extkeys = isctest.kasp.keydir_to_keylist(zone, params["external-keydir"])
        keys = keys + extkeys

    isctest.kasp.check_keys(zone, keys, expected)
    isctest.kasp.check_dnssec_verify(server, zone)
    isctest.kasp.check_apex(server, zone, keys, [])

    query = isctest.query.create(fqdn, dns.rdatatype.NSEC3PARAM)
    nsec3param_response = isctest.query.tcp(query, server.ip)
    assert nsec3param_response.rcode() == dns.rcode.NOERROR

    query = isctest.query.create(f"nosuchname.{fqdn}", dns.rdatatype.A)
    response = isctest.query.tcp(query, server.ip)
    assert response.rcode() == dns.rcode.NXDOMAIN

    if nsec3:
        # NSEC3
        minimum = params.get("soa-minimum", 3600)
        iterations = 0
        optout = 0
        saltlen = 0
        if "nsec3param" in params:
            optout = params["nsec3param"].get("optout", 0)
            saltlen = params["nsec3param"].get("salt-length", 0)

        match = f"{fqdn} {minimum} IN NSEC3PARAM 1 0 {iterations}"

        salt = check_nsec3param(nsec3param_response, match, saltlen)

        check_auth_nsec3(response, iterations, optout, salt)
    else:
        # NSEC
        assert len(nsec3param_response.answer) == 0
        check_auth_nsec(nsec3param_response)

        check_auth_nsec(response)
