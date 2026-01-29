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
    "nsec-to-nsec3.kasp",
    "nsec3-xfr-inline.kasp",
    "nsec3-dynamic-update-inline.kasp",
    "nsec3.kasp",
    "nsec3-dynamic.kasp",
    "nsec3-change.kasp",
    "nsec3-dynamic-change.kasp",
    "nsec3-dynamic-to-inline.kasp",
    "nsec3-inline-to-dynamic.kasp",
    "nsec3-to-nsec.kasp",
    "nsec3-to-optout.kasp",
    "nsec3-from-optout.kasp",
    "nsec3-other.kasp",
}

if os.environ["RSASHA1_SUPPORTED"] == "1":
    ZONES.update(
        {
            "rsasha1-to-nsec3.kasp",
            "rsasha1-to-nsec3-wait.kasp",
            "nsec3-to-rsasha1.kasp",
            "nsec3-to-rsasha1-ds.kasp",
        }
    )


def bootstrap():
    return {
        "zones": ZONES,
    }


@pytest.mark.parametrize(
    "params",
    [
        pytest.param(
            {
                "zone": "nsec-to-nsec3.kasp",
                "policy": "nsec",
                "key-properties": [
                    f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
            },
            id="nsec-to-nsec3.kasp",
        ),
        pytest.param(
            {
                "zone": "rsasha1-to-nsec3.kasp",
                "policy": "rsasha1",
                "key-properties": [
                    f"csk 0 {RSASHA1.number} 2048 goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent",
                ],
            },
            id="rsasha1-to-nsec3.kasp",
            marks=isctest.mark.with_algorithm("RSASHA1"),
        ),
        pytest.param(
            {
                "zone": "rsasha1-to-nsec3-wait.kasp",
                "policy": "rsasha1",
                "key-properties": [
                    f"csk 0 {RSASHA1.number} 2048 goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent",
                ],
            },
            id="rsasha1-to-nsec3-wait.kasp",
            marks=isctest.mark.with_algorithm("RSASHA1"),
        ),
        pytest.param(
            {
                # This is a secondary zone, where the primary is signed with
                # NSEC3 but the dnssec-policy dictates NSEC.
                "zone": "nsec3-xfr-inline.kasp",
                "policy": "nsec",
                "key-properties": [
                    f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
                "external-keys": [
                    f"csk 0 {ALGORITHM} {SIZE}",
                ],
                "external-keydir": "ns2",
            },
            id="nsec3-xfr-inline.kasp",
        ),
        pytest.param(
            {
                "zone": "nsec3-dynamic-update-inline.kasp",
                "policy": "nsec",
                "key-properties": [
                    f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
            },
            id="nsec3-dynamic-update-inline.kasp",
        ),
    ],
)
def test_nsec_case(ns3, params):
    # Get test parameters.
    zone = params["zone"]

    # First make sure the zone is properly signed.
    isctest.kasp.wait_keymgr_done(ns3, zone)

    # Test case.
    check_nsec3_case(ns3, params, nsec3=False)

    # Extra test for nsec3-dynamic-update-inline.kasp.
    if zone == "nsec3-dynamic-update-inline.kasp":
        isctest.log.info(f"dynamic update dnssec-policy zone {zone} with NSEC3")
        update_msg = dns.update.UpdateMessage(zone)
        update_msg.add(
            f"04O18462RI5903H8RDVL0QDT5B528DUJ.{zone}.",
            3600,
            "NSEC3",
            "0 0 0 408A4B2D412A4E95 1JMDDPMTFF8QQLIOINSIG4CR9OTICAOC A RRSIG",
        )

        with ns3.watch_log_from_here() as watcher:
            ns3.nsupdate(update_msg, expected_rcode=dns.rcode.REFUSED)
            watcher.wait_for_line(
                f"updating zone '{zone}/IN': update failed: explicit NSEC3 updates are not allowed in secure zones (REFUSED)"
            )


@pytest.mark.parametrize(
    "params",
    [
        pytest.param(
            {
                "zone": "nsec3-to-rsasha1.kasp",
                "policy": "nsec3",
                "key-properties": [
                    f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent",
                ],
            },
            id="nsec3-to-rsasha1.kasp",
            marks=isctest.mark.with_algorithm("RSASHA1"),
        ),
        pytest.param(
            {
                "zone": "nsec3-to-rsasha1-ds.kasp",
                "policy": "nsec3",
                "key-properties": [
                    f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent",
                ],
            },
            id="nsec3-to-rsasha1-ds.kasp",
            marks=isctest.mark.with_algorithm("RSASHA1"),
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
                "zone": "nsec3-change.kasp",
                "policy": "nsec3",
                "key-properties": [
                    f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
            },
            id="nsec3-change.kasp",
        ),
        pytest.param(
            {
                "zone": "nsec3-dynamic-change.kasp",
                "policy": "nsec3",
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
        pytest.param(
            {
                "zone": "nsec3-to-nsec.kasp",
                "policy": "nsec3",
                "key-properties": [
                    f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
            },
            id="nsec3-to-nsec.kasp",
        ),
        pytest.param(
            {
                "zone": "nsec3-to-optout.kasp",
                "policy": "nsec3",
                "key-properties": [
                    f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
            },
            id="nsec3-to-optout.kasp",
        ),
        pytest.param(
            {
                "zone": "nsec3-from-optout.kasp",
                "policy": "optout",
                "nsec3param": {
                    "optout": 1,
                    "salt-length": 0,
                },
                "key-properties": [
                    f"csk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
            },
            id="nsec3-from-optout.kasp",
        ),
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
    isctest.kasp.wait_keymgr_done(ns3, zone)

    # Test case.
    check_nsec3_case(ns3, params)
