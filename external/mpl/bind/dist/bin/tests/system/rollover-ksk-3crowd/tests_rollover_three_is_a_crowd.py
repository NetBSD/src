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

from datetime import timedelta

import isctest
from isctest.kasp import KeyTimingMetadata
from rollover.common import (
    pytestmark,
    alg,
    size,
    KSK_CONFIG,
    KSK_LIFETIME_POLICY,
    KSK_IPUB,
    KSK_IRET,
)
from rollover.setup import (
    configure_root,
    configure_tld,
    configure_ksk_3crowd,
)


CDSS = ["CDS (SHA-256)"]
POLICY = "ksk-doubleksk-autosign"
OFFSET1 = -int(timedelta(days=60).total_seconds())
OFFSET2 = -int(timedelta(hours=27).total_seconds())
TTL = int(KSK_CONFIG["dnskey-ttl"].total_seconds())


def bootstrap():
    data = {
        "tlds": [],
        "trust_anchors": [],
    }

    tlds = []
    tld_name = "kasp"
    delegations = configure_ksk_3crowd(tld_name)
    tld = configure_tld(tld_name, delegations)
    tlds.append(tld)
    data["tlds"].append(tld_name)
    ta = configure_root(tlds)
    data["trust_anchors"].append(ta)
    return data


def test_rollover_ksk_three_is_a_crowd(alg, size, ns3):
    """Test #2375: Scheduled rollovers are happening faster than they can finish."""
    zone = "three-is-a-crowd.kasp"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    step = {
        "zone": zone,
        "cdss": CDSS,
        "keyprops": [
            f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:unretentive offset:{OFFSET1}",
            f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:rumoured offset:{OFFSET2}",
            f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{OFFSET1}",
        ],
        "keyrelationships": [0, 1],
    }
    isctest.kasp.check_rollover_step(ns3, KSK_CONFIG, POLICY, step)

    # Rollover successor KSK (with DS in rumoured state).
    expected = isctest.kasp.policy_to_properties(TTL, step["keyprops"])
    keys = isctest.kasp.keydir_to_keylist(zone, ns3.identifier)
    isctest.kasp.check_keys(zone, keys, expected)
    key = expected[1].key
    now = KeyTimingMetadata.now()
    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"dnssec -rollover -key {key.tag} -when {now} {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    # We now expect four keys (3x KSK, 1x ZSK).
    step = {
        "zone": zone,
        "cdss": CDSS,
        "keyprops": [
            f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:unretentive offset:{OFFSET1}",
            f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:rumoured offset:{OFFSET2}",
            f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden offset:0",
            f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{OFFSET1}",
        ],
        "check-keytimes": False,  # checked manually with modified values
    }
    isctest.kasp.check_rollover_step(ns3, KSK_CONFIG, POLICY, step)

    expected = isctest.kasp.policy_to_properties(TTL, step["keyprops"])
    keys = isctest.kasp.keydir_to_keylist(zone, ns3.identifier)
    isctest.kasp.check_keys(zone, keys, expected)

    expected[0].metadata["Successor"] = expected[1].key.tag
    expected[1].metadata["Predecessor"] = expected[0].key.tag
    # Three is a crowd scenario.
    expected[1].metadata["Successor"] = expected[2].key.tag
    expected[2].metadata["Predecessor"] = expected[1].key.tag
    isctest.kasp.check_keyrelationships(keys, expected)
    for kp in expected:
        kp.set_expected_keytimes(KSK_CONFIG)

    # The first successor KSK is already being retired.
    expected[1].timing["Retired"] = now + KSK_IPUB
    expected[1].timing["Removed"] = now + KSK_IPUB + KSK_IRET

    isctest.kasp.check_keytimes(keys, expected)
