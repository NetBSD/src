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
import time

import dns.update
import pytest

pytest.importorskip("dns", minversion="2.0.0")
import isctest
import isctest.mark
from isctest.vars.algorithms import RSASHA1
from nsec3.common import (
    ALGORITHM,
    SIZE,
    default_config,
    pytestmark,
    check_nsec3_case,
)


# include the following zones when rendering named configs
ZONES = {
    "nsec3-change.kasp",
}


def bootstrap():
    return {
        "zones": ZONES,
    }


@pytest.fixture(scope="module", autouse=True)
def after_servers_start(ns3, templates):

    def check_soa_update():
        query = isctest.query.create(fqdn, dns.rdatatype.SOA)
        response = isctest.query.tcp(query, ns3.ip, attempts=1, timeout=2)
        rrset = response.get_rrset(
            response.answer,
            dns.name.from_text(fqdn),
            dns.rdataclass.IN,
            dns.rdatatype.SOA,
        )
        return rrset[0].minimum == 900

    nsdir = ns3.identifier

    zone = "nsec3-change.kasp"
    fqdn = f"{zone}."
    isctest.kasp.wait_keymgr_done(ns3, zone)

    shutil.copyfile(f"{nsdir}/template2.db.in", f"{nsdir}/{zone}.db")
    ns3.rndc(f"reload {zone}")

    isctest.run.retry_with_timeout(check_soa_update, timeout=10)
    # After reconfig, the NSEC3PARAM TTL should match the new SOA MINIMUM.

    # Reconfigure.
    data = {
        "reconfiged": True,
        "zones": ZONES,
    }
    templates.render(f"{nsdir}/named-fips.conf", data)
    templates.render(f"{nsdir}/named-rsasha1.conf", data)

    # Wait for the NSEC3 chain is finished rebuilding.
    messages = [
        f"zone {zone}/IN (signed): generated salt",
        f"zone_nsec3chain: zone {zone}/IN (signed): enter",
        f"add {zone}.	900	IN	NSEC3PARAM 1 0 0",
        f"zone_needdump: zone {zone}/IN (signed): enter",
    ]
    with ns3.watch_log_from_start() as watcher:
        ns3.reconfigure()
        watcher.wait_for_sequence(messages)


def test_nsec3_case(ns3):
    # Get test parameters.
    params = {
        "zone": "nsec3-change.kasp",
        "policy": "nsec3",
        "soa-minimum": 900,
        "nsec3param": {
            "optout": 1,
            "salt-length": 8,
        },
        "key-properties": [
            f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
        ],
    }
    zone = params["zone"]

    # First make sure the zone is properly signed.
    isctest.kasp.wait_keymgr_done(ns3, zone, reconfig=True)

    # Test case.
    check_nsec3_case(ns3, params)

    # Using rndc signing -nsec3param (should fail)
    isctest.log.info(f"use rndc signing -nsec3param {zone} to change NSEC3 settings")
    response = ns3.rndc(f"signing -nsec3param 1 1 12 ffff {zone}")
    assert "zone uses dnssec-policy, use rndc dnssec command instead" in response.out
