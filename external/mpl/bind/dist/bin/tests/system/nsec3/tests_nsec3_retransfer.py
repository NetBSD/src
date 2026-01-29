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

# pylint: disable=redefined-outer-name,unused-import

import os
import shutil

from datetime import timedelta

import dns.update
import pytest

pytest.importorskip("dns", minversion="2.0.0")
import isctest
import isctest.mark
from isctest.vars.algorithms import RSASHA256
from nsec3.common import (
    pytestmark,
    check_auth_nsec3,
    check_nsec3param,
)

DNSKEY_TTL = int(timedelta(hours=1).total_seconds())
ZSK_LIFETIME = int(timedelta(days=90).total_seconds())

# include the following zones when rendering named configs
ZONES = {
    "retransfer.kasp",
}


def bootstrap():
    return {
        "zones": ZONES,
    }


def perform_nsec3_tests(server, params):
    # Get test parameters.
    zone = params["zone"]
    fqdn = f"{zone}."
    policy = params["policy"]
    keydir = server.identifier
    minimum = params.get("soa-minimum", 3600)
    expected = isctest.kasp.policy_to_properties(
        ttl=DNSKEY_TTL, keys=params["key-properties"]
    )

    iterations = 0
    optout = 0
    saltlen = 0

    match = f"{fqdn} {minimum} IN NSEC3PARAM 1 0 {iterations}"

    # Test case.
    isctest.log.info(f"check nsec3 case zone {zone} policy {policy}")

    # First make sure the zone is properly signed.
    isctest.kasp.wait_keymgr_done(server, zone)

    keys = isctest.kasp.keydir_to_keylist(zone, keydir)
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if k.is_zsk()]
    isctest.kasp.check_keys(zone, keys, expected)
    isctest.kasp.check_dnssec_verify(server, zone)
    isctest.kasp.check_apex(server, zone, ksks, zsks)

    query = isctest.query.create(fqdn, dns.rdatatype.NSEC3PARAM)
    response = isctest.query.tcp(query, server.ip, server.ports.dns, timeout=3)
    assert response.rcode() == dns.rcode.NOERROR

    salt = check_nsec3param(response, match, saltlen)

    query = isctest.query.create(f"nosuchname.{fqdn}", dns.rdatatype.A)
    response = isctest.query.tcp(query, server.ip, server.ports.dns, timeout=3)
    assert response.rcode() == dns.rcode.NXDOMAIN
    check_auth_nsec3(response, iterations, optout, salt)

    return salt


def test_nsec3_retransfer(servers, templates):
    ns2 = servers["ns2"]
    ns3 = servers["ns3"]

    params = {
        "zone": "retransfer.kasp",
        "policy": "nsec3rsa256",
        "key-properties": [
            f"ksk 0 {RSASHA256.number} 2048 goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden",
            f"zsk {ZSK_LIFETIME} {RSASHA256.number} 2048 goal:omnipresent dnskey:rumoured zrrsig:rumoured",
        ],
    }

    zone = params["zone"]
    salt = perform_nsec3_tests(ns3, params)

    # Stop primary.
    ns2.stop()

    # Update the zone.
    serial = 10
    templates.render(f"{ns2.identifier}/{zone}.db", {"serial": serial})

    with ns2.watch_log_from_here() as watcher:
        ns2.start(["--noclean", "--restart", "--port", os.environ["PORT"]])
        watcher.wait_for_line("all zones loaded")

    # Test NSEC3 and NSEC3PARAM is the same after retransfer.
    isctest.log.info(f"check zone {zone} after retransfer has salt {salt}")
    prevsalt = salt

    # Retransfer zone, NSEC3 should stay the same.
    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"retransfer {zone}")
        # When sending notifies, the zone should be up to date.
        watcher.wait_for_line(f"zone {zone}/IN (signed): sending notify to 10.53.0.4")

    salt = perform_nsec3_tests(ns3, params)
    assert prevsalt == salt
