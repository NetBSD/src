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

from rollover.common import CDSS, DEFAULT_CONFIG, DURATION, ROLLOVER_MARK
from rollover.setup import configure_root, configure_straight2none, configure_tld

import isctest

pytestmark = ROLLOVER_MARK


def bootstrap():
    data = {
        "tlds": [],
        "trust_anchors": [],
    }

    tlds = []
    tld_name = "kasp"
    delegations = configure_straight2none(tld_name)
    tld = configure_tld(tld_name, delegations)
    tlds.append(tld)
    data["tlds"].append(tld_name)
    ta = configure_root(tlds)
    data["trust_anchors"].append(ta)
    return data


@pytest.fixture(scope="module", autouse=True)
def after_servers_start(ns3, templates):
    isctest.kasp.wait_keymgr_done(ns3, "going-straight-to-none.kasp")
    isctest.kasp.wait_keymgr_done(ns3, "going-straight-to-none-dynamic.kasp")

    templates.render("ns3/named.conf", {"policy": "none"})
    ns3.reconfigure()


@pytest.mark.parametrize(
    "zone",
    [
        "going-straight-to-none.kasp",
        "going-straight-to-none-dynamic.kasp",
    ],
)
def test_straight2none_reconfig(zone, ns3, default_algorithm):
    config = DEFAULT_CONFIG
    policy = None

    step = {
        "zone": zone,
        "cdss": CDSS,
        # These zones will go bogus after signatures expire, but
        # remain validly signed for now.
        "keyprops": [
            f"csk 0 {default_algorithm.number} {default_algorithm.bits} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{-DURATION['P10D']}",
        ],
        "nextev": None,
    }
    isctest.kasp.check_rollover_step(ns3, config, policy, step)
