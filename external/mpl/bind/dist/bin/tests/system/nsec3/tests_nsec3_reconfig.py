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
    "nsec3-to-nsec.kasp",
    "nsec-to-nsec3.kasp",
    "nsec3.kasp",
    "nsec3-dynamic.kasp",
    "nsec3-dynamic-change.kasp",
    "nsec3-dynamic-to-inline.kasp",
    "nsec3-inline-to-dynamic.kasp",
    # "nsec3-to-optout.kasp",
    # "nsec3-from-optout.kasp",
    "nsec3-other.kasp",
    "nsec3-ent.kasp",
}

if os.environ["RSASHA1_SUPPORTED"] == "1":
    ZONES.update(
        {
            "rsasha1-to-nsec3-wait.kasp",
            "nsec3-to-rsasha1.kasp",
            "nsec3-to-rsasha1-ds.kasp",
            "rsasha1-to-nsec3.kasp",
        }
    )


def bootstrap():
    return {
        "zones": ZONES,
    }


@pytest.fixture(scope="module", autouse=True)
def after_servers_start(ns3, templates):
    # First make sure all zones are properly signed. Here we specifically need
    # to wait until all zones have finished key management before we can
    # reconfigure the server, because changing the DNSSEC policy relies on
    # zones having finished applying their initial policy.
    for zone in ZONES:
        isctest.kasp.wait_keymgr_done(ns3, zone)

    # Ensure rsasha1-to-nsec3-wait.kasp is fully signed prior to reconfig.
    with_rsasha1 = "RSASHA1_SUPPORTED"
    assert with_rsasha1 in os.environ, f"{with_rsasha1} env variable undefined"
    if os.getenv(with_rsasha1) == "1":
        zone = "rsasha1-to-nsec3-wait.kasp"
        isctest.kasp.check_dnssec_verify(ns3, zone)

    # Reconfigure.
    data = {
        "reconfiged": True,
        "zones": ZONES,
    }
    templates.render(f"{ns3.identifier}/named-fips.conf", data)
    templates.render(f"{ns3.identifier}/named-rsasha1.conf", data)
    ns3.reconfigure()


@pytest.mark.parametrize(
    "params",
    [
        pytest.param(
            {
                "zone": "rsasha1-to-nsec3.kasp",
                "policy": "nsec3",
                "key-properties": [
                    f"csk 0 {RSASHA1.number} 2048 goal:hidden dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent",
                    f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
            },
            id="rsasha1-to-nsec3.kasp",
            marks=isctest.mark.with_algorithm("RSASHA1"),
        ),
        pytest.param(
            {
                "zone": "rsasha1-to-nsec3-wait.kasp",
                "policy": "nsec3",
                "key-properties": [
                    f"csk 0 {RSASHA1.number} 2048 goal:hidden dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent",
                    f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
            },
            id="rsasha1-to-nsec3-wait.kasp",
            marks=isctest.mark.with_algorithm("RSASHA1"),
        ),
        pytest.param(
            {
                "zone": "nsec3-to-rsasha1.kasp",
                "policy": "rsasha1",
                "key-properties": [
                    f"csk 0 {ALGORITHM} {SIZE} goal:hidden dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent",
                    f"csk 0 {RSASHA1.number} 2048 goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
            },
            id="nsec3-to-rsasha1.kasp",
            marks=isctest.mark.with_algorithm("RSASHA1"),
        ),
        pytest.param(
            {
                "zone": "nsec3-to-rsasha1-ds.kasp",
                "policy": "rsasha1",
                "key-properties": [
                    f"csk 0 {ALGORITHM} {SIZE} goal:hidden dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent",
                    f"csk 0 {RSASHA1.number} 2048 goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
            },
            id="nsec3-to-rsasha1-ds.kasp",
            marks=isctest.mark.with_algorithm("RSASHA1"),
        ),
        pytest.param(
            {
                "zone": "nsec3-to-nsec.kasp",
                "policy": "nsec",
                "key-properties": [
                    f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
            },
            id="nsec3-to-nsec.kasp",
        ),
    ],
)
def test_nsec_case(ns3, params):
    zone = params["zone"]

    # First make sure the zone is properly signed.
    isctest.kasp.wait_keymgr_done(ns3, zone, reconfig=True)

    # Test case.
    check_nsec3_case(ns3, params, nsec3=False)


@pytest.mark.parametrize(
    "params",
    [
        pytest.param(
            {
                "zone": "nsec-to-nsec3.kasp",
                "policy": "nsec3",
                "key-properties": [
                    f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
            },
            id="nsec-to-nsec3.kasp",
        ),
        pytest.param(
            {
                "zone": "nsec3.kasp",
                "policy": "nsec3",
                "key-properties": [
                    f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
            },
            id="nsec3.kasp",
        ),
        pytest.param(
            {
                "zone": "nsec3-dynamic.kasp",
                "policy": "nsec3",
                "key-properties": [
                    f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
            },
            id="nsec3-dynamic.kasp",
        ),
        pytest.param(
            {
                "zone": "nsec3-dynamic-change.kasp",
                "policy": "nsec3-other",
                "nsec3param": {
                    "optout": 1,
                    "salt-length": 8,
                },
                "key-properties": [
                    f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
            },
            id="nsec3-dynamic-change.kasp",
        ),
        pytest.param(
            {
                "zone": "nsec3-dynamic-to-inline.kasp",
                "policy": "nsec3",
                "key-properties": [
                    f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
            },
            id="nsec3-dynamic-to-inline.kasp",
        ),
        pytest.param(
            {
                "zone": "nsec3-inline-to-dynamic.kasp",
                "policy": "nsec3",
                "key-properties": [
                    f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
            },
            id="nsec3-inline-to-dynamic.kasp",
        ),
        # DISABLED:
        # There is a bug in the nsec3param building code that thinks when the
        # optout bit is changed, the chain already exists. [GL #2216]
        # pytest.param(
        #    {
        #        "zone": "nsec3-to-optout.kasp",
        #        "policy": "nsec3",
        #        "nsec3param": {
        #            "optout": 1,
        #            "salt-length": 0,
        #        },
        #        "key-properties": [
        #            f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
        #        ],
        #    },
        #    id="nsec3-to-optout.kasp",
        # ),
        # DISABLED:
        # There is a bug in the nsec3param building code that thinks when the
        # optout bit is changed, the chain already exists. [GL #2216]
        # pytest.param(
        #    {
        #        "zone": "nsec3-from-optout.kasp",
        #        "policy": "optout",
        #        "key-properties": [
        #            f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
        #        ],
        #    },
        #    id="nsec3-from-optout.kasp",
        # ),
        pytest.param(
            {
                "zone": "nsec3-other.kasp",
                "policy": "nsec3-other",
                "nsec3param": {
                    "optout": 1,
                    "salt-length": 8,
                },
                "key-properties": [
                    f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
            },
            id="nsec3-other.kasp",
        ),
    ],
)
def test_nsec3_case(ns3, params):
    # Get test parameters.
    zone = params["zone"]

    # First make sure the zone is properly signed.
    isctest.kasp.wait_keymgr_done(ns3, zone, reconfig=True)

    # Test case.
    check_nsec3_case(ns3, params)


def test_nsec3_ent(ns3, templates):
    # Zone: nsec3-ent.kasp (regression test for #5108)
    params = {
        "zone": "nsec3-ent.kasp",
        "policy": "nsec3",
        "key-properties": [
            f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
        ],
    }

    zone = params["zone"]
    fqdn = f"{zone}."

    # First make sure the zone is properly signed.
    isctest.kasp.wait_keymgr_done(ns3, zone, reconfig=True)

    # Test case.
    check_nsec3_case(ns3, params)

    # Test empty non-terminals do not trigger a crash.
    isctest.log.info("check query for newly empty name does not crash")

    # confirm the pre-existing name still exists
    query = isctest.query.create(f"c.{fqdn}", dns.rdatatype.A)
    response = isctest.query.tcp(query, ns3.ip, ns3.ports.dns, timeout=3)
    assert response.rcode() == dns.rcode.NOERROR

    match = "10.0.0.3"
    rrset = response.get_rrset(
        response.answer,
        dns.name.from_text(f"c.{fqdn}"),
        dns.rdataclass.IN,
        dns.rdatatype.A,
    )
    assert rrset is not None, "no A records found in answer section"
    assert match in str(rrset[0])

    # remove a name, bump the SOA, and reload
    templates.render(f"{ns3.identifier}/nsec3-ent.kasp.db", {"serial": 2})

    messages = [
        f"zone {zone}/IN (unsigned): loaded serial 2",
        f"zone_needdump: zone {zone}/IN (signed): enter",
    ]
    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"reload {zone}")
        watcher.wait_for_sequence(messages)

    # try the query again
    query = isctest.query.create(f"c.{fqdn}", dns.rdatatype.A)
    response = isctest.query.tcp(query, ns3.ip, ns3.ports.dns, timeout=3)
    assert response.rcode() == dns.rcode.NXDOMAIN

    isctest.log.info("check queries for new names below ENT do not crash")

    # confirm the ENT name does not exist yet
    query = isctest.query.create(f"x.y.z.{fqdn}", dns.rdatatype.A)
    response = isctest.query.tcp(query, ns3.ip, ns3.ports.dns, timeout=3)
    assert response.rcode() == dns.rcode.NXDOMAIN

    # add a name with an ENT, bump the SOA, and reload ensuring the time stamp changes
    templates.render(f"{ns3.identifier}/nsec3-ent.kasp.db", {"serial": 3})

    messages = [
        f"zone {zone}/IN (unsigned): loaded serial 3",
        f"zone_needdump: zone {zone}/IN (signed): enter",
    ]
    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"reload {zone}")
        watcher.wait_for_sequence(messages)

    # try the query again
    query = isctest.query.create(f"x.y.z.{fqdn}", dns.rdatatype.A)
    response = isctest.query.tcp(query, ns3.ip, ns3.ports.dns, timeout=3)
    assert response.rcode() == dns.rcode.NOERROR

    match = "10.0.0.4"
    rrset = response.get_rrset(
        response.answer,
        dns.name.from_text(f"x.y.z.{fqdn}"),
        dns.rdataclass.IN,
        dns.rdatatype.A,
    )
    assert rrset is not None, "no A records found in answer section"
    assert match in str(rrset[0])
