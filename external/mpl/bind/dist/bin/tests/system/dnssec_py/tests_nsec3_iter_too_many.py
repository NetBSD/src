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

from re import compile as Re

from dnssec_py.common import DNSSEC_PY_MARK
from isctest.template import NS2, NS3, zones
from isctest.zone import Zone, configure_root

import isctest

pytestmark = DNSSEC_PY_MARK


def bootstrap():
    sub = Zone("sub.nsec3-iter-too-many", NS3, signed=False)
    sub.configure()

    parent = Zone("nsec3-iter-too-many", NS2, signed=True)
    parent.delegations = [sub]
    parent.configure(sign_params="-3 A1B2C3D4 -H too-many -H 51")

    root = configure_root([parent])

    return {
        "trust_anchors": root.trust_anchors(),
        "zones": zones([root, parent, sub]),
    }


def test_excessive_nsec3_iterations_delegation(ns9):
    # reproducer for CVE-2026-1519 [GL#5708]
    zone = "a.sub.nsec3-iter-too-many"
    msg = isctest.query.create(zone, "A")
    res = isctest.query.tcp(msg, ns9.ip)

    # an insecure response is expected regardless of the NSEC3 iteration limit,
    # because the sub.nsec3-iter-too-many. zone is unsigned. the real
    # difference is in the CPU usage required for generating such response, but
    # that can't be easily and reliably tested in an automated fashion
    isctest.check.noerror(res)

    with ns9.watch_log_from_start() as watcher:
        watcher.wait_for_line(Re(f"validating {zone}/A:.*too many iterations"))
