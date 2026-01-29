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

import pytest

import isctest
from rollover.common import (
    pytestmark,
    alg,
    size,
    CDSS,
    DURATION,
    DEFAULT_CONFIG,
)
from rollover.setup import (
    configure_root,
    configure_tld,
    configure_straight2none,
)


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
def test_straight2none_reconfig(zone, ns3, alg, size):
    config = DEFAULT_CONFIG
    policy = None

    step = {
        "zone": zone,
        "cdss": CDSS,
        # These zones will go bogus after signatures expire, but
        # remain validly signed for now.
        "keyprops": [
            f"csk 0 {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{-DURATION['P10D']}",
        ],
        "nextev": None,
    }
    isctest.kasp.check_rollover_step(ns3, config, policy, step)
