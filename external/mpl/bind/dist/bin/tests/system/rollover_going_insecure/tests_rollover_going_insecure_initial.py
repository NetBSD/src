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

import pytest

from rollover.common import CDSS, DURATION, ROLLOVER_MARK, UNSIGNING_CONFIG
from rollover.setup import configure_going_insecure, configure_root, configure_tld

import isctest

pytestmark = ROLLOVER_MARK


def bootstrap():
    data = {
        "tlds": [],
        "trust_anchors": [],
    }

    tlds = []
    tld_name = "kasp"
    delegations = configure_going_insecure(tld_name, reconfig=False)
    tld = configure_tld(tld_name, delegations)
    tlds.append(tld)
    data["tlds"].append(tld_name)
    ta = configure_root(tlds)
    data["trust_anchors"].append(ta)
    return data


@pytest.mark.parametrize(
    "zone",
    [
        "going-insecure.kasp",
        "going-insecure-dynamic.kasp",
    ],
)
def test_going_insecure_initial(zone, ns3, default_algorithm):
    config = UNSIGNING_CONFIG
    policy = "unsigning"
    zone = f"step1.{zone}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    step = {
        "zone": zone,
        "cdss": CDSS,
        "keyprops": [
            f"ksk 0 {default_algorithm.number} {default_algorithm.bits} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{-DURATION['P10D']}",
            f"zsk {DURATION['P60D']} {default_algorithm.number} {default_algorithm.bits} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{-DURATION['P10D']}",
        ],
        "nextev": None,
    }
    isctest.kasp.check_rollover_step(ns3, config, policy, step)
