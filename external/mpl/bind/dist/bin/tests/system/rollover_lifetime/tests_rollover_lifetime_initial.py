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

from isctest.util import param
from rollover.common import CDSS, DEFAULT_CONFIG, DURATION, ROLLOVER_MARK

import isctest

pytestmark = ROLLOVER_MARK


@pytest.mark.parametrize(
    "zone, policy, lifetime",
    [
        param("shorter-lifetime", "long-lifetime", "P1Y"),
        param("longer-lifetime", "short-lifetime", "P6M"),
        param("limit-lifetime", "unlimited-lifetime", 0),
        param("unlimit-lifetime", "short-lifetime", "P6M"),
    ],
)
def test_lifetime_initial(zone, policy, lifetime, ns3, default_algorithm):
    config = DEFAULT_CONFIG

    isctest.kasp.wait_keymgr_done(ns3, f"{zone}.kasp")

    step = {
        "zone": f"{zone}.kasp",
        "cdss": CDSS,
        "keyprops": [
            f"csk {DURATION[lifetime]} {default_algorithm.number} {default_algorithm.bits} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
        ],
        "nextev": None,
    }
    isctest.kasp.check_rollover_step(ns3, config, policy, step)
