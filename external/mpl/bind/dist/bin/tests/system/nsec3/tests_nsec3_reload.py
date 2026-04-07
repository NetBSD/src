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

import shutil

from nsec3.common import NSEC3_MARK, check_nsec3_case

import isctest

pytestmark = NSEC3_MARK

# include the following zones when rendering named configs
ZONES = {
    "nsec3-fails-to-load.kasp",
}


def bootstrap():
    return {
        "zones": ZONES,
    }


def test_nsec3_case(ns3, default_algorithm):
    # Get test parameters.
    params = {
        "zone": "nsec3-fails-to-load.kasp",
        "policy": "nsec3",
        "key-properties": [
            f"csk 0 {default_algorithm.number} {default_algorithm.bits} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
        ],
    }
    zone = params["zone"]

    # nsec3-fails-to-load.kasp. fails to load (should be fixed after reload).
    zone = "nsec3-fails-to-load.kasp"
    with ns3.watch_log_from_start() as watcher:
        watcher.wait_for_line(f"zone {zone}/IN (unsigned): not loaded due to errors.")

    shutil.copyfile(f"{ns3.identifier}/template.db.in", f"{ns3.identifier}/{zone}.db")
    ns3.rndc(f"reload {zone}")

    # First make sure the zone is properly signed.
    isctest.kasp.wait_keymgr_done(ns3, zone)

    # Test case.
    check_nsec3_case(ns3, params)
